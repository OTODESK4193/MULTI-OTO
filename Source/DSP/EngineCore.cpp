#include "EngineCore.h"
#include "FastMath.h"
#include <cmath>

EngineCore::EngineCore() {
    nodes.resize(MAX_NODES);
    crossovers.resize(MAX_NODES);
    dryCrossovers.resize(MAX_NODES);
    preLpfL.setType(juce::dsp::StateVariableTPTFilterType::lowpass); preLpfR.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    postHpfL.setType(juce::dsp::StateVariableTPTFilterType::highpass); postHpfR.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    postLpfL.setType(juce::dsp::StateVariableTPTFilterType::lowpass); postLpfR.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
}

void EngineCore::prepare(double sr, int spb) {
    currentSampleRate = sr;

    // ホストが宣言より大きいブロックを渡してきても落ちないよう、
    // 内部バッファは余裕を持たせつつ process() 側でも分割処理する。
    maxBlockSize = juce::jmax(64, spb);

    juce::dsp::ProcessSpec spec{ sr, static_cast<juce::uint32>(maxBlockSize), 2 };
    juce::dsp::ProcessSpec monoSpec{ sr, static_cast<juce::uint32>(maxBlockSize), 1 };

    for (auto& c : crossovers) c.prepare(spec);
    for (auto& dc : dryCrossovers) dc.prepare(spec);
    for (auto& n : nodes) n.prepare(sr, maxBlockSize);

    // IIR::Filter は mono。numChannels=2 を渡すと Debug ビルドで assert する。
    dcBlockL.prepare(monoSpec); dcBlockR.prepare(monoSpec);
    *dcBlockL.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, 10.0f);
    *dcBlockR.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, 10.0f);

    preLpfL.prepare(spec); preLpfR.prepare(spec); preLpfL.setCutoffFrequency(17000); preLpfR.setCutoffFrequency(17000);
    postHpfL.prepare(spec); postHpfR.prepare(spec); postLpfL.prepare(spec); postLpfR.prepare(spec);

    simdBuffer.resize(static_cast<size_t>(maxBlockSize));
    dryBuffer.setSize(2, maxBlockSize);

    limiterReleaseCoef = std::exp(-1.0f / (0.050f * static_cast<float>(sr)));
    driveSmoother.reset(sr, 0.05); oddSmoother.reset(sr, 0.05); evenSmoother.reset(sr, 0.05);
    inGainSmoother.reset(sr, 0.05); outGainSmoother.reset(sr, 0.05);

    activeCount = -1;
    cachedXLow = cachedXHigh = -1.0f;

    reset(); isPrepared.store(true);
}

void EngineCore::reset() {
    for (auto& c : crossovers) c.reset();
    for (auto& dc : dryCrossovers) dc.reset();
    for (auto& n : nodes) n.reset();
    preLpfL.reset(); preLpfR.reset(); satL.reset(); satR.reset();
    postHpfL.reset(); postHpfR.reset(); postLpfL.reset(); postLpfR.reset();
    dcBlockL.reset(); dcBlockR.reset();

    // これを消し忘れると、一度 Inf が入ったリミッターが
    // 永久に「ゲイン 0」を掛け続ける (旧・無音バグ)。
    limiterEnvL = limiterEnvR = 0.0f;
}

void EngineCore::updateParameters(const EngineParams& p) {
    currentParams = p;

    const int count = juce::jlimit(2, MAX_NODES, p.total_ott_count);
    currentParams.total_ott_count = count;

    driveSmoother.setTargetValue(p.drive); oddSmoother.setTargetValue(p.odd); evenSmoother.setTargetValue(p.even);
    inGainSmoother.setTargetValue(FastMath::fast_exp2(p.inGain * 0.16609f));
    outGainSmoother.setTargetValue(FastMath::fast_exp2(p.outGain * 0.16609f));

    // クロスオーバー係数の再計算は tan() を大量に呼ぶので、値が動いたときだけ。
    // (毎ブロック 128 個 × 2 セット × 10 フィルタを更新していた)
    if (p.xLow != cachedXLow || p.xHigh != cachedXHigh) {
        for (auto& c : crossovers) c.setFrequencies(p.xLow, p.xHigh);
        for (auto& dc : dryCrossovers) dc.setFrequencies(p.xLow, p.xHigh);
        cachedXLow = p.xLow; cachedXHigh = p.xHigh;
    }

    xoverLoAtomic.store(p.xLow, std::memory_order_relaxed);
    xoverHiAtomic.store(p.xHigh, std::memory_order_relaxed);

    postHpfL.setCutoffFrequency(p.post_hpf); postHpfR.setCutoffFrequency(p.post_hpf);
    postLpfL.setCutoffFrequency(p.post_lpf); postLpfR.setCutoffFrequency(p.post_lpf);

    // OTT 数が変わると Stage1/2 の担当ノードが入れ替わるので、全ノードを初期化。
    if (count != activeCount) {
        for (auto& n : nodes) n.reset();
        limiterEnvL = limiterEnvR = 0.0f;
        activeCount = count;
    }

    // 係数はステージごとに 1 回だけ計算して配る (旧実装は 128 回計算していた)
    const auto c1 = DynamicsNode::computeCoeffs(currentSampleRate, p.s1_gain, p.s1_depth,
                                                p.s1_up, p.s1_down, p.s1_time, p.s1_atk, p.s1_rel, p.s1_mix);
    const auto c2 = DynamicsNode::computeCoeffs(currentSampleRate, p.s2_gain, p.s2_depth,
                                                p.s2_up, p.s2_down, p.s2_time, p.s2_atk, p.s2_rel, p.s2_mix);

    const int half = count / 2;
    for (int i = 0; i < half; ++i)      nodes[i].applyCoeffs(c1);
    for (int i = half; i < count; ++i)  nodes[i].applyCoeffs(c2);

    currentLimitThreshold = FastMath::fast_exp2(p.limitCeil * 0.16609f);
}

void EngineCore::process(juce::AudioBuffer<float>& buffer) {
    if (!isPrepared.load()) { buffer.clear(); return; }
    if (buffer.getNumChannels() < 2) { buffer.clear(); return; }

    const int total = buffer.getNumSamples();
    float* left  = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);

    // 内部バッファ長を超えるブロックが来ても安全に処理できるよう分割する
    int offset = 0;
    while (offset < total) {
        const int n = juce::jmin(maxBlockSize, total - offset);
        processChunk(left + offset, right + offset, n);
        offset += n;
    }

    writeMeters();
}

void EngineCore::processChunk(float* left, float* right, int numSamples) {
    const int count = currentParams.total_ott_count;
    const int half  = count / 2;

    // 人工ノイズは一切なし。無音時は完全なるデジタル・ゼロを維持します。
    float* dL = dryBuffer.getWritePointer(0);
    float* dR = dryBuffer.getWritePointer(1);
    for (int i = 0; i < numSamples; ++i) { dL[i] = left[i]; dR[i] = right[i]; }

    // ALIGN PHASE: DRY 側にも WET と同じ位相回転を通す。
    // これで DRY/WET を混ぜても位相キャンセル (コムフィルタ) が起きない。
    if (currentParams.phase_mode == 1) {
        for (int n = 0; n < count; ++n) {
            const bool isOn = (n < half) ? currentParams.s1_on : currentParams.s2_on;
            if (isOn)
                for (int i = 0; i < numSamples; ++i)
                    dryCrossovers[n].processDry(dL[i], dR[i], dL[i], dR[i]);
        }
    }

    const float inG = inGainSmoother.getNextValue(); inGainSmoother.skip(numSamples - 1);
    juce::FloatVectorOperations::multiply(left, inG, numSamples);
    juce::FloatVectorOperations::multiply(right, inG, numSamples);

    if (currentParams.predrive_on) {
        for (int i = 0; i < numSamples; ++i) {
            left[i]  = preLpfL.processSample(0, left[i]);
            right[i] = preLpfR.processSample(1, right[i]);
        }

        // skip() を忘れるとスムーザーが 1 ブロックにつき 1 サンプルしか進まず、
        // Drive / ODD / EVEN が目標値に届くまで数十秒かかっていた。
        const float dv = driveSmoother.getNextValue(); driveSmoother.skip(numSamples - 1);
        const float ov = oddSmoother.getNextValue();   oddSmoother.skip(numSamples - 1);
        const float ev = evenSmoother.getNextValue();  evenSmoother.skip(numSamples - 1);

        satL.processBlock_AVX2(left,  numSamples, dv, ov, ev);
        satR.processBlock_AVX2(right, numSamples, dv, ov, ev);

        // サチュレーターの DC オフセットを殺し、無音時のジーノイズ発振を防ぐ
        for (int i = 0; i < numSamples; ++i) {
            left[i]  = dcBlockL.processSample(left[i]);
            right[i] = dcBlockR.processSample(right[i]);
        }
    }

    // 【ドロピー音の源泉】 毎ステージで「分割 → コンプ → 再合成」を行い、位相を意図的にねじ曲げる
    for (int n = 0; n < count; ++n) {
        const bool isOn = (n < half) ? currentParams.s1_on : currentParams.s2_on;
        if (!isOn) continue;

        for (int i = 0; i < numSamples; ++i) {
            float lL, lR, mL, mR, hL, hR;
            crossovers[n].process(left[i], right[i], lL, lR, mL, mR, hL, hR);
            alignas(32) float raw[8] = { lL, lR, mL, mR, hL, hR, 0, 0 };
            simdBuffer[static_cast<size_t>(i)] = juce::dsp::SIMDRegister<float>::fromRawArray(raw);
        }

        nodes[n].process(simdBuffer.data(), numSamples);

        for (int i = 0; i < numSamples; ++i) {
            alignas(32) float raw[8];
            simdBuffer[static_cast<size_t>(i)].copyToRawArray(raw);
            left[i]  = raw[0] + raw[2] + raw[4];
            right[i] = raw[1] + raw[3] + raw[5];
        }
    }

    const float outG = outGainSmoother.getNextValue(); outGainSmoother.skip(numSamples - 1);
    const float kCeiling = DynamicsNode::kSafeCeiling;

    for (int i = 0; i < numSamples; ++i) {
        const float wL = postLpfL.processSample(0, postHpfL.processSample(0, left[i]));
        const float wR = postLpfR.processSample(1, postHpfR.processSample(1, right[i]));

        float oL = (dL[i] + (wL - dL[i]) * currentParams.dryWet * 0.01f) * outG;
        float oR = (dR[i] + (wR - dR[i]) * currentParams.dryWet * 0.01f) * outG;

        // フィルタ内部の状態が壊れていた場合の最終防衛線
        if (!std::isfinite(oL)) oL = 0.0f;
        if (!std::isfinite(oR)) oR = 0.0f;
        oL = juce::jlimit(-kCeiling, kCeiling, oL);
        oR = juce::jlimit(-kCeiling, kCeiling, oR);

        limiterEnvL = std::max(std::abs(oL), limiterEnvL * limiterReleaseCoef);
        limiterEnvR = std::max(std::abs(oR), limiterEnvR * limiterReleaseCoef);
        if (!std::isfinite(limiterEnvL)) limiterEnvL = 0.0f;
        if (!std::isfinite(limiterEnvR)) limiterEnvR = 0.0f;

        left[i]  = oL * ((limiterEnvL > currentLimitThreshold) ? (currentLimitThreshold / limiterEnvL) : 1.0f);
        right[i] = oR * ((limiterEnvR > currentLimitThreshold) ? (currentLimitThreshold / limiterEnvR) : 1.0f);
    }
}

void EngineCore::writeMeters() {
    const int count = currentParams.total_ott_count;
    const int half  = count / 2;
    float envDb[3], gainDb[3];

    if (currentParams.s1_on && count > 0) {
        nodes[0].getLastMeter(envDb, gainDb);
        for (int b = 0; b < 3; ++b) {
            s1Meter.envDb[b].store(envDb[b], std::memory_order_relaxed);
            s1Meter.gainDb[b].store(gainDb[b], std::memory_order_relaxed);
        }
    } else {
        for (int b = 0; b < 3; ++b) {
            s1Meter.envDb[b].store(-60.0f, std::memory_order_relaxed);
            s1Meter.gainDb[b].store(0.0f, std::memory_order_relaxed);
        }
    }

    if (currentParams.s2_on && half < count) {
        nodes[half].getLastMeter(envDb, gainDb);
        for (int b = 0; b < 3; ++b) {
            s2Meter.envDb[b].store(envDb[b], std::memory_order_relaxed);
            s2Meter.gainDb[b].store(gainDb[b], std::memory_order_relaxed);
        }
    } else {
        for (int b = 0; b < 3; ++b) {
            s2Meter.envDb[b].store(-60.0f, std::memory_order_relaxed);
            s2Meter.gainDb[b].store(0.0f, std::memory_order_relaxed);
        }
    }
}

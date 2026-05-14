#include "EngineCore.h"
#include "FastMath.h"
#include <cmath>

EngineCore::EngineCore() {
    nodes.resize(NUM_NODES);

    postHpfL.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    postHpfR.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    postLpfL.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    postLpfR.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
}

void EngineCore::prepare(double sampleRate, int samplesPerBlock) {
    juce::dsp::ProcessSpec spec{ sampleRate, static_cast<juce::uint32>(samplesPerBlock), 2 };

    crossover.prepare(spec);
    dummyCrossover.prepare(spec);
    for (auto& node : nodes) node.prepare(sampleRate, samplesPerBlock);

    postHpfL.prepare(spec); postHpfR.prepare(spec);
    postLpfL.prepare(spec); postLpfR.prepare(spec);

    simdBuffer.resize(static_cast<size_t>(samplesPerBlock));
    dryBuffer.setSize(2, samplesPerBlock, false, true, true); // 明示的ゼロクリア

    limiterReleaseCoef = std::exp(-1.0f / (0.050f * static_cast<float>(sampleRate)));

    // スムーザーの初期化
    driveSmoother.reset(sampleRate, 0.05); // 50ms smooth
    oddSmoother.reset(sampleRate, 0.05);
    evenSmoother.reset(sampleRate, 0.05);

    reset();
}

void EngineCore::reset() {
    crossover.reset();
    dummyCrossover.reset();
    for (auto& node : nodes) node.reset();
    satL.reset(); satR.reset();
    postHpfL.reset(); postHpfR.reset();
    postLpfL.reset(); postLpfR.reset();
    limiterEnvL = 0.0f; limiterEnvR = 0.0f;
}

void EngineCore::updateParameters(const EngineParams& p) {
    currentParams = p;

    driveSmoother.setTargetValue(p.drive);
    oddSmoother.setTargetValue(p.odd);
    evenSmoother.setTargetValue(p.even);

    crossover.setFrequencies(p.xLow, p.xHigh);
    dummyCrossover.setFrequencies(p.xLow, p.xHigh);

    postHpfL.setCutoffFrequency(p.post_hpf); postHpfR.setCutoffFrequency(p.post_hpf);
    postLpfL.setCutoffFrequency(p.post_lpf); postLpfR.setCutoffFrequency(p.post_lpf);

    nodes[0].setParameters(p.s1_gain, p.s1_depth, p.s1_time, p.s1_atk, p.s1_rel);
    nodes[1].setParameters(p.s2_gain, p.s2_depth, p.s2_time, p.s2_atk, p.s2_rel);

    currentLimitThreshold = FastMath::fast_exp2(p.limitCeil * 0.16609f);
}

void EngineCore::process(juce::AudioBuffer<float>& buffer) {
    const int numSamples = buffer.getNumSamples();
    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);
    float* dryLeft = dryBuffer.getWritePointer(0);
    float* dryRight = dryBuffer.getWritePointer(1);

    float inGainLin = FastMath::fast_exp2(currentParams.inGain * 0.16609f);
    float outGainLin = FastMath::fast_exp2(currentParams.outGain * 0.16609f);
    float mix = currentParams.dryWet * 0.01f;

    // =========================================================================
    // STEP 1: PURE DRY BUFFERING (Phase Mode Alignment)
    // =========================================================================
    // Input Gain を適用する *前* に、純粋な原音を Dry バッファに退避・位相補正を行う。
    for (int i = 0; i < numSamples; ++i) {
        float l = left[i];
        float r = right[i];

        float matchedL = l;
        float matchedR = r;

        if (currentParams.phase_mode == 1) {
            float d_lL, d_lR, d_mL, d_mR, d_hL, d_hR;
            // Phase Mode Align: フィルターネットワークの群遅延のみをDry音に付加する
            dummyCrossover.process(l, r, d_lL, d_lR, d_mL, d_mR, d_hL, d_hR);
            matchedL = d_lL + d_mL + d_hL;
            matchedR = d_lR + d_mR + d_hR;
        }
        dryLeft[i] = matchedL;
        dryRight[i] = matchedR;
    }

    // =========================================================================
    // STEP 2: INPUT GAIN STAGE
    // =========================================================================
    // 以降の処理（Wetパス）のために、メインバッファに Input Gain を適用する。
    // SIMD最適化された一括乗算を利用。
    juce::FloatVectorOperations::multiply(left, inGainLin, numSamples);
    juce::FloatVectorOperations::multiply(right, inGainLin, numSamples);

    // =========================================================================
    // STEP 3: ADAA SATURATION (Wet Path Only)
    // =========================================================================
    float driveTgt = driveSmoother.getNextValue();
    float oddTgt = oddSmoother.getNextValue();
    float evenTgt = evenSmoother.getNextValue();
    driveSmoother.skip(numSamples - 1);
    oddSmoother.skip(numSamples - 1);
    evenSmoother.skip(numSamples - 1);

    if (driveTgt > 0.01f || oddTgt > 0.01f || evenTgt > 0.01f) {
        satL.processBlock_AVX2(left, numSamples, driveTgt, oddTgt, evenTgt);
        satR.processBlock_AVX2(right, numSamples, driveTgt, oddTgt, evenTgt);
    }

    // =========================================================================
    // STEP 4: CROSSOVER & MULTI-STAGE OTT
    // =========================================================================
    for (int i = 0; i < numSamples; ++i) {
        float lL, lR, mL, mR, hL, hR;
        // サチュレーション通過後の信号(left/right)を帯域分割
        crossover.process(left[i], right[i], lL, lR, mL, mR, hL, hR);

        // SIMDレジスタ(256-bit)へのパッキング (SoA構造)
        alignas(32) float raw[8] = { lL, lR, mL, mR, hL, hR, 0.0f, 0.0f };
        simdBuffer[static_cast<size_t>(i)] = juce::dsp::SIMDRegister<float>::fromRawArray(raw);
    }

    // 各DynamicsNode（OTTステージ）の実行
    for (auto& node : nodes) {
        node.process(simdBuffer.data(), numSamples);
    }

    // =========================================================================
    // STEP 5: RE-SYNTHESIS, MIX & MASTER
    // =========================================================================
    for (int i = 0; i < numSamples; ++i) {
        alignas(32) float raw[8];
        simdBuffer[static_cast<size_t>(i)].copyToRawArray(raw);

        // 帯域の再合成 (Wetパス)
        float wetL = raw[0] + raw[2] + raw[4];
        float wetR = raw[1] + raw[3] + raw[5];

        // Post フィルターの適用
        wetL = postLpfL.processSample(0, postHpfL.processSample(0, wetL));
        wetR = postLpfR.processSample(1, postHpfR.processSample(1, wetR));

        // Dry / Wet ミックス (STEP 1で退避した純粋なDry音を使用)
        float outL = dryLeft[i] + (wetL - dryLeft[i]) * mix;
        float outR = dryRight[i] + (wetR - dryRight[i]) * mix;

        outL *= outGainLin;
        outR *= outGainLin;

        // Safety Limiter (エンベロープベースのクリッパー)
        float absL = std::abs(outL);
        float absR = std::abs(outR);
        limiterEnvL = (absL > limiterEnvL) ? absL : limiterEnvL * limiterReleaseCoef;
        limiterEnvR = (absR > limiterEnvR) ? absR : limiterEnvR * limiterReleaseCoef;

        float gainL = (limiterEnvL > currentLimitThreshold) ? (currentLimitThreshold / limiterEnvL) : 1.0f;
        float gainR = (limiterEnvR > currentLimitThreshold) ? (currentLimitThreshold / limiterEnvR) : 1.0f;

        left[i] = outL * gainL;
        right[i] = outR * gainR;
    }
}
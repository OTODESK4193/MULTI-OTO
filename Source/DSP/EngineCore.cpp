#include "EngineCore.h"
#include "FastMath.h"

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
    juce::dsp::ProcessSpec spec{ sr, static_cast<juce::uint32>(spb), 2 };

    for (auto& c : crossovers) c.prepare(spec);
    for (auto& dc : dryCrossovers) dc.prepare(spec);
    for (auto& n : nodes) n.prepare(sr, spb);

    *dcBlockL.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, 10.0f);
    *dcBlockR.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, 10.0f);
    dcBlockL.prepare(spec); dcBlockR.prepare(spec);

    preLpfL.prepare(spec); preLpfR.prepare(spec); preLpfL.setCutoffFrequency(17000); preLpfR.setCutoffFrequency(17000);
    postHpfL.prepare(spec); postHpfR.prepare(spec); postLpfL.prepare(spec); postLpfR.prepare(spec);
    simdBuffer.resize(spb); dryBuffer.setSize(2, spb);
    limiterReleaseCoef = std::exp(-1.0f / (0.050f * static_cast<float>(sr)));
    driveSmoother.reset(sr, 0.05); oddSmoother.reset(sr, 0.05); evenSmoother.reset(sr, 0.05); inGainSmoother.reset(sr, 0.05); outGainSmoother.reset(sr, 0.05);
    reset(); isPrepared.store(true);
}

void EngineCore::reset() {
    for (auto& c : crossovers) c.reset();
    for (auto& dc : dryCrossovers) dc.reset();
    for (auto& n : nodes) n.reset();
    preLpfL.reset(); preLpfR.reset(); satL.reset(); satR.reset(); postHpfL.reset(); postHpfR.reset(); postLpfL.reset(); postLpfR.reset();
    dcBlockL.reset(); dcBlockR.reset();
}

void EngineCore::updateParameters(const EngineParams& p) {
    currentParams = p;
    driveSmoother.setTargetValue(p.drive); oddSmoother.setTargetValue(p.odd); evenSmoother.setTargetValue(p.even);
    inGainSmoother.setTargetValue(FastMath::fast_exp2(p.inGain * 0.16609f)); outGainSmoother.setTargetValue(FastMath::fast_exp2(p.outGain * 0.16609f));

    for (auto& c : crossovers) c.setFrequencies(p.xLow, p.xHigh);
    for (auto& dc : dryCrossovers) dc.setFrequencies(p.xLow, p.xHigh);

    postHpfL.setCutoffFrequency(p.post_hpf); postHpfR.setCutoffFrequency(p.post_hpf); postLpfL.setCutoffFrequency(p.post_lpf); postLpfR.setCutoffFrequency(p.post_lpf);
    int half = p.total_ott_count / 2;
    for (int i = 0; i < half; ++i) nodes[i].setParameters(p.s1_gain, p.s1_depth, p.s1_time, p.s1_atk, p.s1_rel, p.s1_mix);
    for (int i = half; i < p.total_ott_count; ++i) nodes[i].setParameters(p.s2_gain, p.s2_depth, p.s2_time, p.s2_atk, p.s2_rel, p.s2_mix);
    currentLimitThreshold = FastMath::fast_exp2(p.limitCeil * 0.16609f);
}

void EngineCore::process(juce::AudioBuffer<float>& buffer) {
    if (!isPrepared.load()) { buffer.clear(); return; }
    const int numSamples = buffer.getNumSamples();
    float* left = buffer.getWritePointer(0); float* right = buffer.getWritePointer(1);

    // 人工ノイズは一切なし。無音時は完全なるデジタル・ゼロを維持します。

    dryBuffer.clear(); float* dL = dryBuffer.getWritePointer(0); float* dR = dryBuffer.getWritePointer(1);

    for (int i = 0; i < numSamples; ++i) { dL[i] = left[i]; dR[i] = right[i]; }

    if (currentParams.phase_mode == 1) {
        for (int n = 0; n < currentParams.total_ott_count; ++n) {
            bool isOn = (n < currentParams.total_ott_count / 2) ? currentParams.s1_on : currentParams.s2_on;
            if (isOn) {
                for (int i = 0; i < numSamples; ++i) dryCrossovers[n].processDry(dL[i], dR[i], dL[i], dR[i]);
            }
        }
    }

    float inG = inGainSmoother.getNextValue(); inGainSmoother.skip(numSamples - 1);
    juce::FloatVectorOperations::multiply(left, inG, numSamples); juce::FloatVectorOperations::multiply(right, inG, numSamples);

    if (currentParams.predrive_on) {
        for (int i = 0; i < numSamples; ++i) { left[i] = preLpfL.processSample(0, left[i]); right[i] = preLpfR.processSample(1, right[i]); }
        satL.processBlock_AVX2(left, numSamples, driveSmoother.getNextValue(), oddSmoother.getNextValue(), evenSmoother.getNextValue());
        satR.processBlock_AVX2(right, numSamples, driveSmoother.getCurrentValue(), oddSmoother.getCurrentValue(), evenSmoother.getCurrentValue());

        // サチュレーターのDCオフセットを完全に殺し、無音時のジーノイズ発振を防ぐ
        for (int i = 0; i < numSamples; ++i) {
            left[i] = dcBlockL.processSample(left[i]);
            right[i] = dcBlockR.processSample(right[i]);
        }
    }

    // 【ドロピー音の源泉】 毎ステージで「分割 → コンプ → 再合成」を行い、位相を意図的にねじ曲げる
    int half = currentParams.total_ott_count / 2;
    for (int n = 0; n < currentParams.total_ott_count; ++n) {
        bool isOn = (n < half) ? currentParams.s1_on : currentParams.s2_on;
        if (isOn) {
            for (int i = 0; i < numSamples; ++i) {
                float lL, lR, mL, mR, hL, hR;
                crossovers[n].process(left[i], right[i], lL, lR, mL, mR, hL, hR);
                alignas(32) float raw[8] = { lL, lR, mL, mR, hL, hR, 0, 0 };
                simdBuffer[i] = juce::dsp::SIMDRegister<float>::fromRawArray(raw);
            }
            nodes[n].process(simdBuffer.data(), numSamples);
            for (int i = 0; i < numSamples; ++i) {
                alignas(32) float raw[8]; simdBuffer[i].copyToRawArray(raw);
                left[i] = raw[0] + raw[2] + raw[4];
                right[i] = raw[1] + raw[3] + raw[5];
            }
        }
    }

    float outG = outGainSmoother.getNextValue(); outGainSmoother.skip(numSamples - 1);
    for (int i = 0; i < numSamples; ++i) {
        float wL = postLpfL.processSample(0, postHpfL.processSample(0, left[i]));
        float wR = postLpfR.processSample(1, postHpfR.processSample(1, right[i]));
        float oL = (dL[i] + (wL - dL[i]) * currentParams.dryWet * 0.01f) * outG;
        float oR = (dR[i] + (wR - dR[i]) * currentParams.dryWet * 0.01f) * outG;
        limiterEnvL = std::max(std::abs(oL), limiterEnvL * limiterReleaseCoef); limiterEnvR = std::max(std::abs(oR), limiterEnvR * limiterReleaseCoef);
        left[i] = oL * ((limiterEnvL > currentLimitThreshold) ? (currentLimitThreshold / limiterEnvL) : 1.0f);
        right[i] = oR * ((limiterEnvR > currentLimitThreshold) ? (currentLimitThreshold / limiterEnvR) : 1.0f);
    }
}
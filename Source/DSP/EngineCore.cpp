#include "EngineCore.h"
#include "FastMath.h"

EngineCore::EngineCore() {
    nodes.resize(MAX_NODES);
    preLpfL.setType(juce::dsp::StateVariableTPTFilterType::lowpass); preLpfR.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    postHpfL.setType(juce::dsp::StateVariableTPTFilterType::highpass); postHpfR.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    postLpfL.setType(juce::dsp::StateVariableTPTFilterType::lowpass); postLpfR.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
}

void EngineCore::prepare(double sr, int spb) {
    currentSampleRate = sr;
    juce::dsp::ProcessSpec spec{ sr, static_cast<juce::uint32>(spb), 2 };
    crossover.prepare(spec);
    for (auto& n : nodes) n.prepare(sr, spb);
    preLpfL.prepare(spec); preLpfR.prepare(spec); preLpfL.setCutoffFrequency(17000); preLpfR.setCutoffFrequency(17000);
    postHpfL.prepare(spec); postHpfR.prepare(spec); postLpfL.prepare(spec); postLpfR.prepare(spec);
    simdBuffer.resize(spb); dryBuffer.setSize(2, spb);
    limiterReleaseCoef = std::exp(-1.0f / (0.050f * static_cast<float>(sr)));
    driveSmoother.reset(sr, 0.05); oddSmoother.reset(sr, 0.05); evenSmoother.reset(sr, 0.05); inGainSmoother.reset(sr, 0.05); outGainSmoother.reset(sr, 0.05);
    reset(); isPrepared.store(true);
}

void EngineCore::reset() {
    crossover.reset(); for (auto& n : nodes) n.reset();
    preLpfL.reset(); preLpfR.reset(); satL.reset(); satR.reset(); postHpfL.reset(); postHpfR.reset(); postLpfL.reset(); postLpfR.reset();
}

void EngineCore::updateParameters(const EngineParams& p) {
    currentParams = p;
    driveSmoother.setTargetValue(p.drive); oddSmoother.setTargetValue(p.odd); evenSmoother.setTargetValue(p.even);
    inGainSmoother.setTargetValue(FastMath::fast_exp2(p.inGain * 0.16609f)); outGainSmoother.setTargetValue(FastMath::fast_exp2(p.outGain * 0.16609f));
    crossover.setFrequencies(p.xLow, p.xHigh);
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

    // 【修正】人工的なディザー（ホワイトノイズ）注入のループを完全に削除しました。
    // これにより、入力が無音(0.0)の時は完全に静寂を保ちます。

    dryBuffer.clear(); float* dL = dryBuffer.getWritePointer(0); float* dR = dryBuffer.getWritePointer(1);

    for (int i = 0; i < numSamples; ++i) {
        if (currentParams.phase_mode == 1) crossover.processDry(left[i], right[i], dL[i], dR[i]);
        else { dL[i] = left[i]; dR[i] = right[i]; }
    }

    float inG = inGainSmoother.getNextValue(); inGainSmoother.skip(numSamples - 1);
    juce::FloatVectorOperations::multiply(left, inG, numSamples); juce::FloatVectorOperations::multiply(right, inG, numSamples);

    if (currentParams.predrive_on) {
        for (int i = 0; i < numSamples; ++i) { left[i] = preLpfL.processSample(0, left[i]); right[i] = preLpfR.processSample(1, right[i]); }
        satL.processBlock_AVX2(left, numSamples, driveSmoother.getNextValue(), oddSmoother.getNextValue(), evenSmoother.getNextValue());
        satR.processBlock_AVX2(right, numSamples, driveSmoother.getCurrentValue(), oddSmoother.getCurrentValue(), evenSmoother.getCurrentValue());
    }

    for (int i = 0; i < numSamples; ++i) {
        float lL, lR, mL, mR, hL, hR; crossover.process(left[i], right[i], lL, lR, mL, mR, hL, hR);
        alignas(32) float raw[8] = { lL, lR, mL, mR, hL, hR, 0, 0 }; simdBuffer[i] = juce::dsp::SIMDRegister<float>::fromRawArray(raw);
    }

    int half = currentParams.total_ott_count / 2;
    for (int n = 0; n < currentParams.total_ott_count; ++n) {
        if ((n < half && currentParams.s1_on) || (n >= half && currentParams.s2_on)) nodes[n].process(simdBuffer.data(), numSamples);
    }

    float outG = outGainSmoother.getNextValue(); outGainSmoother.skip(numSamples - 1);
    for (int i = 0; i < numSamples; ++i) {
        alignas(32) float raw[8]; simdBuffer[i].copyToRawArray(raw);
        float wL = postLpfL.processSample(0, postHpfL.processSample(0, raw[0] + raw[2] + raw[4]));
        float wR = postLpfR.processSample(1, postHpfR.processSample(1, raw[1] + raw[3] + raw[5]));
        float oL = (dL[i] + (wL - dL[i]) * currentParams.dryWet * 0.01f) * outG;
        float oR = (dR[i] + (wR - dR[i]) * currentParams.dryWet * 0.01f) * outG;
        limiterEnvL = std::max(std::abs(oL), limiterEnvL * limiterReleaseCoef); limiterEnvR = std::max(std::abs(oR), limiterEnvR * limiterReleaseCoef);
        left[i] = oL * ((limiterEnvL > currentLimitThreshold) ? (currentLimitThreshold / limiterEnvL) : 1.0f);
        right[i] = oR * ((limiterEnvR > currentLimitThreshold) ? (currentLimitThreshold / limiterEnvR) : 1.0f);
    }
}
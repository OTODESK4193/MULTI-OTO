#include "EngineCore.h"
#include "FastMath.h"
#include <cmath>

EngineCore::EngineCore() {
    nodes.resize(MAX_NODES);

    preLpfL.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    preLpfR.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    postHpfL.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    postHpfR.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    postLpfL.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    postLpfR.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
}

void EngineCore::prepare(double sampleRate, int samplesPerBlock) {
    currentSampleRate = sampleRate;
    juce::dsp::ProcessSpec spec{ sampleRate, static_cast<juce::uint32>(samplesPerBlock), 2 };

    crossover.prepare(spec);
    for (auto& node : nodes) node.prepare(sampleRate, samplesPerBlock);

    preLpfL.prepare(spec); preLpfR.prepare(spec);
    preLpfL.setCutoffFrequency(17000.0f); preLpfR.setCutoffFrequency(17000.0f);
    preLpfL.setResonance(0.707106f); preLpfR.setResonance(0.707106f);

    postHpfL.prepare(spec); postHpfR.prepare(spec);
    postLpfL.prepare(spec); postLpfR.prepare(spec);

    simdBuffer.resize(static_cast<size_t>(samplesPerBlock));
    dryBuffer.setSize(2, samplesPerBlock, false, true, true);

    limiterReleaseCoef = std::exp(-1.0f / (0.050f * static_cast<float>(sampleRate)));

    driveSmoother.reset(sampleRate, 0.05);
    oddSmoother.reset(sampleRate, 0.05);
    evenSmoother.reset(sampleRate, 0.05);
    inGainSmoother.reset(sampleRate, 0.05);
    outGainSmoother.reset(sampleRate, 0.05);

    reset();
    isPrepared.store(true, std::memory_order_release);
}

void EngineCore::reset() {
    crossover.reset();
    for (auto& node : nodes) node.reset();
    preLpfL.reset(); preLpfR.reset();
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
    inGainSmoother.setTargetValue(FastMath::fast_exp2(p.inGain * 0.16609f));
    outGainSmoother.setTargetValue(FastMath::fast_exp2(p.outGain * 0.16609f));

    crossover.setFrequencies(p.xLow, p.xHigh);
    postHpfL.setCutoffFrequency(p.post_hpf); postHpfR.setCutoffFrequency(p.post_hpf);
    postLpfL.setCutoffFrequency(p.post_lpf); postLpfR.setCutoffFrequency(p.post_lpf);

    // 意図通り、Stage 1 と Stage 2 に半分ずつパラメータを分配
    int half = p.total_ott_count / 2;
    for (int i = 0; i < half; ++i) {
        nodes[i].setParameters(p.s1_gain, p.s1_depth, p.s1_time, p.s1_atk, p.s1_rel);
    }
    for (int i = half; i < p.total_ott_count; ++i) {
        nodes[i].setParameters(p.s2_gain, p.s2_depth, p.s2_time, p.s2_atk, p.s2_rel);
    }

    currentLimitThreshold = FastMath::fast_exp2(p.limitCeil * 0.16609f);
}

void EngineCore::process(juce::AudioBuffer<float>& buffer) {
    juce::ScopedNoDenormals noDenormals;
    if (!isPrepared.load(std::memory_order_acquire)) { buffer.clear(); return; }

    const int numSamples = buffer.getNumSamples();
    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);

    dryBuffer.clear();
    float* dryLeft = dryBuffer.getWritePointer(0);
    float* dryRight = dryBuffer.getWritePointer(1);
    float mix = currentParams.dryWet * 0.01f;

    // --- STEP 1: DRY BUFFERING (位相アライン対応) ---
    for (int i = 0; i < numSamples; ++i) {
        if (currentParams.phase_mode == 1) { // ALIGN MODE
            crossover.processDry(left[i], right[i], dryLeft[i], dryRight[i]);
        }
        else { // COLOR MODE
            dryLeft[i] = left[i];
            dryRight[i] = right[i];
        }
    }

    // --- STEP 2: INPUT GAIN ---
    float inGainTgt = inGainSmoother.getNextValue();
    inGainSmoother.skip(numSamples - 1);
    juce::FloatVectorOperations::multiply(left, inGainTgt, numSamples);
    juce::FloatVectorOperations::multiply(right, inGainTgt, numSamples);

    // --- STEP 3: PRE-FILTERING & ADAA SATURATION ---
    float driveTgt = driveSmoother.getNextValue();
    float oddTgt = oddSmoother.getNextValue();
    float evenTgt = evenSmoother.getNextValue();
    driveSmoother.skip(numSamples - 1);
    oddSmoother.skip(numSamples - 1);
    evenSmoother.skip(numSamples - 1);

    if (driveTgt > 0.01f || oddTgt > 0.01f || evenTgt > 0.01f) {
        for (int i = 0; i < numSamples; ++i) {
            left[i] = preLpfL.processSample(0, left[i]);
            right[i] = preLpfR.processSample(1, right[i]);
        }
        satL.processBlock_AVX2(left, numSamples, driveTgt, oddTgt, evenTgt);
        satR.processBlock_AVX2(right, numSamples, driveTgt, oddTgt, evenTgt);
    }

    // --- STEP 4: CROSSOVER & OTT (64段動的ループ) ---
    for (int i = 0; i < numSamples; ++i) {
        float lL, lR, mL, mR, hL, hR;
        crossover.process(left[i], right[i], lL, lR, mL, mR, hL, hR);
        alignas(32) float raw[8] = { lL, lR, mL, mR, hL, hR, 0.0f, 0.0f };
        simdBuffer[static_cast<size_t>(i)] = juce::dsp::SIMDRegister<float>::fromRawArray(raw);
    }

    // 指定された total_ott_count 分だけ直列に回す
    for (int n = 0; n < currentParams.total_ott_count; ++n) {
        nodes[n].process(simdBuffer.data(), numSamples);
    }

    // --- STEP 5: RESYNTHESIS & MIX ---
    float outGainTgt = outGainSmoother.getNextValue();
    outGainSmoother.skip(numSamples - 1);

    for (int i = 0; i < numSamples; ++i) {
        alignas(32) float raw[8];
        simdBuffer[static_cast<size_t>(i)].copyToRawArray(raw);

        // 位相補正済みのため、単に足すだけでフラットになる
        float wetL = raw[0] + raw[2] + raw[4];
        float wetR = raw[1] + raw[3] + raw[5];

        wetL = postLpfL.processSample(0, postHpfL.processSample(0, wetL));
        wetR = postLpfR.processSample(1, postHpfR.processSample(1, wetR));

        float outL = (dryLeft[i] + (wetL - dryLeft[i]) * mix) * outGainTgt;
        float outR = (dryRight[i] + (wetR - dryRight[i]) * mix) * outGainTgt;

        float absL = std::abs(outL); float absR = std::abs(outR);
        limiterEnvL = (absL > limiterEnvL) ? absL : limiterEnvL * limiterReleaseCoef;
        limiterEnvR = (absR > limiterEnvR) ? absR : limiterEnvR * limiterReleaseCoef;

        float gainL = (limiterEnvL > currentLimitThreshold) ? (currentLimitThreshold / limiterEnvL) : 1.0f;
        float gainR = (limiterEnvR > currentLimitThreshold) ? (currentLimitThreshold / limiterEnvR) : 1.0f;

        left[i] = outL * gainL;
        right[i] = outR * gainR;
    }
}
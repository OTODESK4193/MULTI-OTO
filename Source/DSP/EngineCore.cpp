#include "EngineCore.h"
#include "FastMath.h"
#include <cmath>
#include <algorithm>

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
    dryBuffer.setSize(2, samplesPerBlock); // オーディオスレッド外での安全なメモリ確保

    limiterReleaseCoef = std::exp(-1.0f / (0.050f * static_cast<float>(sampleRate)));
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
    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    auto* dryLeft = dryBuffer.getWritePointer(0);
    auto* dryRight = dryBuffer.getWritePointer(1);

    float inGainLin = FastMath::fast_exp2(currentParams.inGain * 0.16609f);
    float outGainLin = FastMath::fast_exp2(currentParams.outGain * 0.16609f);
    float mix = currentParams.dryWet * 0.01f;

    // --- 1. PRE-DRIVE & DRY BUFFERING ---
    for (int i = 0; i < numSamples; ++i) {
        float l = left[i] * inGainLin;
        float r = right[i] * inGainLin;

        if (currentParams.drive > 0.1f) {
            l = static_cast<float>(satL.processSample(l, currentParams.drive, currentParams.odd, currentParams.even));
            r = static_cast<float>(satR.processSample(r, currentParams.drive, currentParams.odd, currentParams.even));
        }

        // Phase Mode に応じた Dry音の退避
        float matchedL = l;
        float matchedR = r;
        if (currentParams.phase_mode == 1) { // Align Mode: ダミークロスオーバー通過
            float d_lL, d_lR, d_mL, d_mR, d_hL, d_hR;
            dummyCrossover.process(l, r, d_lL, d_lR, d_mL, d_mR, d_hL, d_hR);
            matchedL = d_lL + d_mL + d_hL;
            matchedR = d_lR + d_mR + d_hR;
        }
        dryLeft[i] = matchedL;
        dryRight[i] = matchedR;

        // 本線のクロスオーバー分割
        float lL, lR, mL, mR, hL, hR;
        crossover.process(l, r, lL, lR, mL, mR, hL, hR);
        alignas(32) float raw[8] = { lL, lR, mL, mR, hL, hR, 0.0f, 0.0f };
        simdBuffer[static_cast<size_t>(i)] = juce::dsp::SIMDRegister<float>::fromRawArray(raw);
    }

    // --- 2. MULTI-STAGE OTT ---
    for (auto& node : nodes) {
        node.process(simdBuffer.data(), numSamples);
    }

    // --- 3. RE-SYNTHESIS & MASTER ---
    for (int i = 0; i < numSamples; ++i) {
        alignas(32) float raw[8];
        simdBuffer[static_cast<size_t>(i)].copyToRawArray(raw);

        float wetL = raw[0] + raw[2] + raw[4];
        float wetR = raw[1] + raw[3] + raw[5];

        // Post Filter の適用
        wetL = postHpfL.processSample(0, wetL);
        wetL = postLpfL.processSample(0, wetL);
        wetR = postHpfR.processSample(1, wetR);
        wetR = postLpfR.processSample(1, wetR);

        // Dry / Wet ミックス (線形補間)
        float outL = dryLeft[i] + (wetL - dryLeft[i]) * mix;
        float outR = dryRight[i] + (wetR - dryRight[i]) * mix;

        // アウトプットゲイン
        outL *= outGainLin;
        outR *= outGainLin;

        // Safety Limiter
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
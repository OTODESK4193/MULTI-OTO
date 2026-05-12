#include "EngineCore.h"
#include "FastMath.h"
#include <cmath>
#include <algorithm>

EngineCore::EngineCore() {
    nodes.resize(NUM_NODES);
}

void EngineCore::prepare(double sampleRate, int samplesPerBlock) {
    juce::dsp::ProcessSpec spec{ sampleRate, static_cast<juce::uint32>(samplesPerBlock), 2 };
    crossover.prepare(spec);
    for (auto& node : nodes) node.prepare(sampleRate, samplesPerBlock);
    simdBuffer.resize(static_cast<size_t>(samplesPerBlock));
    limiterReleaseCoef = std::exp(-1.0f / (0.050f * static_cast<float>(sampleRate)));
    reset();
}

void EngineCore::reset() {
    crossover.reset();
    for (auto& node : nodes) node.reset();
    satL.reset(); satR.reset();
    limiterEnvL = 0.0f; limiterEnvR = 0.0f;
}

void EngineCore::updateParameters(const EngineParams& p) {
    currentParams = p;
    crossover.setFrequencies(p.xLow, p.xHigh);

    // 後ほどDynamicsNode側で受け取る準備をします
    // nodes[0].setParameters(p.s1_gain, p.s1_depth, p.s1_time, p.s1_atk, p.s1_rel);
    // nodes[1].setParameters(p.s2_gain, p.s2_depth, p.s2_time, p.s2_atk, p.s2_rel);

    currentLimitThreshold = FastMath::fast_exp2(p.limitCeil * 0.16609f);
}

void EngineCore::process(juce::AudioBuffer<float>& buffer) {
    const int numSamples = buffer.getNumSamples();
    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    float inGainLin = FastMath::fast_exp2(currentParams.inGain * 0.16609f);
    float outGainLin = FastMath::fast_exp2(currentParams.outGain * 0.16609f);

    for (int i = 0; i < numSamples; ++i) {
        float l = left[i] * inGainLin;
        float r = right[i] * inGainLin;

        if (currentParams.drive > 0.1f) {
            l = static_cast<float>(satL.processSample(l, currentParams.drive, currentParams.odd, currentParams.even));
            r = static_cast<float>(satR.processSample(r, currentParams.drive, currentParams.odd, currentParams.even));
        }

        float lL, lR, mL, mR, hL, hR;
        crossover.process(l, r, lL, lR, mL, mR, hL, hR);
        alignas(32) float raw[8] = { lL, lR, mL, mR, hL, hR, 0.0f, 0.0f };
        simdBuffer[static_cast<size_t>(i)] = juce::dsp::SIMDRegister<float>::fromRawArray(raw);
    }

    for (auto& node : nodes) {
        node.process(simdBuffer.data(), numSamples);
    }

    for (int i = 0; i < numSamples; ++i) {
        alignas(32) float raw[8];
        simdBuffer[static_cast<size_t>(i)].copyToRawArray(raw);

        float outL = raw[0] + raw[2] + raw[4];
        float outR = raw[1] + raw[3] + raw[5];

        outL *= outGainLin;
        outR *= outGainLin;

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
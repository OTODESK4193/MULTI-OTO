#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include "Crossover.h"
#include "DynamicsNode.h"
#include "Saturator.h"

struct EngineParams {
    float inGain, drive, odd, even;
    float xLow, xHigh;
    float s1_gain[3], s1_depth[3], s1_time, s1_atk[3], s1_rel[3];
    float s2_gain[3], s2_depth[3], s2_time, s2_atk[3], s2_rel[3];
    float post_hpf, post_lpf;
    float dryWet, outGain, limitCeil;
    int total_ott, phase_mode;
};

class EngineCore {
public:
    EngineCore();
    void prepare(double sampleRate, int samplesPerBlock);
    void updateParameters(const EngineParams& p);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    static constexpr int NUM_NODES = 2;
    EngineParams currentParams;

    Crossover crossover;
    Crossover dummyCrossover; // 位相補正用のダミー

    juce::dsp::StateVariableTPTFilter<float> postHpfL, postHpfR;
    juce::dsp::StateVariableTPTFilter<float> postLpfL, postLpfR;

    std::vector<DynamicsNode> nodes;
    std::vector<juce::dsp::SIMDRegister<float>> simdBuffer;
    juce::AudioBuffer<float> dryBuffer; // Dry音保持用

    ADAASaturator satL, satR;

    float limiterEnvL = 0.0f;
    float limiterEnvR = 0.0f;
    float limiterReleaseCoef = 0.0f;
    float currentLimitThreshold = 0.988f;

    static_assert(juce::dsp::SIMDRegister<float>::size() == 8,
        "CRITICAL: MULTI-OTO requires AVX2 instructions.");
};
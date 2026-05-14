#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include "Crossover.h"
#include "DynamicsNode.h"
#include "ADAASaturator.h"

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
    ~EngineCore() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void updateParameters(const EngineParams& p);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    static constexpr int NUM_NODES = 2;

    // パラメータ平滑化（ジッパーノイズ対策）
    juce::SmoothedValue<float> driveSmoother;
    juce::SmoothedValue<float> oddSmoother;
    juce::SmoothedValue<float> evenSmoother;
    juce::SmoothedValue<float> inGainSmoother;
    juce::SmoothedValue<float> outGainSmoother;

    EngineParams currentParams;

    Crossover crossover;
    Crossover dummyCrossover;

    // ZDF/TPT フィルター (レイテンシーなし・安定性保証)
    juce::dsp::StateVariableTPTFilter<float> postHpfL, postHpfR;
    juce::dsp::StateVariableTPTFilter<float> postLpfL, postLpfR;

    std::vector<DynamicsNode> nodes;
    std::vector<juce::dsp::SIMDRegister<float>> simdBuffer;
    juce::AudioBuffer<float> dryBuffer;

    ADAASaturator satL, satR;

    float limiterEnvL = 0.0f;
    float limiterEnvR = 0.0f;
    float limiterReleaseCoef = 0.0f;
    float currentLimitThreshold = 0.988f;
};
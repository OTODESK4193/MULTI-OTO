#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <atomic>
#include "Crossover.h"
#include "DynamicsNode.h"
#include "ADAASaturator.h"

struct EngineParams {
    float inGain, drive, odd, even, xLow, xHigh, s1_gain[3], s1_depth[3], s1_time, s1_mix, s1_atk[3], s1_rel[3], s2_gain[3], s2_depth[3], s2_time, s2_mix, s2_atk[3], s2_rel[3], post_hpf, post_lpf, dryWet, outGain, limitCeil;
    int total_ott_count, phase_mode;
    bool predrive_on, s1_on, s2_on;
};

class EngineCore {
public:
    EngineCore();
    void prepare(double sampleRate, int samplesPerBlock);
    void updateParameters(const EngineParams& p);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    static constexpr int MAX_NODES = 64;
    juce::SmoothedValue<float> driveSmoother, oddSmoother, evenSmoother, inGainSmoother, outGainSmoother;
    EngineParams currentParams;

    // 【重要】64個のOTTそれぞれに独立したCrossoverを持たせる
    std::vector<Crossover> crossovers;
    std::vector<Crossover> dryCrossovers; // Align Phase用
    std::vector<DynamicsNode> nodes;

    juce::dsp::StateVariableTPTFilter<float> preLpfL, preLpfR, postHpfL, postHpfR, postLpfL, postLpfR;
    juce::dsp::IIR::Filter<float> dcBlockL, dcBlockR; // DCオフセット発振キラー

    std::vector<juce::dsp::SIMDRegister<float>> simdBuffer;
    juce::AudioBuffer<float> dryBuffer;
    ADAASaturator satL, satR;
    float limiterEnvL = 0, limiterEnvR = 0, limiterReleaseCoef = 0, currentLimitThreshold = 0.988f;
    double currentSampleRate = 48000.0;
    std::atomic<bool> isPrepared{ false };
};
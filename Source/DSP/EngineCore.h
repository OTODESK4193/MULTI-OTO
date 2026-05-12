#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include "Crossover.h"
#include "DynamicsNode.h"

class EngineCore {
public:
    EngineCore();
    void prepare(double sampleRate, int samplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    static constexpr int NUM_NODES = 2; // 現在はプロトタイプ2段

    Crossover crossover;
    std::vector<DynamicsNode> nodes;
    std::vector<juce::dsp::SIMDRegister<float>> simdBuffer;

    // Safety Limiter用の状態変数
    float limiterEnvL = 0.0f;
    float limiterEnvR = 0.0f;
    float limiterReleaseCoef = 0.0f;
    const float LIMITER_THRESHOLD = 0.988f; // 約 -0.1 dBFS (絶対超えない壁)

    static_assert(juce::dsp::SIMDRegister<float>::size() == 8,
        "CRITICAL: MULTI-OTO requires AVX2 instructions. juce::dsp::SIMDRegister size must be 8.");
};
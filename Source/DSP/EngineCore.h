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
    static constexpr int NUM_NODES = 2; // プロトタイプは2段（最終目標64段）

    Crossover crossover;
    std::vector<DynamicsNode> nodes;

    // SIMD SoA (Structure of Arrays) バッファ
    std::vector<juce::dsp::SIMDRegister<float>> simdBuffer;

    // 【重要】CMakeの/arch:AVX2が効いており、SIMD幅が8(256bit)であることをコンパイル時に強制チェック
    static_assert(juce::dsp::SIMDRegister<float>::size() == 8,
        "CRITICAL: MULTI-OTO requires AVX2 instructions. juce::dsp::SIMDRegister size must be 8.");
};
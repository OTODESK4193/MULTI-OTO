#pragma once
#include <juce_dsp/juce_dsp.h>

class DynamicsNode {
public:
    using Vector = juce::dsp::SIMDRegister<float>;

    DynamicsNode();
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    void process(Vector* simmData, int numSamples);

private:
    Vector envelope{ 0.0f };
    double currentSampleRate = 44100.0;

    float attackCoef = 0.0f;
    float releaseCoef = 0.0f;

    // OTTスタイルのコンプレッション・パラメータ
    float upwardThresholdDb = -40.0f;
    float downwardThresholdDb = -15.0f;
    float upwardRatio = 0.5f;   // 0.5:1 (増幅)
    float downwardRatio = 4.0f; // 4:1 (圧縮)
    float upwardDepth = 1.0f;   // 100%
    float downwardDepth = 1.0f; // 100%
};
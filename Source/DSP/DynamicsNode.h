#pragma once
#include <juce_dsp/juce_dsp.h>

class DynamicsNode {
public:
    // SIMDRegisterはAVX2環境で8個のfloatを1クロックで処理します
    using Vector = juce::dsp::SIMDRegister<float>;

    DynamicsNode();
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    // SoA化されたSIMDバッファを直接書き換えるコア処理
    void process(Vector* simmData, int numSamples);

private:
    Vector envelope{ 0.0f };
    double currentSampleRate = 44100.0;

    Vector attackCoef{ 0.0f };
    Vector releaseCoef{ 0.0f };
};
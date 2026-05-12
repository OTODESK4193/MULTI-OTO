#pragma once
#include <juce_dsp/juce_dsp.h>

class DynamicsNode {
public:
    using Vector = juce::dsp::SIMDRegister<float>;

    DynamicsNode();
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    // GUIから受け取ったパラメータを設定・計算するメソッド
    void setParameters(const float* gains, const float* depths, float macroTime, const float* attacks, const float* releases);

    void process(Vector* simmData, int numSamples);

private:
    Vector envelope{ 0.0f };
    double currentSampleRate = 44100.0;

    // 帯域ごとの設定値ベクトル (L/R独立, 3バンド = 計6レーン)
    Vector attackCoef{ 0.0f };
    Vector releaseCoef{ 0.0f };
    Vector bandDepth{ 1.0f };
    Vector bandGain{ 1.0f };

    float upwardThresholdDb = -40.0f;
    float downwardThresholdDb = -15.0f;
    float upwardRatio = 0.5f;
    float downwardRatio = 4.0f;
};
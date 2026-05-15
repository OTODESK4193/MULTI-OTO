#pragma once
#include <juce_dsp/juce_dsp.h>

/**
 * Phase-Aligned 3-Band Crossover
 * 従来の Linkwitz-Riley に加え、All-Pass Filter を用いた位相補償を実装。
 * Low 帯域に X-High 周波数の APF を通すことで、全帯域の群遅延を一致させ、
 * 再合成時のフラットな特性（完全再構築）を実現します。
 */
class Crossover {
public:
    Crossover();
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void setFrequencies(float lowFreq, float highFreq);

    // Wet 信号の分割（内部で位相補償済み）
    void process(float inL, float inR,
        float& lowL, float& lowR,
        float& midL, float& midR,
        float& highL, float& highR);

    // Dry 信号用：Wet と同じ位相回転を与える
    void processDry(float inL, float inR, float& dryL, float& dryR);

private:
    // 分割用 Linkwitz-Riley (24dB/oct)
    juce::dsp::LinkwitzRileyFilter<float> lp1L, hp1L, lp2L, hp2L;
    juce::dsp::LinkwitzRileyFilter<float> lp1R, hp1R, lp2R, hp2R;

    // 位相補償用 All-Pass Filter (Low 帯域を Mid/High の遅延に合わせる)
    juce::dsp::IIR::Filter<float> ap1L, ap1R;

    // Dry 信号アライン用 (X-Low と X-High 両方の回転を与える)
    juce::dsp::IIR::Filter<float> dryAp1L, dryAp1R, dryAp2L, dryAp2R;

    float currentLowFreq = 88.0f;
    float currentHighFreq = 2500.0f;
    double currentSampleRate = 44100.0;
};
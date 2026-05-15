#pragma once
#include <juce_dsp/juce_dsp.h>

class Crossover {
public:
    Crossover();
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    void setFrequencies(float lowFreq, float highFreq);

    // 位相補償を行わず、純粋なLinkwitz-Rileyの位相回転（ディスパーション）を許容する
    void process(float inL, float inR,
        float& lowL, float& lowR,
        float& midL, float& midR,
        float& highL, float& highR);

    // Dry信号もそのまま通過させる（Alignを廃止しColorのみに）
    void processDry(float inL, float inR, float& dryL, float& dryR);

private:
    juce::dsp::LinkwitzRileyFilter<float> lp1L, hp1L, lp2L, hp2L;
    juce::dsp::LinkwitzRileyFilter<float> lp1R, hp1R, lp2R, hp2R;
    float currentLowFreq = 88.0f;
    float currentHighFreq = 2500.0f;
};
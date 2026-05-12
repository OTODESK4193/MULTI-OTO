#pragma once
#include <juce_dsp/juce_dsp.h>

class Crossover {
public:
    Crossover();
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void setFrequencies(float lowFreq, float highFreq);

    void process(float inL, float inR,
        float& lowL, float& lowR,
        float& midL, float& midR,
        float& highL, float& highR);
private:
    juce::dsp::LinkwitzRileyFilter<float> lp1L, hp1L, lp2L, hp2L;
    juce::dsp::LinkwitzRileyFilter<float> lp1R, hp1R, lp2R, hp2R;
};
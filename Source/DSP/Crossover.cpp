#include "Crossover.h"

Crossover::Crossover() {
    auto setType = [](juce::dsp::LinkwitzRileyFilter<float>& f, juce::dsp::LinkwitzRileyFilterType type) {
        f.setType(type);
        };

    setType(lp1L, juce::dsp::LinkwitzRileyFilterType::lowpass);
    setType(hp1L, juce::dsp::LinkwitzRileyFilterType::highpass);
    setType(lp1R, juce::dsp::LinkwitzRileyFilterType::lowpass);
    setType(hp1R, juce::dsp::LinkwitzRileyFilterType::highpass);

    setType(lp2L, juce::dsp::LinkwitzRileyFilterType::lowpass);
    setType(hp2L, juce::dsp::LinkwitzRileyFilterType::highpass);
    setType(lp2R, juce::dsp::LinkwitzRileyFilterType::lowpass);
    setType(hp2R, juce::dsp::LinkwitzRileyFilterType::highpass);

    setFrequencies(88.0f, 2500.0f);
}

void Crossover::prepare(const juce::dsp::ProcessSpec& spec) {
    lp1L.prepare(spec); hp1L.prepare(spec); lp2L.prepare(spec); hp2L.prepare(spec);
    lp1R.prepare(spec); hp1R.prepare(spec); lp2R.prepare(spec); hp2R.prepare(spec);
}

void Crossover::reset() {
    lp1L.reset(); hp1L.reset(); lp2L.reset(); hp2L.reset();
    lp1R.reset(); hp1R.reset(); lp2R.reset(); hp2R.reset();
}

void Crossover::setFrequencies(float lowFreq, float highFreq) {
    lp1L.setCutoffFrequency(lowFreq); hp1L.setCutoffFrequency(lowFreq);
    lp1R.setCutoffFrequency(lowFreq); hp1R.setCutoffFrequency(lowFreq);

    lp2L.setCutoffFrequency(highFreq); hp2L.setCutoffFrequency(highFreq);
    lp2R.setCutoffFrequency(highFreq); hp2R.setCutoffFrequency(highFreq);
}

void Crossover::process(float inL, float inR, float& lowL, float& lowR, float& midL, float& midR, float& highL, float& highR) {
    lowL = lp1L.processSample(0, inL);
    lowR = lp1R.processSample(1, inR);

    float remL = hp1L.processSample(0, inL);
    float remR = hp1R.processSample(1, inR);

    midL = lp2L.processSample(0, remL);
    midR = lp2R.processSample(1, remR);

    highL = hp2L.processSample(0, remL);
    highR = hp2R.processSample(1, remR);
}
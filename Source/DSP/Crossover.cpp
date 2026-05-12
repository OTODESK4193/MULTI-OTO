#include "Crossover.h"

Crossover::Crossover() {
    auto setType = [](juce::dsp::LinkwitzRileyFilter<float>& f, juce::dsp::LinkwitzRileyFilterType type, float freq) {
        f.setType(type);
        f.setCutoffFrequency(freq);
        };

    // 第1スプリット：88 Hz (Low と Mid/High の分離)
    setType(lp1L, juce::dsp::LinkwitzRileyFilterType::lowpass, 88.0f);
    setType(hp1L, juce::dsp::LinkwitzRileyFilterType::highpass, 88.0f);
    setType(lp1R, juce::dsp::LinkwitzRileyFilterType::lowpass, 88.0f);
    setType(hp1R, juce::dsp::LinkwitzRileyFilterType::highpass, 88.0f);

    // 第2スプリット：2500 Hz (Mid と High の分離)
    setType(lp2L, juce::dsp::LinkwitzRileyFilterType::lowpass, 2500.0f);
    setType(hp2L, juce::dsp::LinkwitzRileyFilterType::highpass, 2500.0f);
    setType(lp2R, juce::dsp::LinkwitzRileyFilterType::lowpass, 2500.0f);
    setType(hp2R, juce::dsp::LinkwitzRileyFilterType::highpass, 2500.0f);
}

void Crossover::prepare(const juce::dsp::ProcessSpec& spec) {
    lp1L.prepare(spec); hp1L.prepare(spec); lp2L.prepare(spec); hp2L.prepare(spec);
    lp1R.prepare(spec); hp1R.prepare(spec); lp2R.prepare(spec); hp2R.prepare(spec);
}

void Crossover::reset() {
    lp1L.reset(); hp1L.reset(); lp2L.reset(); hp2L.reset();
    lp1R.reset(); hp1R.reset(); lp2R.reset(); hp2R.reset();
}

void Crossover::process(float inL, float inR, float& lowL, float& lowR, float& midL, float& midR, float& highL, float& highR) {
    // 1段目: Low成分を抽出
    lowL = lp1L.processSample(0, inL);
    lowR = lp1R.processSample(1, inR);

    // 1段目: 残りの帯域（Mid + High）を抽出
    float remL = hp1L.processSample(0, inL);
    float remR = hp1R.processSample(1, inR);

    // 2段目: 残りからMidとHighを分離
    midL = lp2L.processSample(0, remL);
    midR = lp2R.processSample(1, remR);

    highL = hp2L.processSample(0, remL);
    highR = hp2R.processSample(1, remR);
}
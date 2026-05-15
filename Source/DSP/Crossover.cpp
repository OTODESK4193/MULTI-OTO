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
}

void Crossover::prepare(const juce::dsp::ProcessSpec& spec) {
    currentSampleRate = spec.sampleRate;
    lp1L.prepare(spec); hp1L.prepare(spec); lp2L.prepare(spec); hp2L.prepare(spec);
    lp1R.prepare(spec); hp1R.prepare(spec); lp2R.prepare(spec); hp2R.prepare(spec);

    ap1L.prepare(spec); ap1R.prepare(spec);
    dryAp1L.prepare(spec); dryAp1R.prepare(spec);
    dryAp2L.prepare(spec); dryAp2R.prepare(spec);

    setFrequencies(currentLowFreq, currentHighFreq);
}

void Crossover::reset() {
    lp1L.reset(); hp1L.reset(); lp2L.reset(); hp2L.reset();
    lp1R.reset(); hp1R.reset(); lp2R.reset(); hp2R.reset();
    ap1L.reset(); ap1R.reset();
    dryAp1L.reset(); dryAp1R.reset();
    dryAp2L.reset(); dryAp2R.reset();
}

void Crossover::setFrequencies(float lowFreq, float highFreq) {
    currentLowFreq = lowFreq;
    currentHighFreq = highFreq;

    lp1L.setCutoffFrequency(lowFreq); hp1L.setCutoffFrequency(lowFreq);
    lp1R.setCutoffFrequency(lowFreq); hp1R.setCutoffFrequency(lowFreq);

    lp2L.setCutoffFrequency(highFreq); hp2L.setCutoffFrequency(highFreq);
    lp2R.setCutoffFrequency(highFreq); hp2R.setCutoffFrequency(highFreq);

    // 位相補償用オールパス係数計算 (2次のオールパスをLinkwitz-Rileyの位相特性に合わせる)
    auto apCoeffs = juce::dsp::IIR::Coefficients<float>::makeAllPass(currentSampleRate, highFreq);
    *ap1L.coefficients = *apCoeffs;
    *ap1R.coefficients = *apCoeffs;
    *dryAp2L.coefficients = *apCoeffs;
    *dryAp2R.coefficients = *apCoeffs;

    auto apCoeffsLow = juce::dsp::IIR::Coefficients<float>::makeAllPass(currentSampleRate, lowFreq);
    *dryAp1L.coefficients = *apCoeffsLow;
    *dryAp1R.coefficients = *apCoeffsLow;
}

void Crossover::process(float inL, float inR, float& lowL, float& lowR, float& midL, float& midR, float& highL, float& highR) {
    // 1段目: Low vs (Mid + High)
    float rawLowL = lp1L.processSample(0, inL);
    float rawLowR = lp1R.processSample(1, inR);
    float remL = hp1L.processSample(0, inL);
    float remR = hp1R.processSample(1, inR);

    // 【重要】Low 帯域に 2段目の分割周波数(X-High)の回転を与える
    lowL = ap1L.processSample(rawLowL);
    lowR = ap1R.processSample(rawLowR);

    // 2段目: Mid vs High
    midL = lp2L.processSample(0, remL);
    midR = lp2R.processSample(1, remR);
    highL = hp2L.processSample(0, remL);
    highR = hp2R.processSample(1, remR);
}

void Crossover::processDry(float inL, float inR, float& dryL, float& dryR) {
    // Dry 信号に X-Low と X-High の両方の位相回転を与え、Wet と完全に一致させる
    dryL = dryAp2L.processSample(dryAp1L.processSample(inL));
    dryR = dryAp2R.processSample(dryAp1R.processSample(inR));
}
#include "MinimalUI.h"

MinimalUI::MinimalUI() {}
MinimalUI::~MinimalUI() = default;

void MinimalUI::paint(juce::Graphics& g) {
    g.setColour(juce::Colours::white);
    g.setFont(24.0f);

    // プロトタイプのタイトル表示
    g.drawText("MULTI-OTO: 64-Stage OTT Prototype", getLocalBounds().withBottom(100), juce::Justification::centred, true);

    g.setColour(juce::Colours::lightgrey);
    g.setFont(16.0f);
    g.drawText("[ SoA SIMD Processing Engine Active ]", getLocalBounds().withTop(50), juce::Justification::centred, true);
}

void MinimalUI::resized() {}
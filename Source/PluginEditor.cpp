#include "PluginProcessor.h"
#include "PluginEditor.h"

MultiOtoAudioProcessorEditor::MultiOtoAudioProcessorEditor(MultiOtoAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    addAndMakeVisible(minimalUI);
    setSize(800, 600); // UIの初期サイズ
}

MultiOtoAudioProcessorEditor::~MultiOtoAudioProcessorEditor() = default;

void MultiOtoAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff121212)); // ダークな背景
}

void MultiOtoAudioProcessorEditor::resized() {
    minimalUI.setBounds(getLocalBounds());
}
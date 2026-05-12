#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "GUI/MinimalUI.h"

class MultiOtoAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
    MultiOtoAudioProcessorEditor(MultiOtoAudioProcessor&);
    ~MultiOtoAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    MultiOtoAudioProcessor& audioProcessor;
    MinimalUI minimalUI;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiOtoAudioProcessorEditor)
};
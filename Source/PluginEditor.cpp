#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DSP/EngineCore.h"

// ============================================================================
//  ContentComponent
// ============================================================================
MultiOtoAudioProcessorEditor::ContentComponent::ContentComponent (
    MultiOtoAudioProcessor& proc, MultiOtoLookAndFeel& laf)
    : processor (proc),
      mainPanel (proc.apvts, laf)
{
    addAndMakeVisible (header);
    addAndMakeVisible (mainPanel);

    mainPanel.bindApvts (proc.apvts);
}

MultiOtoAudioProcessorEditor::ContentComponent::~ContentComponent()
{
}

void MultiOtoAudioProcessorEditor::ContentComponent::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (
        MOColors::bg, 0, 0,
        MOColors::bg.darker (0.15f), 0, (float) getHeight(), false));
    g.fillAll();
}

void MultiOtoAudioProcessorEditor::ContentComponent::resized()
{
    auto area = getLocalBounds();

    // ヘッダー (32px)
    header.setBounds (area.removeFromTop (32));

    // 単一メインパネル領域
    mainPanel.setBounds (area.reduced (4, 2));
}

void MultiOtoAudioProcessorEditor::ContentComponent::connectMeters()
{
    auto* engine = processor.getEngineCore();
    if (engine == nullptr) return;

    mainPanel.setMeters (&engine->s1Meter, &engine->s2Meter,
                         &engine->xoverLoAtomic, &engine->xoverHiAtomic);
}

// ============================================================================
//  PluginEditor
// ============================================================================
MultiOtoAudioProcessorEditor::MultiOtoAudioProcessorEditor (MultiOtoAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      content (p, laf)
{
    setOpaque (true);
    setLookAndFeel (&laf);

    constrainer.setFixedAspectRatio ((double) kBaseW / (double) kBaseH);
    constrainer.setMinimumSize (kBaseW / 2, kBaseH / 2);
    constrainer.setMaximumSize (kBaseW * 2, kBaseH * 2);
    setConstrainer (&constrainer);
    setResizable (true, true);

    addAndMakeVisible (content);

    content.connectMeters();

    setSize (kBaseW, kBaseH);
}

MultiOtoAudioProcessorEditor::~MultiOtoAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void MultiOtoAudioProcessorEditor::paint (juce::Graphics&)
{
}

void MultiOtoAudioProcessorEditor::resized()
{
    float scale = (float) getWidth() / (float) kBaseW;
    content.setTransform (juce::AffineTransform::scale (scale));
    content.setBounds (0, 0, kBaseW, kBaseH);
}
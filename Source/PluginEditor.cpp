#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DSP/EngineCore.h"

// ============================================================================
//  ContentComponent
// ============================================================================
MultiOtoAudioProcessorEditor::ContentComponent::ContentComponent (
    MultiOtoAudioProcessor& proc, MultiOtoLookAndFeel& laf)
    : processor (proc),
      mainPanel   (proc.apvts, laf),
      stage1Panel (proc.apvts, 1, laf),
      stage2Panel (proc.apvts, 2, laf)
{
    addAndMakeVisible (header);
    addAndMakeVisible (mainPanel);
    addAndMakeVisible (stage1Panel);
    addAndMakeVisible (stage2Panel);

    // APVTS パラメータバインド (DynVisual からのドラッグ操作用)
    mainPanel.bindApvts (proc.apvts);

    header.onTabChanged = [this] (TabHeader::Tab t) { setActiveTab (t); };

    setActiveTab (TabHeader::Main);
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

    // ヘッダー (40px)
    header.setBounds (area.removeFromTop (40));

    // 各タブのパネル領域
    auto panelArea = area.reduced (6, 4);
    mainPanel.setBounds   (panelArea);
    stage1Panel.setBounds (panelArea);
    stage2Panel.setBounds (panelArea);
}

void MultiOtoAudioProcessorEditor::ContentComponent::setActiveTab (TabHeader::Tab t)
{
    mainPanel.setVisible   (t == TabHeader::Main);
    stage1Panel.setVisible (t == TabHeader::Stage1);
    stage2Panel.setVisible (t == TabHeader::Stage2);
}

void MultiOtoAudioProcessorEditor::ContentComponent::connectMeters()
{
    auto* engine = processor.getEngineCore();
    if (engine == nullptr) return;

    mainPanel.setMeters (&engine->s1Meter, &engine->s2Meter,
                         &engine->xoverLoAtomic, &engine->xoverHiAtomic);
    stage1Panel.setMeter (&engine->s1Meter, &engine->xoverLoAtomic, &engine->xoverHiAtomic);
    stage2Panel.setMeter (&engine->s2Meter, &engine->xoverLoAtomic, &engine->xoverHiAtomic);
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
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ColorPalette.h"
#include "MinimalUI.h"
#include "DynVisual.h"

struct StageMeter;

// ============================================================================
//  MasterPanel — MASTER タブ
//  全タブ共通の上部 DynVisual 描画エリア (高さ 200px)
//  下部: Post HPF/LPF, Phase Mode, DRY/WET, OUT, CEIL
// ============================================================================
class MasterPanel : public juce::Component
{
public:
    MasterPanel (juce::AudioProcessorValueTreeState& apvts, MultiOtoLookAndFeel& laf)
    {
        postHPF.build   (apvts, "post_hpf",   "HPF",     this, laf);
        postLPF.build   (apvts, "post_lpf",   "LPF",     this, laf);
        dryWet.build    (apvts, "dry_wet",     "DRY/WET", this, laf);
        outGain.build   (apvts, "out_gain",    "OUT GAIN",this, laf);
        limitCeil.build (apvts, "limit_ceil",  "CEILING", this, laf);

        phaseModeBox.addItemList ({ "COLOR PHASE", "ALIGN PHASE" }, 1);
        phaseModeBox.setLookAndFeel (&laf);
        phaseModeBox.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (phaseModeBox);
        phaseModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            apvts, "phase_mode", phaseModeBox);

        dynS1.setTitle ("STAGE 1 / 3-BAND OTT");
        dynS2.setTitle ("STAGE 2 / 3-BAND OTT");
        addAndMakeVisible (dynS1);
        addAndMakeVisible (dynS2);

        filterLabel.setText ("POST FILTER & PHASE MODE", juce::dontSendNotification);
        outputLabel.setText ("MASTER OUTPUT & LIMITER", juce::dontSendNotification);
        for (auto* lbl : { &filterLabel, &outputLabel })
        {
            lbl->setColour (juce::Label::textColourId, MOColors::textDim);
            lbl->setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
            addAndMakeVisible (lbl);
        }
    }

    ~MasterPanel() override
    {
        phaseModeBox.setLookAndFeel (nullptr);
    }

    void setMeters (const StageMeter* s1, const StageMeter* s2,
                    std::atomic<float>* xLo, std::atomic<float>* xHi)
    {
        dynS1.setMeter (s1);
        dynS2.setMeter (s2);
        dynS1.setCrossoverFreqs (xLo, xHi);
        dynS2.setCrossoverFreqs (xLo, xHi);
    }

    void paint (juce::Graphics& g) override
    {
        MOColors::paintPanel (g, getLocalBounds());
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (12, 10);

        // 1. 全タブ共通 上部 DynVisual 描画エリア (高さ 200px)
        auto dynArea = area.removeFromTop (200);
        int halfW = (dynArea.getWidth() - 12) / 2;
        dynS1.setBounds (dynArea.removeFromLeft (halfW));
        dynArea.removeFromLeft (12);
        dynS2.setBounds (dynArea.removeFromLeft (halfW));

        area.removeFromTop (12);

        // 2. 下部 コントロールエリア
        int kS   = 85;   // ノブサイズ 85px
        int gapX = 24;

        // セクション1: POST FILTER & PHASE MODE
        filterLabel.setBounds (area.removeFromTop (20));
        area.removeFromTop (8);

        auto filterRow = area.removeFromTop (kS);
        postHPF.setBounds (filterRow.removeFromLeft (kS)); filterRow.removeFromLeft (gapX);
        postLPF.setBounds (filterRow.removeFromLeft (kS)); filterRow.removeFromLeft (gapX + 10);
        phaseModeBox.setBounds (filterRow.removeFromLeft (140).withSizeKeepingCentre (135, 28));

        area.removeFromTop (15);

        // セクション2: MASTER OUTPUT & LIMITER
        outputLabel.setBounds (area.removeFromTop (20));
        area.removeFromTop (8);

        auto outRow = area.removeFromTop (kS);
        dryWet.setBounds    (outRow.removeFromLeft (kS)); outRow.removeFromLeft (gapX);
        outGain.setBounds   (outRow.removeFromLeft (kS)); outRow.removeFromLeft (gapX);
        limitCeil.setBounds (outRow.removeFromLeft (kS));
    }

private:
    ArcKnob postHPF, postLPF, dryWet, outGain, limitCeil;

    juce::ComboBox phaseModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> phaseModeAttachment;

    DynVisualComponent dynS1, dynS2;
    juce::Label filterLabel, outputLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterPanel)
};

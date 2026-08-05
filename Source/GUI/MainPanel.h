#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ColorPalette.h"
#include "MinimalUI.h"
#include "DynVisual.h"

struct StageMeter;

// ============================================================================
//  MainPanel — MAIN タブ
//  上部: DynVisual (Stage1 & Stage2 横並び)
//  中段: PRE-DRIVE & CROSSOVER CONTROLS
//  下段: MASTER OUTPUT & FILTER CONTROLS (旧Masterタブから移植)
// ============================================================================
class MainPanel : public juce::Component
{
public:
    MainPanel (juce::AudioProcessorValueTreeState& apvts, MultiOtoLookAndFeel& laf)
    {
        auto setupBtn = [&] (juce::TextButton& b, const char* pID,
                             std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& a) {
            b.setClickingTogglesState (true);
            b.setColour (juce::TextButton::buttonColourId,    MOColors::knobTrack);
            b.setColour (juce::TextButton::buttonOnColourId,  MOColors::accent);
            b.setColour (juce::TextButton::textColourOffId,   MOColors::textDim);
            b.setColour (juce::TextButton::textColourOnId,    MOColors::bg);
            addAndMakeVisible (b);
            a = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, pID, b);
        };

        setupBtn (preDriveBtn, "predrive_on", preDriveAt);

        totalOttBox.addItemList ({ "2","4","8","16","32","64","128" }, 1);
        totalOttBox.setLookAndFeel (&laf);
        addAndMakeVisible (totalOttBox);
        totalOttAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, "total_ott", totalOttBox);

        totalOttLabel.setText ("COUNT", juce::dontSendNotification);
        totalOttLabel.setColour (juce::Label::textColourId, MOColors::textDim);
        totalOttLabel.setJustificationType (juce::Justification::centred);
        totalOttLabel.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        addAndMakeVisible (totalOttLabel);

        // PRE-DRIVE ノブ
        inGain.build    (apvts, "in_gain",    "IN GAIN",  this, laf);
        drive.build     (apvts, "drive",      "DRIVE",    this, laf);
        oddBlend.build  (apvts, "odd_blend",  "ODD",      this, laf);
        evenBlend.build (apvts, "even_blend", "EVEN",     this, laf);
        xLow.build      (apvts, "xover_low",  "LOW X",    this, laf);
        xHigh.build     (apvts, "xover_high", "HIGH X",   this, laf);

        // MASTER ノブ (移植)
        postHPF.build   (apvts, "post_hpf",   "HPF",      this, laf);
        postLPF.build   (apvts, "post_lpf",   "LPF",      this, laf);
        dryWet.build    (apvts, "dry_wet",     "DRY/WET",  this, laf);
        outGain.build   (apvts, "out_gain",    "OUT GAIN", this, laf);
        limitCeil.build (apvts, "limit_ceil",  "CEILING",  this, laf);

        phaseModeBox.addItemList ({ "COLOR PHASE", "ALIGN PHASE" }, 1);
        phaseModeBox.setLookAndFeel (&laf);
        phaseModeBox.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (phaseModeBox);
        phaseModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            apvts, "phase_mode", phaseModeBox);

        // DynVisuals (両ステージ)
        dynS1.setTitle ("STAGE 1 / 3-BAND OTT");
        dynS2.setTitle ("STAGE 2 / 3-BAND OTT");
        addAndMakeVisible (dynS1);
        addAndMakeVisible (dynS2);

        // セクションラベル
        preLabel.setText ("PRE-DRIVE & CROSSOVER", juce::dontSendNotification);
        masterLabel.setText ("MASTER CONTROLS", juce::dontSendNotification);
        for (auto* lbl : { &preLabel, &masterLabel })
        {
            lbl->setColour (juce::Label::textColourId, MOColors::textDim);
            lbl->setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
            addAndMakeVisible (lbl);
        }
    }

    ~MainPanel() override
    {
        totalOttBox.setLookAndFeel (nullptr);
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

    /** DynVisual からのドラッグ操作用 APVTS 接続 */
    void bindApvts (juce::AudioProcessorValueTreeState& apvts)
    {
        dynS1.bindStageParameters (apvts, 1);
        dynS2.bindStageParameters (apvts, 2);
    }

    void paint (juce::Graphics& g) override
    {
        MOColors::paintPanel (g, getLocalBounds());
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (12, 8);

        // 1. 上部 DynVisual 描画エリア (高さ 190px)
        auto dynArea = area.removeFromTop (190);
        int halfW = (dynArea.getWidth() - 12) / 2;
        dynS1.setBounds (dynArea.removeFromLeft (halfW));
        dynArea.removeFromLeft (12);
        dynS2.setBounds (dynArea.removeFromLeft (halfW));

        area.removeFromTop (8);

        int kS   = 78;   // ノブサイズ 78px
        int gapX = 18;

        // 2. 中段 PRE-DRIVE & CROSSOVER
        preLabel.setBounds (area.removeFromTop (16));
        area.removeFromTop (4);

        auto preRow = area.removeFromTop (kS);
        inGain.setBounds    (preRow.removeFromLeft (kS)); preRow.removeFromLeft (gapX);
        drive.setBounds     (preRow.removeFromLeft (kS)); preRow.removeFromLeft (gapX);
        oddBlend.setBounds  (preRow.removeFromLeft (kS)); preRow.removeFromLeft (gapX);
        evenBlend.setBounds (preRow.removeFromLeft (kS)); preRow.removeFromLeft (gapX + 15);

        auto onCell = preRow.removeFromLeft (kS);
        preDriveBtn.setBounds (onCell.withSizeKeepingCentre (72, 26));
        preRow.removeFromLeft (gapX);

        auto countCell = preRow.removeFromLeft (kS);
        totalOttBox.setBounds (countCell.withSizeKeepingCentre (72, 24).translated (0, -6));
        totalOttLabel.setBounds (countCell.withSizeKeepingCentre (72, 14).translated (0, 14));
        preRow.removeFromLeft (gapX + 15);

        xLow.setBounds  (preRow.removeFromLeft (kS)); preRow.removeFromLeft (gapX);
        xHigh.setBounds (preRow.removeFromLeft (kS));

        area.removeFromTop (10);

        // 3. 下段 MASTER CONTROLS (移植エリア)
        masterLabel.setBounds (area.removeFromTop (16));
        area.removeFromTop (4);

        auto mRow = area.removeFromTop (kS);
        postHPF.setBounds   (mRow.removeFromLeft (kS)); mRow.removeFromLeft (gapX);
        postLPF.setBounds   (mRow.removeFromLeft (kS)); mRow.removeFromLeft (gapX + 10);

        phaseModeBox.setBounds (mRow.removeFromLeft (130).withSizeKeepingCentre (125, 26));
        mRow.removeFromLeft (gapX + 15);

        dryWet.setBounds    (mRow.removeFromLeft (kS)); mRow.removeFromLeft (gapX);
        outGain.setBounds   (mRow.removeFromLeft (kS)); mRow.removeFromLeft (gapX);
        limitCeil.setBounds (mRow.removeFromLeft (kS));
    }

private:
    juce::TextButton preDriveBtn { "PRE-DRIVE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> preDriveAt;

    juce::ComboBox totalOttBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> totalOttAttachment;
    juce::Label totalOttLabel;

    ArcKnob inGain, drive, oddBlend, evenBlend;
    ArcKnob xLow, xHigh;

    // Master 移植ノブ
    ArcKnob postHPF, postLPF, dryWet, outGain, limitCeil;
    juce::ComboBox phaseModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> phaseModeAttachment;

    DynVisualComponent dynS1, dynS2;
    juce::Label preLabel, masterLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainPanel)
};

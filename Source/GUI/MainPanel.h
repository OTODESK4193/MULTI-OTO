#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ColorPalette.h"
#include "MinimalUI.h"
#include "DynVisual.h"

struct StageMeter;

// ============================================================================
//  MainPanel — MAIN タブ
//  全タブ共通の上部 DynVisual 描画エリア (高さ 200px)
//  下部: PRE-DRIVE & X-OVER パラメータ (大きめノブでバランス配置)
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

        inGain.build    (apvts, "in_gain",    "IN GAIN",  this, laf);
        drive.build     (apvts, "drive",      "DRIVE",    this, laf);
        oddBlend.build  (apvts, "odd_blend",  "ODD",      this, laf);
        evenBlend.build (apvts, "even_blend", "EVEN",     this, laf);
        xLow.build      (apvts, "xover_low",  "LOW X",    this, laf);
        xHigh.build     (apvts, "xover_high", "HIGH X",   this, laf);

        dynS1.setTitle ("STAGE 1 / 3-BAND OTT");
        dynS2.setTitle ("STAGE 2 / 3-BAND OTT");
        addAndMakeVisible (dynS1);
        addAndMakeVisible (dynS2);

        sectionLabel.setText ("PRE-DRIVE & CROSSOVER CONTROLS", juce::dontSendNotification);
        sectionLabel.setColour (juce::Label::textColourId, MOColors::textDim);
        sectionLabel.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        addAndMakeVisible (sectionLabel);
    }

    ~MainPanel() override
    {
        totalOttBox.setLookAndFeel (nullptr);
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

        // 1. 全タブ共通 上部 DynVisual 描画エリア (200px)
        auto dynArea = area.removeFromTop (200);
        int halfW = (dynArea.getWidth() - 12) / 2;
        dynS1.setBounds (dynArea.removeFromLeft (halfW));
        dynArea.removeFromLeft (12);
        dynS2.setBounds (dynArea.removeFromLeft (halfW));

        area.removeFromTop (12);

        // 2. 下部 コントロールエリア
        sectionLabel.setBounds (area.removeFromTop (20));
        area.removeFromTop (10);

        int kS   = 85;   // ノブの大きさを85pxに拡大
        int gapX = 24;

        // ノブ行1: IN GAIN, DRIVE, ODD, EVEN, LOW X, HIGH X (6個を均等配置)
        auto row1 = area.removeFromTop (kS);
        inGain.setBounds    (row1.removeFromLeft (kS)); row1.removeFromLeft (gapX);
        drive.setBounds     (row1.removeFromLeft (kS)); row1.removeFromLeft (gapX);
        oddBlend.setBounds  (row1.removeFromLeft (kS)); row1.removeFromLeft (gapX);
        evenBlend.setBounds (row1.removeFromLeft (kS)); row1.removeFromLeft (gapX + 20);

        xLow.setBounds  (row1.removeFromLeft (kS)); row1.removeFromLeft (gapX);
        xHigh.setBounds (row1.removeFromLeft (kS));

        area.removeFromTop (15);

        // スイッチ & カウント行
        auto row2 = area.removeFromTop (36);
        auto onCell = row2.removeFromLeft (kS);
        preDriveBtn.setBounds (onCell.withSizeKeepingCentre (70, 28));
        row2.removeFromLeft (gapX);

        auto countCell = row2.removeFromLeft (kS);
        totalOttBox.setBounds (countCell.withSizeKeepingCentre (75, 26).translated (0, -6));
        totalOttLabel.setBounds (countCell.withSizeKeepingCentre (75, 14).translated (0, 14));
    }

private:
    juce::TextButton preDriveBtn { "PRE-DRIVE ON" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> preDriveAt;

    juce::ComboBox totalOttBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> totalOttAttachment;
    juce::Label totalOttLabel;

    ArcKnob inGain, drive, oddBlend, evenBlend;
    ArcKnob xLow, xHigh;

    DynVisualComponent dynS1, dynS2;
    juce::Label sectionLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainPanel)
};

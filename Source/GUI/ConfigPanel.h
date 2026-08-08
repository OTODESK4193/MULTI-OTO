// ============================================================================
//  ConfigPanel.h
//  表示テーマとリミッター詳細設定のオーバーレイ
// ============================================================================
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ColorPalette.h"
#include "MinimalUI.h"

class ConfigPanel : public juce::Component
{
public:
    ConfigPanel (juce::AudioProcessorValueTreeState& apvts, MultiOtoLookAndFeel& laf)
    {
        // --- カラーテーマ ---
        themeBox.addItemList (MOColors::getThemeNames(), 1);
        themeBox.setLookAndFeel (&laf);
        themeBox.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (themeBox);
        themeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            apvts, "color_theme", themeBox);
        themeBox.onChange = [this] { if (onThemeChanged) onThemeChanged(); };

        // --- リミッター ---
        limitModeBox.addItemList ({ "LIMIT", "CLIP" }, 1);
        limitModeBox.setLookAndFeel (&laf);
        limitModeBox.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (limitModeBox);
        limitModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            apvts, "limit_mode", limitModeBox);

        ceiling.build (apvts, "limit_ceil",    "CEILING", this, laf, MOColors::lavender);
        release.build (apvts, "limit_release", "RELEASE", this, laf, MOColors::lavender);

        btnClose.setButtonText ("CLOSE");
        btnClose.setColour (juce::TextButton::buttonColourId,  MOColors::knobTrack);
        btnClose.setColour (juce::TextButton::textColourOffId, MOColors::text);
        btnClose.onClick = [this] { setVisible (false); };
        addAndMakeVisible (btnClose);
    }

    ~ConfigPanel() override
    {
        themeBox.setLookAndFeel (nullptr);
        limitModeBox.setLookAndFeel (nullptr);
    }

    std::function<void()> onThemeChanged;

    /** テーマ切替後に自分の中の色を貼り直す */
    void applyTheme()
    {
        ceiling.setAccent (MOColors::lavender);
        release.setAccent (MOColors::lavender);
        btnClose.setColour (juce::TextButton::buttonColourId,  MOColors::knobTrack);
        btnClose.setColour (juce::TextButton::textColourOffId, MOColors::text);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (MOColors::bg.withAlpha (0.94f));

        auto panel = getLocalBounds().reduced (16).toFloat();
        g.setColour (MOColors::panel);
        g.fillRoundedRectangle (panel, 8.0f);
        g.setColour (MOColors::accent.withAlpha (0.65f));
        g.drawRoundedRectangle (panel, 8.0f, 1.5f);

        g.setColour (MOColors::accent);
        g.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));
        g.drawText ("CONFIG", titleArea, juce::Justification::centredLeft);

        auto caption = [&] (const juce::String& t, juce::Rectangle<int> r, juce::Colour c) {
            g.setColour (c);
            g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
            g.drawText (t, r, juce::Justification::centredLeft);
        };

        caption ("APPEARANCE", sectionA, MOColors::textDim);
        caption ("LIMITER",    sectionB, MOColors::textDim);

        // 日本語は必ず MOText::u8() を通す (JUCE は const char* を Latin-1 とみなすため)
        g.setColour (MOColors::textDim);
        g.setFont (MOText::bodyFont (14.0f));
        g.drawFittedText (
            MOText::u8 ("COLOR THEME  -  GUI の配色とノブ・メーターの色を変更します。音には影響しません。"),
            descA, juce::Justification::topLeft, 2);

        g.drawFittedText (
            MOText::u8 ("LIMIT : ピークを追従して滑らかに抑えます。RELEASE で戻りの速さを調整します。\n"
                        "CLIP  : シーリングで即座に折り返します。倍音は増えますが密度が上がります。\n"
                        "どちらのモードでも、出力が CEILING を超えることはありません。"),
            descB, juce::Justification::topLeft, 4);

        g.setColour (MOColors::peach);
        g.setFont (MOText::bodyFont (14.0f, true));
        g.drawFittedText (
            MOText::u8 ("128 段のアップワード・コンプレッションは膨大なゲインを生みます。"
                        "CEILING は常に有効ですが、モニターの音量には十分注意してください。"),
            warnArea, juce::Justification::topLeft, 2);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (26);

        titleArea = area.removeFromTop (28);
        auto closeRow = titleArea;
        btnClose.setBounds (closeRow.removeFromRight (76).withSizeKeepingCentre (76, 26));

        area.removeFromTop (12);

        // --- APPEARANCE ---
        sectionA = area.removeFromTop (16);
        area.removeFromTop (6);
        auto rowA = area.removeFromTop (30);
        themeBox.setBounds (rowA.removeFromLeft (170));
        area.removeFromTop (8);
        descA = area.removeFromTop (24);

        area.removeFromTop (18);

        // --- LIMITER ---
        sectionB = area.removeFromTop (16);
        area.removeFromTop (6);

        auto rowB = area.removeFromTop (96);
        auto modeCell = rowB.removeFromLeft (170);
        limitModeBox.setBounds (modeCell.removeFromTop (30));
        rowB.removeFromLeft (24);
        ceiling.setBounds (rowB.removeFromLeft (96));
        rowB.removeFromLeft (14);
        release.setBounds (rowB.removeFromLeft (96));

        area.removeFromTop (10);
        descB = area.removeFromTop (78);

        area.removeFromTop (10);
        warnArea = area.removeFromTop (44);
    }

private:
    juce::ComboBox themeBox, limitModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> themeAttach, limitModeAttach;

    ArcKnob ceiling, release;
    juce::TextButton btnClose;

    juce::Rectangle<int> titleArea, sectionA, sectionB, descA, descB, warnArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConfigPanel)
};

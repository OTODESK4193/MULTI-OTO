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
        g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
        g.drawText ("CONFIG", titleArea, juce::Justification::centredLeft);

        auto caption = [&] (const juce::String& t, juce::Rectangle<int> r, juce::Colour c) {
            g.setColour (c);
            g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
            g.drawText (t, r, juce::Justification::centredLeft);
        };

        caption ("APPEARANCE", sectionA, MOColors::textDim);
        caption ("LIMITER",    sectionB, MOColors::textDim);

        g.setColour (MOColors::textDim);
        g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::plain)));
        g.drawFittedText (
            "COLOR THEME  -  GUI の配色のみを変更します。音には影響しません。",
            descA, juce::Justification::topLeft, 2);

        g.drawFittedText (
            "LIMIT : ピークを追従して滑らかに抑えます。RELEASE で戻りの速さを調整。\n"
            "CLIP  : シーリングで即座に折り返します。倍音は増えますが密度が上がります。\n"
            "どちらのモードでも出力が CEILING を超えることはありません。",
            descB, juce::Justification::topLeft, 4);

        g.setColour (MOColors::peach);
        g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        g.drawFittedText (
            "128 段のアップワード・コンプレッションは膨大なゲインを生みます。"
            "CEILING は常に有効ですが、モニターの音量には十分注意してください。",
            warnArea, juce::Justification::topLeft, 2);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (26);

        titleArea = area.removeFromTop (26);
        auto closeRow = titleArea;
        btnClose.setBounds (closeRow.removeFromRight (68).withSizeKeepingCentre (68, 24));

        area.removeFromTop (10);

        // --- APPEARANCE ---
        sectionA = area.removeFromTop (14);
        area.removeFromTop (4);
        auto rowA = area.removeFromTop (26);
        themeBox.setBounds (rowA.removeFromLeft (150));
        area.removeFromTop (4);
        descA = area.removeFromTop (18);

        area.removeFromTop (14);

        // --- LIMITER ---
        sectionB = area.removeFromTop (14);
        area.removeFromTop (4);

        auto rowB = area.removeFromTop (86);
        auto modeCell = rowB.removeFromLeft (150);
        limitModeBox.setBounds (modeCell.removeFromTop (26));
        rowB.removeFromLeft (20);
        ceiling.setBounds (rowB.removeFromLeft (86));
        rowB.removeFromLeft (12);
        release.setBounds (rowB.removeFromLeft (86));

        area.removeFromTop (6);
        descB = area.removeFromTop (60);

        area.removeFromTop (8);
        warnArea = area.removeFromTop (34);
    }

private:
    juce::ComboBox themeBox, limitModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> themeAttach, limitModeAttach;

    ArcKnob ceiling, release;
    juce::TextButton btnClose;

    juce::Rectangle<int> titleArea, sectionA, sectionB, descA, descB, warnArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConfigPanel)
};

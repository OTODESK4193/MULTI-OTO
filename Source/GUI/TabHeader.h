#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "ColorPalette.h"

#ifndef MULTIOTO_VERSION
 #define MULTIOTO_VERSION "1.1.0"
#endif

// ============================================================================
//  TabHeader — タイトルロゴ + PRESET ボタン (高さ 32px)
// ============================================================================
class TabHeader : public juce::Component
{
public:
    TabHeader()
    {
        presetBtn.setButtonText ("PRESET");
        presetBtn.setColour (juce::TextButton::buttonColourId,   MOColors::knobTrack);
        presetBtn.setColour (juce::TextButton::buttonOnColourId, MOColors::accent);
        presetBtn.setColour (juce::TextButton::textColourOffId,  MOColors::text);
        presetBtn.setColour (juce::TextButton::textColourOnId,   MOColors::bg);
        presetBtn.onClick = [this] { if (onPresetClicked) onPresetClicked(); };
        addAndMakeVisible (presetBtn);
    }

    std::function<void()> onPresetClicked;

    /** 現在のプリセット名をヘッダーに表示する */
    void setPresetName (const juce::String& n) { presetName = n; repaint(); }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();

        g.setColour (MOColors::bg.brighter (0.04f));
        g.fillRect (b);

        g.setColour (MOColors::panelLine.withAlpha (0.10f));
        g.drawHorizontalLine (getHeight() - 1, 0.0f, (float) getWidth());

        // ロゴ
        g.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));
        g.setColour (MOColors::accent);
        g.drawText ("MULTI-OTO", 18, 0, 130, getHeight(), juce::Justification::centredLeft);

        // サブタイトル + バージョン
        g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        g.setColour (MOColors::textDim);
        g.drawText (juce::String ("Multi-Stage OTT  V") + MULTIOTO_VERSION,
                    145, 0, 220, getHeight(), juce::Justification::centredLeft);

        // 現在のプリセット名 (PRESET ボタンの左)
        if (presetName.isNotEmpty())
        {
            g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
            g.setColour (MOColors::babyBlue);
            g.drawText (presetName, presetBtn.getX() - 210, 0, 202, getHeight(),
                        juce::Justification::centredRight, true);
        }

        // アクセントバー
        auto accentBar = juce::Rectangle<float> (3, 5, 3, (float) getHeight() - 10);
        g.setGradientFill (juce::ColourGradient (
            MOColors::accent, accentBar.getX(), accentBar.getY(),
            MOColors::peach,  accentBar.getX(), accentBar.getBottom(), false));
        g.fillRoundedRectangle (accentBar, 1.5f);
    }

    void resized() override
    {
        presetBtn.setBounds (getLocalBounds().removeFromRight (86).withSizeKeepingCentre (78, 22));
    }

private:
    juce::TextButton presetBtn;
    juce::String presetName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TabHeader)
};

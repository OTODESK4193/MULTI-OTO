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
        configBtn.setButtonText ("CONFIG");
        modBtn.setButtonText ("MOD");
        presetBtn.onClick = [this] { if (onPresetClicked) onPresetClicked(); };
        configBtn.onClick = [this] { if (onConfigClicked) onConfigClicked(); };
        modBtn.onClick    = [this] { if (onModClicked)    onModClicked(); };
        addAndMakeVisible (presetBtn);
        addAndMakeVisible (configBtn);
        addAndMakeVisible (modBtn);
        applyTheme();
    }

    std::function<void()> onPresetClicked;
    std::function<void()> onConfigClicked;
    std::function<void()> onModClicked;

    /** カラーテーマ切替時に色を貼り直す */
    void applyTheme()
    {
        for (auto* b : { &presetBtn, &configBtn, &modBtn })
        {
            b->setColour (juce::TextButton::buttonColourId,   MOColors::knobTrack);
            b->setColour (juce::TextButton::buttonOnColourId, MOColors::accent);
            b->setColour (juce::TextButton::textColourOffId,  MOColors::text);
            b->setColour (juce::TextButton::textColourOnId,   MOColors::bg);
        }
        repaint();
    }

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
        g.setFont (juce::Font (juce::FontOptions (17.5f, juce::Font::bold)));
        g.setColour (MOColors::accent);
        g.drawText ("MULTI-OTO", 18, 0, 140, getHeight(), juce::Justification::centredLeft);

        // サブタイトル + バージョン
        g.setFont (juce::Font (juce::FontOptions (12.5f, juce::Font::bold)));
        g.setColour (MOColors::textDim);
        g.drawText (juce::String ("Multi-Stage OTT  V") + MULTIOTO_VERSION,
                    160, 0, 230, getHeight(), juce::Justification::centredLeft);

        // 現在のプリセット名 (CONFIG ボタンの左)
        if (presetName.isNotEmpty())
        {
            g.setFont (juce::Font (juce::FontOptions (12.5f, juce::Font::bold)));
            g.setColour (MOColors::babyBlue);
            g.drawText (presetName, modBtn.getX() - 212, 0, 204, getHeight(),
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
        auto r = getLocalBounds();
        presetBtn.setBounds (r.removeFromRight (94).withSizeKeepingCentre (86, 24));
        configBtn.setBounds (r.removeFromRight (90).withSizeKeepingCentre (82, 24));
        modBtn.setBounds    (r.removeFromRight (70).withSizeKeepingCentre (62, 24));
    }

private:
    juce::TextButton presetBtn, configBtn, modBtn;
    juce::String presetName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TabHeader)
};

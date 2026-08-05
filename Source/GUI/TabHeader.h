#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "ColorPalette.h"

// ============================================================================
//  TabHeader — シンプルなタイトルロゴヘッダー (1画面化対応)
//  高さ 36px
// ============================================================================
class TabHeader : public juce::Component
{
public:
    TabHeader() {}

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();

        // ヘッダー背景
        g.setColour (MOColors::bg.brighter (0.04f));
        g.fillRect (b);

        // 下端ライン
        g.setColour (MOColors::panelLine.withAlpha (0.10f));
        g.drawHorizontalLine (getHeight() - 1, 0.0f, (float) getWidth());

        // ロゴ
        g.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));
        g.setColour (MOColors::accent);
        g.drawText ("MULTI-OTO", 18, 0, 140, getHeight(), juce::Justification::centredLeft);

        // サブタイトル
        g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        g.setColour (MOColors::textDim);
        g.drawText ("Multi-Stage OTT Re-synthesis Engine", 145, 0, 300, getHeight(), juce::Justification::centredLeft);

        // アクセントバー (左端 3px 縦グラデーション)
        auto accentBar = juce::Rectangle<float> (3, 5, 3, (float) getHeight() - 10);
        g.setGradientFill (juce::ColourGradient (
            MOColors::accent, accentBar.getX(), accentBar.getY(),
            MOColors::peach,  accentBar.getX(), accentBar.getBottom(), false));
        g.fillRoundedRectangle (accentBar, 1.5f);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TabHeader)
};

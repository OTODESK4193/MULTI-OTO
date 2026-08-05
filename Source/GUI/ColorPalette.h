#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// ============================================================================
//  MULTI-OTO Color Palette
//  Wavetable (OTODESK) 準拠: ダーク紫系背景 × パステルアクセント
// ============================================================================
namespace MOColors
{
    // --- 背景系 ---
    inline const juce::Colour bg        { 0xFF17141f };
    inline const juce::Colour panel     { 0xFF201c2b };
    inline const juce::Colour panelLine { 0xFFe9e3f2 };   // 使用時 withAlpha(0.13f)
    inline const juce::Colour grid      { 0xFFe9e3f2 };   // 使用時 withAlpha(0.08f)
    inline const juce::Colour knobTrack { 0xFF2a2536 };
    inline const juce::Colour well      { 0xFF151220 };   // DynVisual 背景用

    // --- テキスト ---
    inline const juce::Colour text      { 0xFFe9e3f2 };
    inline const juce::Colour textDim   { 0xFF8d86a0 };

    // --- アクセント (機能別) ---
    inline const juce::Colour accent    { 0xFFFF6B00 };   // オレンジ (ヘッダー/ロゴ)
    inline const juce::Colour mint      { 0xFFb5ead7 };   // Upward OTT / OSC
    inline const juce::Colour pink      { 0xFFffb7c5 };   // Downward OTT / Master
    inline const juce::Colour babyBlue  { 0xFFaed9f7 };   // FX / OTT アクセント
    inline const juce::Colour peach     { 0xFFffdac1 };   // Stage
    inline const juce::Colour lavender  { 0xFFc7ceea };   // Master
    inline const juce::Colour lilac     { 0xFFe0c3fc };   // Modulation

    // --- パネル描画ヘルパー ---
    inline void paintPanel (juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        auto b = bounds.toFloat();
        // 微小な縦グラデーション
        g.setGradientFill (juce::ColourGradient (
            panel.brighter (0.03f), b.getX(), b.getY(),
            panel,                  b.getX(), b.getBottom(), false));
        g.fillRoundedRectangle (b, 6.0f);

        // 上端 1px ハイライト
        g.setColour (panelLine.withAlpha (0.10f));
        g.drawHorizontalLine ((int) b.getY(), b.getX() + 6, b.getRight() - 6);

        // 枠線
        g.setColour (panelLine.withAlpha (0.08f));
        g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, 1.0f);
    }

    inline void paintWell (juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        auto b = bounds.toFloat();
        g.setColour (well);
        g.fillRoundedRectangle (b, 4.0f);
        g.setColour (panelLine.withAlpha (0.06f));
        g.drawRoundedRectangle (b.reduced (0.5f), 4.0f, 1.0f);
    }
}

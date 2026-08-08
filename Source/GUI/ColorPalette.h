#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// ============================================================================
//  MULTI-OTO Color Palette
//  6 テーマ切替対応。setTheme() で背景系とアクセントを差し替える。
//  バンド固有カラー (LOW/MID/HIGH) は意味を持つため Mono 以外では不変。
// ============================================================================
namespace MOColors
{
    // --- 背景系 (テーマで変化) ---
    inline juce::Colour bg        { 0xFF17141f };
    inline juce::Colour panel     { 0xFF201c2b };
    inline juce::Colour panelLine { 0xFFe9e3f2 };   // withAlpha(0.13f)
    inline juce::Colour grid      { 0xFFe9e3f2 };   // withAlpha(0.08f)
    inline juce::Colour knobTrack { 0xFF2a2536 };
    inline juce::Colour well      { 0xFF151220 };   // DynVisual 背景用

    // --- テキスト ---
    inline juce::Colour text      { 0xFFe9e3f2 };
    inline juce::Colour textDim   { 0xFF8d86a0 };

    // --- アクセント (機能別) ---
    inline juce::Colour accent    { 0xFFFF6B00 };   // ヘッダー/ロゴ/強調
    inline juce::Colour mint      { 0xFFb5ead7 };   // Upward OTT
    inline juce::Colour pink      { 0xFFffb7c5 };   // Downward OTT
    inline juce::Colour babyBlue  { 0xFFaed9f7 };   // Stage 2 / クロスオーバー
    inline juce::Colour peach     { 0xFFffdac1 };   // Stage 1
    inline juce::Colour lavender  { 0xFFc7ceea };   // Master

    // --- バンド別固有カラー (LOW, MID, HIGH) ---
    inline juce::Colour bandLowUp    { 0xFF64d2ff };  // LOW Upward (シアン)
    inline juce::Colour bandLowDn    { 0xFF3b82f6 };  // LOW Downward (ブルー)
    inline juce::Colour bandMidUp    { 0xFFfde047 };  // MID Upward (イエロー)
    inline juce::Colour bandMidDn    { 0xFFf59e0b };  // MID Downward (アンバー)
    inline juce::Colour bandHighUp   { 0xFFf472b6 };  // HIGH Upward (マゼンタ)
    inline juce::Colour bandHighDn   { 0xFFa855f7 };  // HIGH Downward (パープル)

    inline int currentTheme = 0;

    inline juce::StringArray getThemeNames()
    {
        return { "Midnight", "Sakura", "Ocean", "Forest", "Sunset", "Mono" };
    }

    /** テーマ切替。呼び出し後は GUI 全体の repaint と
        MultiOtoLookAndFeel::refreshColours() が必要。 */
    inline void setTheme (int idx) noexcept
    {
        currentTheme = juce::jlimit (0, 5, idx);

        // 既定 (Midnight) をいったん復元してから差分を当てる
        text      = juce::Colour (0xFFe9e3f2);
        textDim   = juce::Colour (0xFF8d86a0);
        panelLine = juce::Colour (0xFFe9e3f2);
        grid      = juce::Colour (0xFFe9e3f2);
        mint      = juce::Colour (0xFFb5ead7);
        pink      = juce::Colour (0xFFffb7c5);
        babyBlue  = juce::Colour (0xFFaed9f7);
        peach     = juce::Colour (0xFFffdac1);
        lavender  = juce::Colour (0xFFc7ceea);

        bandLowUp  = juce::Colour (0xFF64d2ff);  bandLowDn  = juce::Colour (0xFF3b82f6);
        bandMidUp  = juce::Colour (0xFFfde047);  bandMidDn  = juce::Colour (0xFFf59e0b);
        bandHighUp = juce::Colour (0xFFf472b6);  bandHighDn = juce::Colour (0xFFa855f7);

        switch (currentTheme)
        {
        case 1: // Sakura
            bg = juce::Colour (0xFF1C1417); panel = juce::Colour (0xFF2A1E24);
            well = juce::Colour (0xFF190F14); knobTrack = juce::Colour (0xFF342630);
            accent = juce::Colour (0xFFFF6E9C);
            panelLine = grid = juce::Colour (0xFFF6E3EC);
            text = juce::Colour (0xFFF6E3EC); textDim = juce::Colour (0xFFA08993);
            break;

        case 2: // Ocean
            bg = juce::Colour (0xFF101A20); panel = juce::Colour (0xFF182730);
            well = juce::Colour (0xFF0C161C); knobTrack = juce::Colour (0xFF1F323D);
            accent = juce::Colour (0xFF32C8FF);
            panelLine = grid = juce::Colour (0xFFDDEEF6);
            text = juce::Colour (0xFFDDEEF6); textDim = juce::Colour (0xFF7E96A3);
            break;

        case 3: // Forest
            bg = juce::Colour (0xFF121C16); panel = juce::Colour (0xFF1A2A20);
            well = juce::Colour (0xFF0E1812); knobTrack = juce::Colour (0xFF22362A);
            accent = juce::Colour (0xFF6BE39A);
            panelLine = grid = juce::Colour (0xFFE0F2E6);
            text = juce::Colour (0xFFE0F2E6); textDim = juce::Colour (0xFF83A08E);
            break;

        case 4: // Sunset
            bg = juce::Colour (0xFF201412); panel = juce::Colour (0xFF301D1A);
            well = juce::Colour (0xFF1A100E); knobTrack = juce::Colour (0xFF3A2622);
            accent = juce::Colour (0xFFFF8A3D);
            panelLine = grid = juce::Colour (0xFFF7E6DD);
            text = juce::Colour (0xFFF7E6DD); textDim = juce::Colour (0xFFA88D82);
            break;

        case 5: // Mono — バンドカラーもグレースケールへ落とす
            bg = juce::Colour (0xFF121212); panel = juce::Colour (0xFF202020);
            well = juce::Colour (0xFF0E0E0E); knobTrack = juce::Colour (0xFF2A2A2A);
            accent = juce::Colour (0xFFD8D8D8);
            panelLine = grid = juce::Colour (0xFFEDEDED);
            text = juce::Colour (0xFFEDEDED); textDim = juce::Colour (0xFF8A8A8A);
            mint = pink = babyBlue = peach = lavender = juce::Colour (0xFFCACACA);
            bandLowUp  = juce::Colour (0xFFFFFFFF); bandLowDn  = juce::Colour (0xFF9A9A9A);
            bandMidUp  = juce::Colour (0xFFD5D5D5); bandMidDn  = juce::Colour (0xFF808080);
            bandHighUp = juce::Colour (0xFFB0B0B0); bandHighDn = juce::Colour (0xFF6A6A6A);
            break;

        default: // 0: Midnight
            bg = juce::Colour (0xFF17141f); panel = juce::Colour (0xFF201c2b);
            well = juce::Colour (0xFF151220); knobTrack = juce::Colour (0xFF2a2536);
            accent = juce::Colour (0xFFFF6B00);
            break;
        }
    }

    // --- パネル描画ヘルパー ---
    inline void paintPanel (juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        auto b = bounds.toFloat();
        g.setGradientFill (juce::ColourGradient (
            panel.brighter (0.03f), b.getX(), b.getY(),
            panel,                  b.getX(), b.getBottom(), false));
        g.fillRoundedRectangle (b, 6.0f);

        g.setColour (panelLine.withAlpha (0.10f));
        g.drawHorizontalLine ((int) b.getY(), b.getX() + 6, b.getRight() - 6);

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

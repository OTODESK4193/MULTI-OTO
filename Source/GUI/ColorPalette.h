#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// ============================================================================
//  MULTI-OTO Color Palette
//
//  10 テーマ切替対応。背景系・アクセント・ノブ色・メーターのバンド色まで
//  テーマごとに丸ごと差し替える。
//
//  【重要】juce::String(const char*) は入力を ASCII/Latin-1 として解釈するため、
//  UTF-8 の日本語リテラルをそのまま渡すと文字化けする。
//  非 ASCII を含む文字列は必ず MOText::u8() を通すこと。
// ============================================================================

namespace MOText
{
    /** UTF-8 のリテラルを正しく juce::String へ変換する */
    inline juce::String u8 (const char* utf8)
    {
        return juce::String (juce::CharPointer_UTF8 (utf8));
    }

    /** 日本語を含む説明文用のフォント。CJK を持つ書体を優先的に要求する。
        見つからない場合 JUCE が既定書体へフォールバックする。 */
    inline juce::Font bodyFont (float height, bool bold = false)
    {
       #if JUCE_WINDOWS
        return juce::Font (juce::FontOptions ("Yu Gothic UI", height,
                                              bold ? juce::Font::bold : juce::Font::plain));
       #else
        return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
       #endif
    }
}

namespace MOColors
{
    // --- 背景系 ---
    inline juce::Colour bg        { 0xFF17141f };
    inline juce::Colour panel     { 0xFF201c2b };
    inline juce::Colour panelLine { 0xFFe9e3f2 };   // withAlpha して使う
    inline juce::Colour grid      { 0xFFe9e3f2 };
    inline juce::Colour knobTrack { 0xFF2a2536 };
    inline juce::Colour well      { 0xFF151220 };

    // --- テキスト ---
    inline juce::Colour text      { 0xFFe9e3f2 };
    inline juce::Colour textDim   { 0xFF8d86a0 };

    // --- アクセント (機能別) ---
    inline juce::Colour accent    { 0xFFFF6B00 };   // ヘッダー / 強調 / PRE-DRIVE
    inline juce::Colour mint      { 0xFFb5ead7 };   // LINK
    inline juce::Colour pink      { 0xFFffb7c5 };
    inline juce::Colour babyBlue  { 0xFFaed9f7 };   // Stage 2 / クロスオーバー
    inline juce::Colour peach     { 0xFFffdac1 };   // Stage 1
    inline juce::Colour lavender  { 0xFFc7ceea };   // Master

    // --- バンド固有カラー (LOW / MID / HIGH の Upward / Downward) ---
    inline juce::Colour bandLowUp    { 0xFF64d2ff };
    inline juce::Colour bandLowDn    { 0xFF3b82f6 };
    inline juce::Colour bandMidUp    { 0xFFfde047 };
    inline juce::Colour bandMidDn    { 0xFFf59e0b };
    inline juce::Colour bandHighUp   { 0xFFf472b6 };
    inline juce::Colour bandHighDn   { 0xFFa855f7 };

    // ------------------------------------------------------------------
    //  テーマ定義テーブル
    // ------------------------------------------------------------------
    struct ThemeDef
    {
        const char* name;
        juce::uint32 bg, panel, well, knobTrack, line, text, textDim, accent;
        juce::uint32 mint, pink, babyBlue, peach, lavender;
        juce::uint32 lowUp, lowDn, midUp, midDn, highUp, highDn;
    };

    inline const ThemeDef& getThemes (int i)
    {
        static const ThemeDef t[] = {
        // name        bg        panel     well      knobTrk   line      text      textDim   accent
        //             mint      pink      blue      peach     lavender
        //             lowUp     lowDn     midUp     midDn     highUp    highDn
        { "Midnight", 0xFF17141F,0xFF201C2B,0xFF151220,0xFF2A2536,0xFFE9E3F2,0xFFE9E3F2,0xFF8D86A0,0xFFFF6B00,
                      0xFFB5EAD7,0xFFFFB7C5,0xFFAED9F7,0xFFFFDAC1,0xFFC7CEEA,
                      0xFF64D2FF,0xFF3B82F6,0xFFFDE047,0xFFF59E0B,0xFFF472B6,0xFFA855F7 },

        { "Sakura",   0xFF1C1417,0xFF2A1E24,0xFF190F14,0xFF342630,0xFFF6E3EC,0xFFF6E3EC,0xFFA08993,0xFFFF6E9C,
                      0xFFFFD3E2,0xFFFF9EBB,0xFFE3B7D9,0xFFFFC9B0,0xFFE7C0E8,
                      0xFFFF9EBB,0xFFD45C87,0xFFFFD9A0,0xFFE8A45C,0xFFE3A8F0,0xFF9B5FC7 },

        { "Ocean",    0xFF101A20,0xFF182730,0xFF0C161C,0xFF1F323D,0xFFDDEEF6,0xFFDDEEF6,0xFF7E96A3,0xFF32C8FF,
                      0xFF8CE8D2,0xFF8FC7F0,0xFF5FD6FF,0xFFA8DCE8,0xFF9FC0E8,
                      0xFF5FD6FF,0xFF2E8FC7,0xFF7BE8C4,0xFF2FA88A,0xFFB4C8FF,0xFF6479D6 },

        { "Forest",   0xFF121C16,0xFF1A2A20,0xFF0E1812,0xFF22362A,0xFFE0F2E6,0xFFE0F2E6,0xFF83A08E,0xFF6BE39A,
                      0xFF8FF0B4,0xFFD7E88C,0xFF8FD9C4,0xFFE0DE93,0xFFB4D9A8,
                      0xFF8FF0B4,0xFF3FA86A,0xFFE4E06B,0xFFA8A32E,0xFF7FD9D2,0xFF2F8F8A },

        { "Sunset",   0xFF201412,0xFF301D1A,0xFF1A100E,0xFF3A2622,0xFFF7E6DD,0xFFF7E6DD,0xFFA88D82,0xFFFF8A3D,
                      0xFFFFC79B,0xFFFF9E85,0xFFFFBE7A,0xFFFFD1A8,0xFFE8A08C,
                      0xFFFFB870,0xFFD4762C,0xFFFFD98A,0xFFE0A03C,0xFFFF9E85,0xFFC7513F },

        { "Mono",     0xFF121212,0xFF202020,0xFF0E0E0E,0xFF2A2A2A,0xFFEDEDED,0xFFEDEDED,0xFF8A8A8A,0xFFD8D8D8,
                      0xFFCACACA,0xFFCACACA,0xFFCACACA,0xFFCACACA,0xFFCACACA,
                      0xFFFFFFFF,0xFF9A9A9A,0xFFD5D5D5,0xFF808080,0xFFB0B0B0,0xFF6A6A6A },

        { "Neon",     0xFF0B0B12,0xFF15121F,0xFF07070D,0xFF231D33,0xFFE8E0FF,0xFFE8E0FF,0xFF8078A0,0xFFFF2D95,
                      0xFF3DFFC9,0xFFFF2D95,0xFF2DE1FF,0xFFFFB03D,0xFFB44DFF,
                      0xFF2DE1FF,0xFF0E86B8,0xFF3DFFC9,0xFF12B892,0xFFFF2D95,0xFFB01466 },

        { "Amber",    0xFF15120C,0xFF221D13,0xFF100E09,0xFF2E2718,0xFFF2E6C8,0xFFF2E6C8,0xFF9C8C68,0xFFFFB300,
                      0xFFD8CF8A,0xFFE8A86A,0xFFC9BE8E,0xFFFFCF7A,0xFFD4B98C,
                      0xFFFFCF7A,0xFFC08A20,0xFFFFB300,0xFFB37D00,0xFFE8A86A,0xFFA9663A },

        { "Ice",      0xFF0E1418,0xFF18222A,0xFF0A1014,0xFF22303A,0xFFE8F4FA,0xFFE8F4FA,0xFF8AA0AD,0xFF9FE8FF,
                      0xFFC4F0E8,0xFFCBD9F0,0xFF9FE8FF,0xFFD8E8F0,0xFFBFD0EA,
                      0xFF9FE8FF,0xFF5CA8CC,0xFFC4F0E8,0xFF6FB8A8,0xFFD8D0FF,0xFF8A82CC },

        { "Vapor",    0xFF141026,0xFF1F1838,0xFF0F0C1C,0xFF2C2350,0xFFF0E4FF,0xFFF0E4FF,0xFF9084B8,0xFFFF71CE,
                      0xFF05FFA1,0xFFFF71CE,0xFF01CDFE,0xFFFFD86E,0xFFB967FF,
                      0xFF01CDFE,0xFF0A82B8,0xFF05FFA1,0xFF06B873,0xFFFF71CE,0xFFB23C92 },
        };
        return t[juce::jlimit (0, (int) (sizeof (t) / sizeof (t[0])) - 1, i)];
    }

    inline int getNumThemes() { return 10; }

    inline juce::StringArray getThemeNames()
    {
        juce::StringArray names;
        for (int i = 0; i < getNumThemes(); ++i)
            names.add (getThemes (i).name);
        return names;
    }

    inline int currentTheme = 0;

    /** テーマ切替。呼び出し後に LookAndFeel::refreshColours() と
        各パネルの applyTheme() / repaint() が必要。 */
    inline void setTheme (int idx) noexcept
    {
        currentTheme = juce::jlimit (0, getNumThemes() - 1, idx);
        const auto& t = getThemes (currentTheme);

        bg        = juce::Colour (t.bg);
        panel     = juce::Colour (t.panel);
        well      = juce::Colour (t.well);
        knobTrack = juce::Colour (t.knobTrack);
        panelLine = juce::Colour (t.line);
        grid      = juce::Colour (t.line);
        text      = juce::Colour (t.text);
        textDim   = juce::Colour (t.textDim);
        accent    = juce::Colour (t.accent);

        mint      = juce::Colour (t.mint);
        pink      = juce::Colour (t.pink);
        babyBlue  = juce::Colour (t.babyBlue);
        peach     = juce::Colour (t.peach);
        lavender  = juce::Colour (t.lavender);

        bandLowUp  = juce::Colour (t.lowUp);   bandLowDn  = juce::Colour (t.lowDn);
        bandMidUp  = juce::Colour (t.midUp);   bandMidDn  = juce::Colour (t.midDn);
        bandHighUp = juce::Colour (t.highUp);  bandHighDn = juce::Colour (t.highDn);
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

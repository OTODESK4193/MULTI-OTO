#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "ColorPalette.h"

// ============================================================================
//  TabHeader — LIFT-X 準拠のタブナビゲーションヘッダー
//  高さ 44px、左端にロゴ、右にタブボタン群を並べる。
// ============================================================================
class TabHeader : public juce::Component
{
public:
    enum Tab { Main = 0, Stage1, Stage2, Master, NumTabs };

    TabHeader()
    {
        for (int i = 0; i < (int) NumTabs; ++i)
        {
            auto* btn = tabButtons.add (new juce::TextButton (tabNames[i]));
            btn->setClickingTogglesState (false);
            btn->setRadioGroupId (9001);
            btn->onClick = [this, i] { setActiveTab ((Tab) i); };
            addAndMakeVisible (btn);
        }
        setActiveTab (Main);
    }

    void setActiveTab (Tab t)
    {
        activeTab = t;
        for (int i = 0; i < tabButtons.size(); ++i)
            styleButton (tabButtons[i], i == (int) t);
        if (onTabChanged) onTabChanged (t);
        repaint();
    }

    Tab getActiveTab() const { return activeTab; }

    std::function<void (Tab)> onTabChanged;

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
        auto logoArea = getLocalBounds().removeFromLeft (120).toFloat();
        g.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));
        g.setColour (MOColors::accent);
        g.drawText ("MULTI-OTO", logoArea.reduced (12, 0), juce::Justification::centredLeft);

        // アクセントバー (左端 3px 縦グラデーション)
        auto accentBar = juce::Rectangle<float> (3, 6, 3, (float) getHeight() - 12);
        g.setGradientFill (juce::ColourGradient (
            MOColors::accent, accentBar.getX(), accentBar.getY(),
            MOColors::peach,  accentBar.getX(), accentBar.getBottom(), false));
        g.fillRoundedRectangle (accentBar, 1.5f);

        // アクティブタブ下線
        if (activeTab < tabButtons.size())
        {
            auto* btn = tabButtons[(int) activeTab];
            auto bBounds = btn->getBounds();
            g.setColour (getTabAccent (activeTab));
            g.fillRect (bBounds.getX(), getHeight() - 2, bBounds.getWidth(), 2);
        }
    }

    void resized() override
    {
        auto area = getLocalBounds();
        area.removeFromLeft (130);  // ロゴ領域

        int btnW = 80;
        int gap  = 5;
        int startX = area.getX() + 10;

        for (int i = 0; i < tabButtons.size(); ++i)
        {
            tabButtons[i]->setBounds (startX + i * (btnW + gap), 6, btnW, getHeight() - 12);
        }
    }

private:
    Tab activeTab = Main;
    juce::OwnedArray<juce::TextButton> tabButtons;

    const juce::String tabNames[NumTabs] = { "MAIN", "STAGE 1", "STAGE 2", "MASTER" };

    static juce::Colour getTabAccent (Tab t)
    {
        switch (t)
        {
            case Main:    return MOColors::accent;
            case Stage1:  return MOColors::peach;
            case Stage2:  return MOColors::babyBlue;
            case Master:  return MOColors::lavender;
            default:      return MOColors::accent;
        }
    }

    void styleButton (juce::TextButton* btn, bool active)
    {
        if (active)
        {
            btn->setColour (juce::TextButton::buttonColourId, getTabAccent (activeTab).withAlpha (0.20f));
            btn->setColour (juce::TextButton::textColourOffId, MOColors::text);
        }
        else
        {
            btn->setColour (juce::TextButton::buttonColourId, MOColors::knobTrack);
            btn->setColour (juce::TextButton::textColourOffId, MOColors::textDim);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TabHeader)
};

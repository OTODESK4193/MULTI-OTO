#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "ColorPalette.h"

struct StageMeter;

// ============================================================================
//  DynVisualComponent
//  3-BAND OTT リアルタイム可視化 (LOW/MID/HIGH 帯域別カラー)
//  ＋ タイトル部が選択ボタンとして動作 (onStageSelected)
//  ＋ Upward / Downward 設定シェードガイド ＆ マウス直感操作
// ============================================================================
class DynVisualComponent : public juce::Component,
                           private juce::Timer
{
public:
    DynVisualComponent() { startTimerHz (30); }
    ~DynVisualComponent() override { stopTimer(); }

    void setMeter (const StageMeter* m) { meter = m; }
    void setTitle (const juce::String& t) { title = t; }
    void setSelected (bool sel) { isSelected = sel; repaint(); }
    bool getSelected() const { return isSelected; }

    void setCrossoverFreqs (std::atomic<float>* lo, std::atomic<float>* hi)
    {
        xoverLo = lo;
        xoverHi = hi;
    }

    void bindStageParameters (juce::AudioProcessorValueTreeState& apvts, int stageNum)
    {
        stage = stageNum;
        juce::String st = juce::String (stageNum);

        paramGain[0] = apvts.getParameter ("s" + st + "_gain_l");
        paramGain[1] = apvts.getParameter ("s" + st + "_gain_m");
        paramGain[2] = apvts.getParameter ("s" + st + "_gain_h");

        paramUp[0] = apvts.getParameter ("s" + st + "_up_l");
        paramUp[1] = apvts.getParameter ("s" + st + "_up_m");
        paramUp[2] = apvts.getParameter ("s" + st + "_up_h");

        paramDown[0] = apvts.getParameter ("s" + st + "_down_l");
        paramDown[1] = apvts.getParameter ("s" + st + "_down_m");
        paramDown[2] = apvts.getParameter ("s" + st + "_down_h");
    }

    std::function<void(int stageNum)> onStageSelected;

    void paint (juce::Graphics& g) override;

    // --- マウス操作 ---
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;

private:
    void timerCallback() override;

    int stage = 1;
    bool isSelected = false;
    const StageMeter* meter = nullptr;
    std::atomic<float>* xoverLo = nullptr;
    std::atomic<float>* xoverHi = nullptr;

    juce::String title { "STAGE 1" };

    juce::RangedAudioParameter* paramGain[3] = { nullptr, nullptr, nullptr };
    juce::RangedAudioParameter* paramUp[3]   = { nullptr, nullptr, nullptr };
    juce::RangedAudioParameter* paramDown[3] = { nullptr, nullptr, nullptr };

    int hoveredBand = -1;
    int draggedBand = -1;
    enum DragTarget { TargetGain, TargetUpward, TargetDownward } dragTarget = TargetGain;
    float dragStartValue = 0.0f;

    float smoothEnvDb[3]  = { -60.f, -60.f, -60.f };
    float smoothGainDb[3] = { 0.f, 0.f, 0.f };

    static constexpr float kAlpha   = 0.45f;
    static constexpr float kMinDb   = -60.0f;
    static constexpr float kMaxDb   =   0.0f;
    static constexpr float kGainMin = -18.0f;
    static constexpr float kGainMax =  18.0f;

    int getBandAtPosition (juce::Point<int> pos) const;
    bool isHeaderPosition (juce::Point<int> pos) const;

    static float levelToNorm (float db)
    {
        return juce::jlimit (0.0f, 1.0f, (db - kMinDb) / (kMaxDb - kMinDb));
    }
    static float gainToNorm (float db)
    {
        return juce::jlimit (-1.0f, 1.0f, db / kGainMax);
    }
    static juce::String freqToString (float f)
    {
        if (f >= 1000.0f)
            return juce::String (f / 1000.0f, 1) + " kHz";
        return juce::String ((int) f) + " Hz";
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DynVisualComponent)
};

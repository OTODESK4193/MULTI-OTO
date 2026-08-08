#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "ColorPalette.h"
#include "DSP/ModMatrix.h"

struct StageMeter;

// ============================================================================
//  DynVisualComponent
//  3-BAND OTT リアルタイム可視化 (LOW/MID/HIGH 帯域別カラー)
//
//  マウス操作:
//    ・バンド上半分を上下ドラッグ  → UPWARD
//    ・バンド下半分を上下ドラッグ  → DOWNWARD
//    ・Shift / Alt + ドラッグ      → BAND GAIN
//    ・ダブルクリック              → GAIN を 0 dB へ
//    ・バンド境界を左右ドラッグ    → クロスオーバー周波数 (↔ カーソル)
// ============================================================================
class DynVisualComponent : public juce::Component,
                           private juce::Timer
{
public:
    DynVisualComponent() { startTimerHz (30); }
    ~DynVisualComponent() override { stopTimer(); }

    void setMeter (const StageMeter* m) { meter = m; }

    /** 帯域ゲインの変調レンジをメーター上に重ねるために使う */
    void setModMatrix (const ModMatrix* m) { matrix = m; }
    void setTitle (const juce::String& t) { title = t; }
    void setSelected (bool sel) { isSelected = sel; repaint(); }
    bool getSelected() const { return isSelected; }

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

        // Stage 1 は従来 ID を流用 (旧セッションとの互換のため)
        const juce::String loID = (stageNum == 1) ? "xover_low"  : "s2_xover_low";
        const juce::String hiID = (stageNum == 1) ? "xover_high" : "s2_xover_high";
        paramXLow  = apvts.getParameter (loID);
        paramXHigh = apvts.getParameter (hiID);
    }

    std::function<void(int stageNum)> onStageSelected;

    void paint (juce::Graphics& g) override;

    // --- マウス操作 ---
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp   (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;

private:
    void timerCallback() override;

    int stage = 1;
    bool isSelected = false;
    const StageMeter* meter = nullptr;
    const ModMatrix*  matrix = nullptr;

    juce::String title { "STAGE 1" };

    juce::RangedAudioParameter* paramGain[3] = { nullptr, nullptr, nullptr };
    juce::RangedAudioParameter* paramUp[3]   = { nullptr, nullptr, nullptr };
    juce::RangedAudioParameter* paramDown[3] = { nullptr, nullptr, nullptr };
    juce::RangedAudioParameter* paramXLow    = nullptr;
    juce::RangedAudioParameter* paramXHigh   = nullptr;

    int hoveredBand = -1;
    int draggedBand = -1;
    int hoveredBoundary = -1;   // 0 = LOW/MID, 1 = MID/HIGH
    int draggedBoundary = -1;

    enum DragTarget { TargetGain, TargetUpward, TargetDownward } dragTarget = TargetGain;
    float dragStartValue = 0.0f;

    float smoothEnvDb[3]  = { -60.f, -60.f, -60.f };
    float smoothGainDb[3] = { 0.f, 0.f, 0.f };

    static constexpr float kAlpha   = 0.45f;
    static constexpr float kMinDb   = -60.0f;
    static constexpr float kMaxDb   =   0.0f;
    static constexpr float kGainMin = -18.0f;
    static constexpr float kGainMax =  18.0f;
    static constexpr int   kBoundaryGrab = 6;   // 境界の当たり判定 (±px)

    float getLoFreq() const;        // MOD 適用後 (表示・当たり判定とも同じ値を使う)
    float getHiFreq() const;
    float applyXoverMod (int dstBase, float base, float lo, float hi) const;

    /** バンド矩形の位置と幅を求める (paint / ヒットテストで共用) */
    void computeBandLayout (juce::Rectangle<int>& areaOut, int bandX[3], int bandW[3]) const;

    int  getBandAtPosition (juce::Point<int> pos) const;
    int  getBoundaryAtPosition (juce::Point<int> pos) const;
    bool isHeaderPosition (juce::Point<int> pos) const;
    void applyBoundaryDrag (int boundary, int mouseX);

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

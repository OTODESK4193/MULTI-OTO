#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>
#include "ColorPalette.h"

// ============================================================================
//  StageMeter — DSP → GUI 受け渡し用 (lock-free)
//  EngineCore.h 側にも同一の構造体を定義している。
//  ここでは GUI 側参照用として forward-include する。
// ============================================================================
struct StageMeter;   // EngineCore.h で定義

// ============================================================================
//  DynVisualComponent
//  Wavetable DynVisual 準拠の 3-BAND OTT リアルタイム可視化。
//  30 Hz タイマーで DSP の StageMeter をポーリングし、
//  指数平滑 (alpha = 0.45) でちらつきを抑えて描画する。
// ============================================================================
class DynVisualComponent : public juce::Component,
                           private juce::Timer
{
public:
    DynVisualComponent() { startTimerHz (30); }
    ~DynVisualComponent() override { stopTimer(); }

    /** DSP 側メーターへのポインタをセット */
    void setMeter (const StageMeter* m) { meter = m; }

    /** ヘッダーテキスト (例: "STAGE 1 / 3-BAND OTT") */
    void setTitle (const juce::String& t) { title = t; }

    /** クロスオーバー周波数の参照値セット */
    void setCrossoverFreqs (std::atomic<float>* lo, std::atomic<float>* hi)
    {
        xoverLo = lo;
        xoverHi = hi;
    }

    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override;

    const StageMeter* meter = nullptr;
    std::atomic<float>* xoverLo = nullptr;
    std::atomic<float>* xoverHi = nullptr;

    juce::String title { "3-BAND OTT" };

    // 平滑化バッファ
    float smoothEnvDb[3]  = { -60.f, -60.f, -60.f };
    float smoothGainDb[3] = { 0.f, 0.f, 0.f };

    static constexpr float kAlpha   = 0.45f;
    static constexpr float kMinDb   = -60.0f;
    static constexpr float kMaxDb   =   0.0f;
    static constexpr float kGainMin = -18.0f;
    static constexpr float kGainMax =  18.0f;

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

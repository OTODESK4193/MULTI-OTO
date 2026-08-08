// ============================================================================
//  ModMatrix.h
//  MULTI-OTO 用モジュレーションマトリクス (ブロックレート処理)
//
//  Sources : LFO x4 (Free / Tempo Sync, 7 波形)
//            ENV FOLLOW (入力レベル追従)  ※本機はエフェクトなので Velocity や
//                                            Note は存在しない。代わりに入力の
//                                            音量そのものをソースにする。
//            DRIFT (連続的にゆらぐランダム。階段状に飛ぶ LFO の S&H / Rnd Trig
//                   とは性格を分けてあり、聴けば区別がつくようにしている)
//  Slots   : 8 ( Source x Amount(-1..+1) -> Destination, Uni / Bipolar )
//  Dests   : TIME / クロスオーバー / MIX / 各バンドの ATK・REL / LFO Rate
//
//  【重要】Dst の値はプリセットと DAW セッションに「番号」で保存される。
//  途中に挿入すると既存プロジェクトのアサイン先が全部ズレるため、
//  新しい行き先は必ず NumDsts の直前に追記すること。
// ============================================================================
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <cmath>

class ModMatrix
{
public:
    static constexpr int kNumLfos  = 4;
    static constexpr int kNumSlots = 8;

    enum Src
    {
        SrcNone = 0,
        SrcLfo1, SrcLfo2, SrcLfo3, SrcLfo4,
        SrcEnvFollow,
        SrcDrift,        // 旧 SrcRandom。値は変えないこと (セッション互換)
        NumSrcs
    };

    enum Dst
    {
        DstNone = 0,

        DstS1Time,  DstS2Time,
        DstS1XLow,  DstS1XHigh,
        DstS2XLow,  DstS2XHigh,
        DstS1Mix,   DstS2Mix,

        DstS1AtkL, DstS1AtkM, DstS1AtkH,
        DstS1RelL, DstS1RelM, DstS1RelH,
        DstS2AtkL, DstS2AtkM, DstS2AtkH,
        DstS2RelL, DstS2RelM, DstS2RelH,

        // LFO 同士のクロスモジュレーション用
        DstLfo1Rate, DstLfo2Rate, DstLfo3Rate, DstLfo4Rate,

        NumDsts
    };

    static juce::StringArray getSourceNames()
    {
        return { "None", "LFO 1", "LFO 2", "LFO 3", "LFO 4", "Env Follow", "Drift" };
    }

    /** Dst enum と 1:1 で対応させること (順番を変えない) */
    static juce::StringArray getDestNames()
    {
        return {
            "None",
            "S1 Time",   "S2 Time",
            "S1 Low X",  "S1 High X",
            "S2 Low X",  "S2 High X",
            "S1 Mix",    "S2 Mix",
            "S1 Atk Low", "S1 Atk Mid", "S1 Atk Hi",
            "S1 Rel Low", "S1 Rel Mid", "S1 Rel Hi",
            "S2 Atk Low", "S2 Atk Mid", "S2 Atk Hi",
            "S2 Rel Low", "S2 Rel Mid", "S2 Rel Hi",
            "LFO 1 Rate", "LFO 2 Rate", "LFO 3 Rate", "LFO 4 Rate"
        };
    }

    static juce::StringArray getWaveNames()
    {
        return { "Sine", "Triangle", "Saw", "Square", "S&H", "Chaos", "Rnd Trig" };
    }

    static juce::StringArray getSyncRateNames()
    {
        return { "1/1", "1/2", "1/2T", "1/4", "1/4.", "1/4T",
                 "1/8", "1/8.", "1/8T", "1/16", "1/16.", "1/16T", "1/32" };
    }

    struct Params
    {
        struct Lfo  { float rateHz = 1.0f; bool sync = false; int rateSync = 6; int wave = 0; };
        struct Slot { int src = 0; int dst = 0; float amt = 0.0f; bool uni = false; };

        std::array<Lfo,  kNumLfos>  lfo;
        std::array<Slot, kNumSlots> slot;
        double bpm = 120.0;
    };

    // ------------------------------------------------------------------
    void prepare (double sr)
    {
        sampleRate = (sr > 1000.0) ? sr : 44100.0;
        reset();
    }

    void reset()
    {
        for (auto& l : lfoState) l = {};
        envFollow   = 0.0f;
        driftValue  = 0.0f;
        driftTarget = 0.0f;
        driftCountdown = 0;
        destAccum.fill (0.0f);
        rangeMin.fill (0.0f);
        rangeMax.fill (0.0f);
        for (auto& a : guiDestAccum) a.store (0.0f, std::memory_order_relaxed);
        for (auto& a : guiRangeMin)  a.store (0.0f, std::memory_order_relaxed);
        for (auto& a : guiRangeMax)  a.store (0.0f, std::memory_order_relaxed);
        for (auto& a : guiLfoValue)  a.store (0.0f, std::memory_order_relaxed);
    }

    /** 入力レベル (0..1 正規化済み) を渡す。PluginProcessor が毎ブロック更新する。 */
    void setEnvFollow (float v) noexcept { envFollow = juce::jlimit (0.0f, 1.0f, v); }

    // ------------------------------------------------------------------
    void processBlock (int numSamples, const Params& p)
    {
        static const double beatsTable[13] = {
            4.0, 2.0, 4.0 / 3.0, 1.0, 1.5, 2.0 / 3.0,
            0.5, 0.75, 1.0 / 3.0, 0.25, 0.375, 1.0 / 6.0, 0.125 };

        std::array<float, NumDsts> localAccum {};
        std::array<float, NumDsts> localMin {};
        std::array<float, NumDsts> localMax {};

        float src[NumSrcs] = {};

        // --- LFO ---
        for (int i = 0; i < kNumLfos; ++i)
        {
            const auto& lp = p.lfo[(size_t) i];
            auto& st = lfoState[(size_t) i];

            double freq = (double) lp.rateHz;
            if (lp.sync)
            {
                const double bpm = (p.bpm > 1.0) ? p.bpm : 120.0;
                freq = bpm / (60.0 * beatsTable[juce::jlimit (0, 12, lp.rateSync)]);
            }

            // destAccum は前ブロックまでの集計値なので、自分自身の Rate を
            // 自分で変調するような自己参照でも 1 ブロック遅延で安全に成立する。
            const float rateMod = destAccum[(size_t) (DstLfo1Rate + i)];
            if (std::abs (rateMod) > 0.0001f)
                freq = juce::jlimit (0.01, 40.0, freq + (double) rateMod * 15.0);

            const float v = lfoValue (st, lp.wave);
            src[SrcLfo1 + i] = v;
            guiLfoValue[(size_t) i].store (v, std::memory_order_relaxed);

            const double inc = freq * (double) numSamples / sampleRate;
            st.phase  += inc;
            st.phase2 += inc * 1.41421356;

            while (st.phase >= 1.0)
            {
                st.phase -= 1.0;
                st.shValue = rng.nextFloat() * 2.0f - 1.0f;

                // Rnd Trig: 1 周期ごとに 50% の確率でだけ値を更新する。
                // 周期は一定でも「鳴る/鳴らない」が不規則になり、
                // ステップシーケンサ的なつっかえた動きになる。
                if (rng.nextFloat() < 0.5f)
                    st.trigValue = rng.nextFloat() * 2.0f - 1.0f;
            }
            while (st.phase2 >= 1.0) st.phase2 -= 1.0;
        }

        // --- ENV FOLLOW / DRIFT ---
        src[SrcEnvFollow] = envFollow;

        // DRIFT は「階段状に飛ばない」連続的なランダム。
        // 約 1 秒ごとに新しい目標値を引き、そこへ時定数 0.4 秒で滑らかに寄る。
        //   ・LFO の S&H      : 一定周期で毎回カクッと飛ぶ
        //   ・LFO の Rnd Trig : 一定周期だが 50% の確率でしか飛ばない (リズムが不規則)
        //   ・DRIFT           : そもそも飛ばない。ゆらゆらと当てもなく漂う
        // この 3 つを聴き分けられるように、あえて性格を分けている。
        {
            const int blocksPerSec = juce::jmax (1, (int) (sampleRate / juce::jmax (1, numSamples)));
            if (--driftCountdown <= 0)
            {
                driftTarget = rng.nextFloat() * 2.0f - 1.0f;
                driftCountdown = juce::jmax (1, blocksPerSec);
            }

            const float blockSec = (float) ((double) numSamples / sampleRate);
            const float coef = 1.0f - std::exp (-blockSec / 0.40f);
            driftValue += juce::jlimit (0.0f, 1.0f, coef) * (driftTarget - driftValue);
        }
        src[SrcDrift] = driftValue;

        // --- スロット集計 ---
        for (const auto& s : p.slot)
        {
            if (s.src <= SrcNone || s.src >= NumSrcs)  continue;
            if (s.dst <= DstNone || s.dst >= NumDsts)  continue;
            if (std::abs (s.amt) < 0.0001f)            continue;

            const bool srcBip = isBipolarSource (s.src);

            float v = src[s.src];
            if (s.uni) { if (srcBip)  v = (v + 1.0f) * 0.5f; }
            else       { if (! srcBip) v = v * 2.0f - 1.0f; }

            localAccum[(size_t) s.dst] += v * s.amt;

            const float lo = s.uni ? 0.0f : -1.0f;
            const float c1 = lo * s.amt, c2 = 1.0f * s.amt;
            localMin[(size_t) s.dst] += juce::jmin (c1, c2);
            localMax[(size_t) s.dst] += juce::jmax (c1, c2);
        }

        destAccum = localAccum;
        rangeMin  = localMin;
        rangeMax  = localMax;

        for (size_t d = 0; d < NumDsts; ++d)
        {
            guiDestAccum[d].store (localAccum[d], std::memory_order_relaxed);
            guiRangeMin[d] .store (localMin[d],   std::memory_order_relaxed);
            guiRangeMax[d] .store (localMax[d],   std::memory_order_relaxed);
        }
    }

    // ------------------------------------------------------------------
    float get (int dst) const noexcept
    {
        return destAccum[(size_t) juce::jlimit (0, (int) NumDsts - 1, dst)];
    }

    /** GUI 用 (ロックフリー) */
    float getForGui (int dst) const noexcept
    {
        return guiDestAccum[(size_t) juce::jlimit (0, (int) NumDsts - 1, dst)].load (std::memory_order_relaxed);
    }

    /** GUI 用: この行き先に掛かる変調の下限 / 上限 (-1..+1 の抽象量) */
    float getRangeMinForGui (int dst) const noexcept
    {
        return guiRangeMin[(size_t) juce::jlimit (0, (int) NumDsts - 1, dst)].load (std::memory_order_relaxed);
    }
    float getRangeMaxForGui (int dst) const noexcept
    {
        return guiRangeMax[(size_t) juce::jlimit (0, (int) NumDsts - 1, dst)].load (std::memory_order_relaxed);
    }

    float getLfoValue (int i) const noexcept
    {
        return guiLfoValue[(size_t) juce::jlimit (0, kNumLfos - 1, i)].load (std::memory_order_relaxed);
    }

    static bool isBipolarSource (int s) noexcept
    {
        return (s >= SrcLfo1 && s <= SrcLfo4) || s == SrcDrift;
    }

    // ==================================================================
    //  変調量 (-1..+1) を実際のパラメータ値へ適用する。
    //
    //  周波数と時間は「加算」ではなく「指数変化」にしている。
    //  88Hz に ±500Hz を足しても下側は潰れてしまうが、±2 オクターブなら
    //  22Hz〜352Hz と上下対称に振れて音楽的になるため。
    //  クランプは呼び出し側 (パラメータのレンジ) で行う。
    // ==================================================================
    static double applyModToValue (int dst, double baseValue, double mod) noexcept
    {
        switch (dst)
        {
        // TIME は倍率 (%) なので ±2 オクターブ = 1/4 倍 〜 4 倍
        case DstS1Time: case DstS2Time:
            return baseValue * std::pow (2.0, mod * 2.0);

        // クロスオーバー周波数は ±2 オクターブ
        case DstS1XLow: case DstS1XHigh:
        case DstS2XLow: case DstS2XHigh:
            return baseValue * std::pow (2.0, mod * 2.0);

        // MIX は 0..100% なので ±50 を加算
        case DstS1Mix: case DstS2Mix:
            return baseValue + mod * 50.0;

        // LFO Rate は Hz 加算
        case DstLfo1Rate: case DstLfo2Rate: case DstLfo3Rate: case DstLfo4Rate:
            return baseValue + mod * 15.0;

        default:
            // ATK / REL (ms) は ±3 オクターブ = 1/8 倍 〜 8 倍
            if (dst >= DstS1AtkL && dst <= DstS2RelH)
                return baseValue * std::pow (2.0, mod * 3.0);
            return baseValue + mod;
        }
    }

private:
    struct LfoState
    {
        double phase = 0.0;
        double phase2 = 0.0;
        float  shValue = 0.0f;
        float  trigValue = 0.0f;
    };

    float lfoValue (const LfoState& st, int wave) const noexcept
    {
        const float ph = (float) st.phase;
        switch (wave)
        {
        case 0: return std::sin (ph * juce::MathConstants<float>::twoPi);
        case 1: return 1.0f - 4.0f * std::abs (ph - 0.5f);
        case 2: return 2.0f * ph - 1.0f;
        case 3: return ph < 0.5f ? 1.0f : -1.0f;
        case 4: return st.shValue;
        case 5: return (std::sin (ph * juce::MathConstants<float>::twoPi)
                      + std::sin ((float) st.phase2 * juce::MathConstants<float>::twoPi)) * 0.5f;
        case 6: return st.trigValue;
        default: return 0.0f;
        }
    }

    double sampleRate = 44100.0;
    std::array<LfoState, kNumLfos> lfoState;

    float envFollow   = 0.0f;
    float driftValue  = 0.0f;
    float driftTarget = 0.0f;
    int   driftCountdown = 0;

    std::array<float, NumDsts> destAccum {};
    std::array<float, NumDsts> rangeMin {};
    std::array<float, NumDsts> rangeMax {};

    juce::Random rng;

    std::array<std::atomic<float>, NumDsts>  guiDestAccum {};
    std::array<std::atomic<float>, NumDsts>  guiRangeMin {};
    std::array<std::atomic<float>, NumDsts>  guiRangeMax {};
    std::array<std::atomic<float>, kNumLfos> guiLfoValue {};
};

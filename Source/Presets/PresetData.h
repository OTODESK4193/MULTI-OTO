// ============================================================================
//  PresetData.h
//  MULTI-OTO ファクトリープリセット (バイナリ内蔵・読み取り専用)
//
//  設計メモ
//   ・OTT COUNT (total_ott) はプリセットに含めない。名前の (xNN) は推奨値。
//   ・段数が増えるほど帯域ゲインは累乗されるため、32 段以上のプリセットでは
//     gain を 0dB 近辺に抑えている。
//   ・MIX は Xfer OTT の "Depth" 相当 (ノード内 Dry/Wet)、
//     DEPTH は各バンドの上下コンプ量、TIME は Attack/Release のスケール (%)。
// ============================================================================
#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <utility>

struct MOStage
{
    float gainL, gainM, gainH;    // dB   (-24 .. 24)
    float depthL, depthM, depthH; // %    (0 .. 100)
    float upL, upM, upH;          // %    (0 .. 100)
    float dnL, dnM, dnH;          // %    (0 .. 100)
    float atkL, atkM, atkH;       // ms   (0.1 .. 100)
    float relL, relM, relH;       // ms   (1 .. 1000)
    float time;                   // %    (10 .. 1000)
    float mix;                    // %    (0 .. 100)
    bool  on;
};

struct MOPreset
{
    const char* name;
    const char* category;
    const char* description;
    int   suggestedCount;

    bool  predrive;
    float inGain, drive, odd, even;

    float xLow, xHigh;      // Stage 1
    float x2Low, x2High;    // Stage 2
    bool  xLink;

    MOStage s1, s2;

    float hpf, lpf, dryWet, outGain, ceiling;
    int   phaseMode;        // 0 = COLOR, 1 = ALIGN
};

// 標準の OTT タイミング (Xfer OTT の既定値に近い) ------------------------------
#define MO_ATK_STD   47.8f, 22.4f, 13.5f
#define MO_REL_STD   168.0f, 282.0f, 132.0f
#define MO_ATK_FAST  2.0f, 1.2f, 0.6f
#define MO_REL_FAST  30.0f, 18.0f, 10.0f
#define MO_ATK_SLOW  90.0f, 70.0f, 55.0f
#define MO_REL_SLOW  900.0f, 800.0f, 700.0f
#define MO_FULL      100.0f, 100.0f, 100.0f

class PresetData
{
public:
    static const std::vector<MOPreset>& getFactoryPresets()
    {
        static const std::vector<MOPreset> presets = {

        // ================= BASIC : 一般的な OTT の用途 =====================
        { "Classic OTT (x2)", "Basic",
          "Xfer OTT の既定値に近い素直なセッティング。まずここから。", 2,
          false, 0, 0, 0, 0,   88, 2500, 88, 2500, true,
          { 8.4f,5.8f,8.6f,  50,50,50,  MO_FULL,  MO_FULL,  MO_ATK_STD, MO_REL_STD, 100, 100, true },
          { 8.4f,5.8f,8.6f,  50,50,50,  MO_FULL,  MO_FULL,  MO_ATK_STD, MO_REL_STD, 100, 100, true },
          20, 20000, 100, -3.0f, -0.1f, 0 },

        { "Gentle Glue (x2)", "Basic",
          "ダイナミクスを殺さずに密度だけ上げる。バスやループの下地に。", 2,
          false, 0, 0, 0, 0,   110, 3200, 110, 3200, true,
          { 0,0,0,  35,35,30,  80,80,80,  100,100,100,  MO_ATK_SLOW, MO_REL_STD, 250, 28, true },
          { 0,0,0,  35,35,30,  80,80,80,  100,100,100,  MO_ATK_SLOW, MO_REL_STD, 250, 28, true },
          20, 20000, 100, 0, -0.1f, 1 },

        { "Vocal Presence (x2)", "Basic",
          "中高域の細部を持ち上げて前に出す。低域はほぼ触らない。", 2,
          false, 0, 0, 0, 0,   150, 2800, 150, 2800, true,
          { 0,3.0f,4.0f,  15,60,70,  60,100,100,  100,90,80,  MO_ATK_STD, MO_REL_STD, 120, 45, true },
          { 0,3.0f,4.0f,  15,60,70,  60,100,100,  100,90,80,  MO_ATK_STD, MO_REL_STD, 120, 45, true },
          80, 18000, 100, 0, -0.1f, 1 },

        { "Drum Punch (x4)", "Basic",
          "速い TIME でトランジェントを締める。ドラムループ向け。", 4,
          false, 0, 0, 0, 0,   120, 3500, 120, 3500, true,
          { 2.0f,1.0f,3.0f,  45,45,50,  60,60,70,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 25, 60, true },
          { 2.0f,1.0f,3.0f,  45,45,50,  60,60,70,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 25, 60, true },
          20, 20000, 100, -1.5f, -0.1f, 1 },

        { "Bass Weight (x2)", "Basic",
          "低域だけ厚く。サブを潰さず 120Hz 以下を持ち上げる。", 2,
          false, 0, 0, 0, 0,   120, 2200, 120, 2200, true,
          { 4.0f,0,0,  70,25,20,  100,50,40,  60,100,100,  MO_ATK_SLOW, MO_REL_STD, 180, 55, true },
          { 4.0f,0,0,  70,25,20,  100,50,40,  60,100,100,  MO_ATK_SLOW, MO_REL_STD, 180, 55, true },
          20, 20000, 100, -1.0f, -0.1f, 1 },

        { "Sub Guardian (x4)", "Basic",
          "60Hz 以下をアップワードさせずダウンワードだけで整える。", 4,
          false, 0, 0, 0, 0,   60, 2500, 60, 2500, true,
          { 0,4.0f,4.0f,  60,50,50,  0,100,100,  MO_FULL,  MO_ATK_SLOW, MO_REL_STD, 150, 60, true },
          { 0,4.0f,4.0f,  60,50,50,  0,100,100,  MO_FULL,  MO_ATK_SLOW, MO_REL_STD, 150, 60, true },
          28, 20000, 100, -1.0f, -0.1f, 1 },

        // ================= BASS MUSIC : 迫力・攻撃性 =======================
        { "Riddim Growl (x8)", "Bass",
          "8 段で密度を稼ぎつつ ODD 倍音で歯を立てる。リディム系ベースに。", 8,
          true, 0, 35, 60, 0,   95, 2400, 95, 2400, true,
          { 2.0f,1.0f,2.0f,  60,60,60,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 40, 80, true },
          { 1.0f,1.0f,1.5f,  55,60,65,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 40, 80, true },
          25, 18000, 100, -4.0f, -0.2f, 0 },

        { "Color Bass Shimmer (x16)", "Bass",
          "高域クロスを 4kHz に上げ、EVEN 倍音でキラつきを足す。", 16,
          true, 0, 25, 30, 45,   90, 4000, 90, 4000, true,
          { 0,1.0f,2.0f,  50,60,80,  MO_FULL,  90,100,100,  MO_ATK_STD, MO_REL_STD, 60, 70, true },
          { 0,1.0f,2.0f,  50,60,80,  MO_FULL,  90,100,100,  MO_ATK_STD, MO_REL_STD, 60, 70, true },
          22, 20000, 100, -5.0f, -0.2f, 0 },

        { "Neuro Grit (x8)", "Bass",
          "極端に速い TIME で細かい粒立ちを出す。ニューロ系リース向け。", 8,
          true, 0, 55, 80, 10,   105, 2100, 105, 2100, true,
          { 1.5f,1.5f,2.0f,  70,70,70,  80,80,80,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 20, 85, true },
          { 1.0f,1.0f,1.0f,  65,70,75,  80,80,80,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 20, 85, true },
          28, 17000, 100, -5.0f, -0.2f, 0 },

        { "Tearout Screech (x16)", "Bass",
          "高域クロスを 1.6kHz まで下げ、上の帯域を全力で引き上げる。", 16,
          true, 0, 45, 75, 0,   80, 1600, 80, 1600, true,
          { -2.0f,0,3.0f,  40,60,100,  60,90,100,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 30, 90, true },
          { -2.0f,0,2.0f,  40,60,100,  60,90,100,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 30, 90, true },
          30, 16000, 100, -6.0f, -0.2f, 0 },

        { "Wobble Enhancer (x4)", "Bass",
          "遅い TIME で LFO の揺れを追いかけさせ、うねりを強調する。", 4,
          false, 0, 15, 40, 0,   130, 2600, 130, 2600, true,
          { 3.0f,1.0f,1.0f,  55,50,45,  MO_FULL,  80,100,100,  MO_ATK_SLOW, MO_REL_SLOW, 300, 65, true },
          { 3.0f,1.0f,1.0f,  55,50,45,  MO_FULL,  80,100,100,  MO_ATK_SLOW, MO_REL_SLOW, 300, 65, true },
          22, 20000, 100, -2.0f, -0.1f, 0 },

        { "Hard Clip Lead (x8)", "Bass",
          "PRE-DRIVE を強めに入れて潰し、その歪みを 8 段で磨き上げる。", 8,
          true, 4.0f, 80, 100, 0,   100, 3000, 100, 3000, true,
          { 1.0f,1.0f,1.5f,  55,55,60,  70,80,90,  MO_FULL,  MO_ATK_FAST, MO_REL_STD, 45, 75, true },
          { 1.0f,1.0f,1.5f,  55,55,60,  70,80,90,  MO_FULL,  MO_ATK_FAST, MO_REL_STD, 45, 75, true },
          25, 17000, 100, -5.0f, -0.2f, 0 },

        // ================= TEXTURE : アーティファクトを積極利用 ============
        { "Droopy Laser (x32)", "Texture",
          "32 段の位相回転でトランジェントが伸び、レーザー状に尾を引く。", 32,
          false, 0, 0, 0, 0,   88, 2500, 88, 2500, true,
          { 0.5f,0.5f,0.5f,  60,60,60,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 35, 100, true },
          { 0.5f,0.5f,0.5f,  60,60,60,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 35, 100, true },
          25, 18000, 100, -8.0f, -0.3f, 0 },

        { "Phase Smear (x32)", "Texture",
          "ALIGN PHASE + DRY/WET 60% で、原音と位相の揃った滲みを混ぜる。", 32,
          false, 0, 0, 0, 0,   88, 2500, 88, 2500, true,
          { 0,0,0,  50,50,50,  MO_FULL,  MO_FULL,  MO_ATK_STD, MO_REL_STD, 80, 100, true },
          { 0,0,0,  50,50,50,  MO_FULL,  MO_FULL,  MO_ATK_STD, MO_REL_STD, 80, 100, true },
          20, 20000, 60, -4.0f, -0.2f, 1 },

        { "Metallic Resonator (x64)", "Texture",
          "LOW X と HIGH X をぎりぎりまで近づけ、中域に金属的な共鳴を作る。", 64,
          false, 0, 0, 0, 0,   400, 1100, 400, 1100, true,
          { 0,0,0,  65,65,65,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 25, 100, true },
          { 0,0,0,  65,65,65,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 25, 100, true },
          30, 16000, 100, -9.0f, -0.3f, 0 },

        { "Granular Dust (x32)", "Texture",
          "極短アタック/リリースで粒状のザラつきを生む。パッドやノイズに。", 32,
          false, 0, 0, 0, 0,   140, 3800, 140, 3800, true,
          { 0,0,0.5f,  75,75,75,  MO_FULL,  MO_FULL,  0.5f,0.5f,0.5f,  8.0f,8.0f,8.0f, 12, 100, true },
          { 0,0,0.5f,  75,75,75,  MO_FULL,  MO_FULL,  0.5f,0.5f,0.5f,  8.0f,8.0f,8.0f, 12, 100, true },
          35, 19000, 100, -7.0f, -0.3f, 0 },

        { "Infinite Tail (x64)", "Texture",
          "アップワードを最大・ダウンワードを最小にして、減衰を永遠に伸ばす。", 64,
          false, 0, 0, 0, 0,   88, 2500, 88, 2500, true,
          { -1.0f,-1.0f,-1.0f,  90,90,90,  MO_FULL,  20,20,20,  MO_ATK_SLOW, MO_REL_SLOW, 600, 100, true },
          { -1.0f,-1.0f,-1.0f,  90,90,90,  MO_FULL,  20,20,20,  MO_ATK_SLOW, MO_REL_SLOW, 600, 100, true },
          25, 18000, 100, -8.0f, -0.3f, 0 },

        { "Spectral Freeze (x64)", "Texture",
          "TIME を限界まで遅くしてエンベロープを凍らせる。持続音が固まる。", 64,
          false, 0, 0, 0, 0,   88, 2500, 88, 2500, true,
          { 0,0,0,  100,100,100,  MO_FULL,  40,40,40,  MO_ATK_SLOW, MO_REL_SLOW, 950, 100, true },
          { 0,0,0,  100,100,100,  MO_FULL,  40,40,40,  MO_ATK_SLOW, MO_REL_SLOW, 950, 100, true },
          25, 18000, 100, -9.0f, -0.3f, 0 },

        { "Split Band Fracture (x32)", "Texture",
          "LINK を切り、Stage1 と Stage2 で違う帯域分割をぶつける。", 32,
          false, 0, 0, 0, 0,   70, 1400, 320, 6000, false,
          { 0,0,0,  70,70,70,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 30, 100, true },
          { 0,0,0,  70,70,70,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 30, 100, true },
          25, 18000, 100, -8.0f, -0.3f, 0 },

        { "Ghost Reverb (x64)", "Texture",
          "アップワードだけを効かせ、部屋鳴りのような尾を人工的に作り出す。", 64,
          false, 0, 0, 0, 0,   180, 3000, 180, 3000, true,
          { -2.0f,-2.0f,-2.0f,  85,85,85,  MO_FULL,  10,10,10,  MO_ATK_SLOW, MO_REL_SLOW, 750, 85, true },
          { -2.0f,-2.0f,-2.0f,  85,85,85,  MO_FULL,  10,10,10,  MO_ATK_SLOW, MO_REL_SLOW, 750, 85, true },
          40, 15000, 100, -8.0f, -0.3f, 0 },

        // ================= DESTROY : 完全に破壊する ========================
        { "Pulveriser (x128)", "Destroy",
          "128 段フル稼働。原音の輪郭は残らない。まずはこれで壊す。", 128,
          false, 0, 0, 0, 0,   88, 2500, 88, 2500, true,
          { 0,0,0,  100,100,100,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 30, 100, true },
          { 0,0,0,  100,100,100,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 30, 100, true },
          25, 18000, 100, -10.0f, -0.3f, 0 },

        { "White Hot (x128)", "Destroy",
          "高域クロスを 6kHz に上げて上だけ焼き切る。シンバルやノイズに。", 128,
          false, 0, 0, 0, 0,   88, 6000, 88, 6000, true,
          { 0.3f,0.3f,0.5f,  80,90,100,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 20, 100, true },
          { 0.3f,0.3f,0.5f,  80,90,100,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 20, 100, true },
          30, 20000, 100, -11.0f, -0.3f, 0 },

        { "Digital Meltdown (x128)", "Destroy",
          "PRE-DRIVE を振り切ってから 128 段。デジタルに溶ける音。", 128,
          true, 6.0f, 100, 100, 60,   88, 2500, 88, 2500, true,
          { 0,0,0,  100,100,100,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 15, 100, true },
          { 0,0,0,  100,100,100,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 15, 100, true },
          30, 16000, 100, -12.0f, -0.4f, 0 },

        { "Screaming Comb (x128)", "Destroy",
          "帯域を 700-1050Hz に押し込め、櫛形の悲鳴を発生させる。", 128,
          false, 0, 0, 0, 0,   700, 1050, 700, 1050, true,
          { 0,0,0,  100,100,100,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 18, 100, true },
          { 0,0,0,  100,100,100,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 18, 100, true },
          40, 14000, 100, -12.0f, -0.4f, 0 },

        { "Sub Annihilator (x64)", "Destroy",
          "低域を叩き潰して中高域だけを暴走させる。細く鋭い破壊音。", 64,
          false, 0, 0, 0, 0,   200, 2500, 200, 2500, true,
          { -6.0f,0,1.0f,  100,100,100,  0,100,100,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 22, 100, true },
          { -6.0f,0,1.0f,  100,100,100,  0,100,100,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 22, 100, true },
          60, 17000, 100, -9.0f, -0.3f, 0 },

        { "Stutter Chaos (x128)", "Destroy",
          "超高速の追従とフルデプスでゲートのように途切れさせる。", 128,
          false, 0, 0, 0, 0,   110, 3300, 110, 3300, true,
          { 0,0,0,  100,100,100,  MO_FULL,  MO_FULL,  0.2f,0.2f,0.2f,  2.0f,2.0f,2.0f, 10, 100, true },
          { 0,0,0,  100,100,100,  MO_FULL,  MO_FULL,  0.2f,0.2f,0.2f,  2.0f,2.0f,2.0f, 10, 100, true },
          28, 18000, 100, -11.0f, -0.4f, 0 },

        // ================= DRIVE : PRE-DRIVE 主体 ==========================
        { "Odd Harmonic Stack (x8)", "Drive",
          "ODD 倍音のみ。矩形波寄りの硬い歪みを 8 段で整える。", 8,
          true, 2.0f, 70, 100, 0,   100, 2800, 100, 2800, true,
          { 1.0f,1.0f,1.0f,  40,40,45,  80,80,80,  MO_FULL,  MO_ATK_STD, MO_REL_STD, 70, 70, true },
          { 1.0f,1.0f,1.0f,  40,40,45,  80,80,80,  MO_FULL,  MO_ATK_STD, MO_REL_STD, 70, 70, true },
          25, 18000, 100, -4.0f, -0.2f, 0 },

        { "Even Warmth (x8)", "Drive",
          "EVEN 倍音で非対称な太さを出す。遅めの TIME で自然に馴染ませる。", 8,
          true, 0, 45, 20, 75,   120, 3000, 120, 3000, true,
          { 1.0f,0.5f,0.5f,  35,35,30,  70,70,70,  100,100,90,  MO_ATK_SLOW, MO_REL_STD, 150, 55, true },
          { 1.0f,0.5f,0.5f,  35,35,30,  70,70,70,  100,100,90,  MO_ATK_SLOW, MO_REL_STD, 150, 55, true },
          22, 19000, 100, -3.0f, -0.1f, 1 },

        { "Fuzz Crusher (x32)", "Drive",
          "入力を突っ込んで PRE-DRIVE で潰し、32 段で毛羽立たせる。", 32,
          true, 6.0f, 100, 90, 45,   95, 2600, 95, 2600, true,
          { 0,0,0.5f,  80,80,80,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 28, 100, true },
          { 0,0,0.5f,  80,80,80,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 28, 100, true },
          30, 16000, 100, -9.0f, -0.3f, 0 },

        // ================= UTILITY ========================================
        { "INIT (Bypass)", "Utility",
          "全パラメータをニュートラルに。音作りをゼロから始めるとき用。", 2,
          false, 0, 0, 0, 0,   88, 2500, 88, 2500, true,
          { 0,0,0,  0,0,0,  MO_FULL,  MO_FULL,  MO_ATK_STD, MO_REL_STD, 100, 0, true },
          { 0,0,0,  0,0,0,  MO_FULL,  MO_FULL,  MO_ATK_STD, MO_REL_STD, 100, 0, true },
          20, 20000, 100, 0, -0.1f, 1 },
        };

        return presets;
    }

    static juce::StringArray getFactoryCategories()
    {
        juce::StringArray cats;
        for (const auto& p : getFactoryPresets())
            cats.addIfNotAlreadyThere (juce::String (p.category));
        cats.sort (true);
        return cats;
    }

    /** プリセットを (paramID, 実値) の並びへ展開する */
    static std::vector<std::pair<juce::String, float>> toParameterValues (const MOPreset& p)
    {
        std::vector<std::pair<juce::String, float>> out;
        out.reserve (80);

        auto add = [&out] (const juce::String& id, float v) { out.emplace_back (id, v); };

        add ("predrive_on",   p.predrive ? 1.0f : 0.0f);
        add ("in_gain",       p.inGain);
        add ("drive",         p.drive);
        add ("odd_blend",     p.odd);
        add ("even_blend",    p.even);

        add ("xover_low",     p.xLow);
        add ("xover_high",    p.xHigh);
        add ("s2_xover_low",  p.x2Low);
        add ("s2_xover_high", p.x2High);
        add ("xover_link",    p.xLink ? 1.0f : 0.0f);

        auto addStage = [&add] (const char* n, const MOStage& s)
        {
            const juce::String st (n);
            add ("s" + st + "_on", s.on ? 1.0f : 0.0f);

            add ("s" + st + "_gain_l", s.gainL);   add ("s" + st + "_gain_m", s.gainM);   add ("s" + st + "_gain_h", s.gainH);
            add ("s" + st + "_depth_l", s.depthL); add ("s" + st + "_depth_m", s.depthM); add ("s" + st + "_depth_h", s.depthH);
            add ("s" + st + "_up_l", s.upL);       add ("s" + st + "_up_m", s.upM);       add ("s" + st + "_up_h", s.upH);
            add ("s" + st + "_down_l", s.dnL);     add ("s" + st + "_down_m", s.dnM);     add ("s" + st + "_down_h", s.dnH);
            add ("s" + st + "_atk_l", s.atkL);     add ("s" + st + "_atk_m", s.atkM);     add ("s" + st + "_atk_h", s.atkH);
            add ("s" + st + "_rel_l", s.relL);     add ("s" + st + "_rel_m", s.relM);     add ("s" + st + "_rel_h", s.relH);
            add ("s" + st + "_time", s.time);      add ("s" + st + "_mix", s.mix);
        };

        addStage ("1", p.s1);
        addStage ("2", p.s2);

        add ("post_hpf",   p.hpf);
        add ("post_lpf",   p.lpf);
        add ("dry_wet",    p.dryWet);
        add ("out_gain",   p.outGain);
        add ("limit_ceil", p.ceiling);
        add ("phase_mode", (float) p.phaseMode);

        return out;
    }
};

#undef MO_ATK_STD
#undef MO_REL_STD
#undef MO_ATK_FAST
#undef MO_REL_FAST
#undef MO_ATK_SLOW
#undef MO_REL_SLOW
#undef MO_FULL

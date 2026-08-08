// ============================================================================
//  PresetData.h
//  MULTI-OTO ファクトリープリセット (バイナリ内蔵・読み取り専用)
//
//  設計メモ
//   ・OTT COUNT (total_ott) はプリセットに含めない。名前の (xNN) は推奨値。
//   ・Stage1 と Stage2 のクロスオーバーは完全に独立。
//     2 段で異なる帯域分割をぶつけると、片方の境界がもう片方のバンド内側に
//     落ちるため、帯域の重なりから複雑な位相干渉と共鳴が生まれる。
//     これが本機を「直列 OTT」から「サウンドデザインツール」に変える核。
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

    float xLow,  xHigh;     // Stage 1 の帯域分割
    float x2Low, x2High;    // Stage 2 の帯域分割 (Stage 1 とは独立)

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
          "Xfer OTT の既定値に近い素直なセッティング。2 段とも同じ 88Hz/2.5kHz。", 2,
          false, 0, 0, 0, 0,   88, 2500,   88, 2500,
          { 8.4f,5.8f,8.6f,  50,50,50,  MO_FULL,  MO_FULL,  MO_ATK_STD, MO_REL_STD, 100, 100, true },
          { 8.4f,5.8f,8.6f,  50,50,50,  MO_FULL,  MO_FULL,  MO_ATK_STD, MO_REL_STD, 100, 100, true },
          20, 20000, 100, -3.0f, -0.1f, 0 },

        { "Gentle Glue (x2)", "Basic",
          "S1=110/3.2k, S2=220/5.5k。境界をずらして帯域の継ぎ目を目立たなくする。", 2,
          false, 0, 0, 0, 0,   110, 3200,   220, 5500,
          { 0,0,0,  35,35,30,  80,80,80,  100,100,100,  MO_ATK_SLOW, MO_REL_STD, 250, 28, true },
          { 0,0,0,  30,30,28,  80,80,80,  100,100,100,  MO_ATK_SLOW, MO_REL_STD, 250, 24, true },
          20, 20000, 100, 0, -0.1f, 1 },

        { "Vocal Presence (x2)", "Basic",
          "S1 で中域、S2 は 320Hz/5.2k に上げて上の帯域をもう一度磨く。", 2,
          false, 0, 0, 0, 0,   150, 2800,   320, 5200,
          { 0,3.0f,4.0f,  15,60,70,  60,100,100,  100,90,80,  MO_ATK_STD, MO_REL_STD, 120, 45, true },
          { 0,2.0f,3.0f,  15,50,65,  60,100,100,  100,90,80,  MO_ATK_STD, MO_REL_STD, 120, 40, true },
          80, 18000, 100, 0, -0.1f, 1 },

        { "Drum Punch (x4)", "Basic",
          "S1=120/3.5k でボディ、S2=65/1.6k でアタック帯を分けて締める。", 4,
          false, 0, 0, 0, 0,   120, 3500,   65, 1600,
          { 2.0f,1.0f,3.0f,  45,45,50,  60,60,70,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 25, 60, true },
          { 1.0f,1.0f,2.0f,  40,45,50,  50,60,70,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 22, 55, true },
          20, 20000, 100, -1.5f, -0.1f, 1 },

        { "Bass Weight (x2)", "Basic",
          "S1=120/2.2k、S2=55/1.1k。2 段目でサブと低中域をさらに切り分ける。", 2,
          false, 0, 0, 0, 0,   120, 2200,   55, 1100,
          { 4.0f,0,0,  70,25,20,  100,50,40,  60,100,100,  MO_ATK_SLOW, MO_REL_STD, 180, 55, true },
          { 3.0f,1.0f,0,  60,30,20,  100,60,40,  70,100,100,  MO_ATK_SLOW, MO_REL_STD, 180, 50, true },
          20, 20000, 100, -1.0f, -0.1f, 1 },

        { "Sub Guardian (x4)", "Basic",
          "S1=60Hz でサブを保護し、S2=130/4.2k で上を整える二段構え。", 4,
          false, 0, 0, 0, 0,   60, 2500,   130, 4200,
          { 0,4.0f,4.0f,  60,50,50,  0,100,100,  MO_FULL,  MO_ATK_SLOW, MO_REL_STD, 150, 60, true },
          { 0,3.0f,3.0f,  50,50,50,  0,100,100,  MO_FULL,  MO_ATK_SLOW, MO_REL_STD, 150, 55, true },
          28, 20000, 100, -1.0f, -0.1f, 1 },

        // ================= BASS MUSIC : 迫力・攻撃性 =======================
        { "Riddim Growl (x8)", "Bass",
          "S2 を 190Hz/1.2k の狭い中域にして、唸りの帯域だけを二重に潰す。", 8,
          true, 0, 35, 60, 0,   95, 2400,   190, 1200,
          { 2.0f,1.0f,2.0f,  60,60,60,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 40, 80, true },
          { 1.0f,2.0f,1.5f,  55,70,65,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 40, 80, true },
          25, 18000, 100, -4.0f, -0.2f, 0 },

        { "Color Bass Shimmer (x16)", "Bass",
          "S1=4k、S2=8.5k と段ごとに上へずらし、倍音を階段状に持ち上げる。", 16,
          true, 0, 25, 30, 45,   90, 4000,   260, 8500,
          { 0,1.0f,2.0f,  50,60,80,  MO_FULL,  90,100,100,  MO_ATK_STD, MO_REL_STD, 60, 70, true },
          { 0,1.0f,2.5f,  50,60,85,  MO_FULL,  90,100,100,  MO_ATK_STD, MO_REL_STD, 55, 70, true },
          22, 20000, 100, -5.0f, -0.2f, 0 },

        { "Neuro Grit (x8)", "Bass",
          "S2=380/1.15k の極狭ミッド。速い TIME と合わせて粒立ちを荒くする。", 8,
          true, 0, 55, 80, 10,   105, 2100,   380, 1150,
          { 1.5f,1.5f,2.0f,  70,70,70,  80,80,80,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 20, 85, true },
          { 1.0f,2.0f,1.0f,  65,80,75,  80,90,80,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 18, 85, true },
          28, 17000, 100, -5.0f, -0.2f, 0 },

        { "Tearout Screech (x16)", "Bass",
          "S1=1.6k で上を薄く切り出し、S2=230/5k でその上をさらに引き上げる。", 16,
          true, 0, 45, 75, 0,   80, 1600,   230, 5000,
          { -2.0f,0,3.0f,  40,60,100,  60,90,100,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 30, 90, true },
          { -2.0f,0,2.5f,  40,70,100,  60,90,100,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 28, 90, true },
          30, 16000, 100, -6.0f, -0.2f, 0 },

        { "Wobble Enhancer (x4)", "Bass",
          "S2=50/1.4k。低い方の分割でうねりの底を掴み、遅い TIME で追従させる。", 4,
          false, 0, 15, 40, 0,   130, 2600,   50, 1400,
          { 3.0f,1.0f,1.0f,  55,50,45,  MO_FULL,  80,100,100,  MO_ATK_SLOW, MO_REL_SLOW, 300, 65, true },
          { 3.0f,1.5f,1.0f,  60,50,45,  MO_FULL,  80,100,100,  MO_ATK_SLOW, MO_REL_SLOW, 320, 65, true },
          22, 20000, 100, -2.0f, -0.1f, 0 },

        { "Hard Clip Lead (x8)", "Bass",
          "PRE-DRIVE で潰し、S1/S2 を 100/3k と 340/6.5k に分けて歪みを磨く。", 8,
          true, 4.0f, 80, 100, 0,   100, 3000,   340, 6500,
          { 1.0f,1.0f,1.5f,  55,55,60,  70,80,90,  MO_FULL,  MO_ATK_FAST, MO_REL_STD, 45, 75, true },
          { 1.0f,1.0f,2.0f,  55,60,65,  70,80,90,  MO_FULL,  MO_ATK_FAST, MO_REL_STD, 45, 75, true },
          25, 17000, 100, -5.0f, -0.2f, 0 },

        // ================= TEXTURE : アーティファクトを積極利用 ============
        { "Droopy Laser (x32)", "Texture",
          "S2=300/7k。S1 のハイバンド内に S2 の境界を落とし、位相を二重に回す。", 32,
          false, 0, 0, 0, 0,   88, 2500,   300, 7000,
          { 0.5f,0.5f,0.5f,  60,60,60,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 35, 100, true },
          { 0.5f,0.5f,0.5f,  60,60,60,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 32, 100, true },
          25, 18000, 100, -8.0f, -0.3f, 0 },

        { "Phase Smear (x32)", "Texture",
          "ALIGN PHASE + DRY/WET 60%。S2=165/4.5k でずらした滲みを原音に重ねる。", 32,
          false, 0, 0, 0, 0,   88, 2500,   165, 4500,
          { 0,0,0,  50,50,50,  MO_FULL,  MO_FULL,  MO_ATK_STD, MO_REL_STD, 80, 100, true },
          { 0,0,0,  50,50,50,  MO_FULL,  MO_FULL,  MO_ATK_STD, MO_REL_STD, 80, 100, true },
          20, 20000, 60, -4.0f, -0.2f, 1 },

        { "Metallic Resonator (x64)", "Texture",
          "S1=400/1.1k、S2=630/1.5k。近接した狭帯域を二段重ねて金属的に鳴らす。", 64,
          false, 0, 0, 0, 0,   400, 1100,   630, 1500,
          { 0,0,0,  65,65,65,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 25, 100, true },
          { 0,0,0,  65,65,65,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 25, 100, true },
          30, 16000, 100, -9.0f, -0.3f, 0 },

        { "Granular Dust (x32)", "Texture",
          "極短アタック/リリース。S2=58/1.25k で低い方から粒を作り直す。", 32,
          false, 0, 0, 0, 0,   140, 3800,   58, 1250,
          { 0,0,0.5f,  75,75,75,  MO_FULL,  MO_FULL,  0.5f,0.5f,0.5f,  8.0f,8.0f,8.0f, 12, 100, true },
          { 0,0,0.5f,  75,75,75,  MO_FULL,  MO_FULL,  0.5f,0.5f,0.5f,  8.0f,8.0f,8.0f, 12, 100, true },
          35, 19000, 100, -7.0f, -0.3f, 0 },

        { "Infinite Tail (x64)", "Texture",
          "アップワード最大・ダウンワード最小。S2=240/6k で尾の帯域をずらす。", 64,
          false, 0, 0, 0, 0,   88, 2500,   240, 6000,
          { -1.0f,-1.0f,-1.0f,  90,90,90,  MO_FULL,  20,20,20,  MO_ATK_SLOW, MO_REL_SLOW, 600, 100, true },
          { -1.0f,-1.0f,-1.0f,  90,90,90,  MO_FULL,  20,20,20,  MO_ATK_SLOW, MO_REL_SLOW, 650, 100, true },
          25, 18000, 100, -8.0f, -0.3f, 0 },

        { "Spectral Freeze (x64)", "Texture",
          "TIME を限界まで遅く。S2=480/9k と大きくずらして凍り方を帯域ごとに変える。", 64,
          false, 0, 0, 0, 0,   88, 2500,   480, 9000,
          { 0,0,0,  100,100,100,  MO_FULL,  40,40,40,  MO_ATK_SLOW, MO_REL_SLOW, 950, 100, true },
          { 0,0,0,  100,100,100,  MO_FULL,  40,40,40,  MO_ATK_SLOW, MO_REL_SLOW, 950, 100, true },
          25, 18000, 100, -9.0f, -0.3f, 0 },

        { "Split Band Fracture (x32)", "Texture",
          "S1=70/1.4k と S2=330/6k。分割が全く噛み合わず、帯域が裂けて聴こえる。", 32,
          false, 0, 0, 0, 0,   70, 1400,   330, 6000,
          { 0,0,0,  70,70,70,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 30, 100, true },
          { 0,0,0,  70,70,70,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 30, 100, true },
          25, 18000, 100, -8.0f, -0.3f, 0 },

        { "Ghost Reverb (x64)", "Texture",
          "アップワードのみで人工的な残響を作る。S2=85/8k で尾を広帯域に散らす。", 64,
          false, 0, 0, 0, 0,   180, 3000,   85, 8000,
          { -2.0f,-2.0f,-2.0f,  85,85,85,  MO_FULL,  10,10,10,  MO_ATK_SLOW, MO_REL_SLOW, 750, 85, true },
          { -2.0f,-2.0f,-2.0f,  85,85,85,  MO_FULL,  10,10,10,  MO_ATK_SLOW, MO_REL_SLOW, 800, 85, true },
          40, 15000, 100, -8.0f, -0.3f, 0 },

        // ================= DESTROY : 完全に破壊する ========================
        { "Pulveriser (x128)", "Destroy",
          "128 段フル稼働。S2=350/6k でずらし、原音の輪郭を完全に消す。", 128,
          false, 0, 0, 0, 0,   88, 2500,   350, 6000,
          { 0,0,0,  100,100,100,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 30, 100, true },
          { 0,0,0,  100,100,100,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 30, 100, true },
          25, 18000, 100, -10.0f, -0.3f, 0 },

        { "White Hot (x128)", "Destroy",
          "S1=6k、S2=12k と上へ上へ。シンバルやノイズの最上部だけを焼き切る。", 128,
          false, 0, 0, 0, 0,   88, 6000,   520, 12000,
          { 0.3f,0.3f,0.5f,  80,90,100,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 20, 100, true },
          { 0.3f,0.3f,0.5f,  80,90,100,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 18, 100, true },
          30, 20000, 100, -11.0f, -0.3f, 0 },

        { "Digital Meltdown (x128)", "Destroy",
          "PRE-DRIVE 全開の後、S2=720/1.15k の極狭帯域に押し込んで溶かす。", 128,
          true, 6.0f, 100, 100, 60,   88, 2500,   720, 1150,
          { 0,0,0,  100,100,100,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 15, 100, true },
          { 0,0,0,  100,100,100,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 15, 100, true },
          30, 16000, 100, -12.0f, -0.4f, 0 },

        { "Screaming Comb (x128)", "Destroy",
          "S1=700/1.05k、S2=880/1.4k。二重の極狭帯域で櫛形の悲鳴を発生させる。", 128,
          false, 0, 0, 0, 0,   700, 1050,   880, 1400,
          { 0,0,0,  100,100,100,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 18, 100, true },
          { 0,0,0,  100,100,100,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 18, 100, true },
          40, 14000, 100, -12.0f, -0.4f, 0 },

        { "Sub Annihilator (x64)", "Destroy",
          "S1=200Hz で低域を叩き潰し、S2=45/1.08k で残骸をもう一度掘り返す。", 64,
          false, 0, 0, 0, 0,   200, 2500,   45, 1080,
          { -6.0f,0,1.0f,  100,100,100,  0,100,100,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 22, 100, true },
          { -4.0f,0,1.0f,  100,100,100,  0,100,100,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 22, 100, true },
          60, 17000, 100, -9.0f, -0.3f, 0 },

        { "Stutter Chaos (x128)", "Destroy",
          "超高速追従でゲート状に途切れる。S2=430/7.5k でその刻みを帯域ごとにずらす。", 128,
          false, 0, 0, 0, 0,   110, 3300,   430, 7500,
          { 0,0,0,  100,100,100,  MO_FULL,  MO_FULL,  0.2f,0.2f,0.2f,  2.0f,2.0f,2.0f, 10, 100, true },
          { 0,0,0,  100,100,100,  MO_FULL,  MO_FULL,  0.2f,0.2f,0.2f,  2.0f,2.0f,2.0f, 10, 100, true },
          28, 18000, 100, -11.0f, -0.4f, 0 },

        // ================= DRIVE : PRE-DRIVE 主体 ==========================
        { "Odd Harmonic Stack (x8)", "Drive",
          "ODD 倍音のみの硬い歪み。S2=270/5.2k でその倍音帯を選んで整える。", 8,
          true, 2.0f, 70, 100, 0,   100, 2800,   270, 5200,
          { 1.0f,1.0f,1.0f,  40,40,45,  80,80,80,  MO_FULL,  MO_ATK_STD, MO_REL_STD, 70, 70, true },
          { 1.0f,1.0f,1.0f,  40,45,50,  80,80,80,  MO_FULL,  MO_ATK_STD, MO_REL_STD, 70, 70, true },
          25, 18000, 100, -4.0f, -0.2f, 0 },

        { "Even Warmth (x8)", "Drive",
          "EVEN 倍音で非対称な太さを。S2=210/6k と広めに取って自然に馴染ませる。", 8,
          true, 0, 45, 20, 75,   120, 3000,   210, 6000,
          { 1.0f,0.5f,0.5f,  35,35,30,  70,70,70,  100,100,90,  MO_ATK_SLOW, MO_REL_STD, 150, 55, true },
          { 1.0f,0.5f,0.5f,  30,35,30,  70,70,70,  100,100,90,  MO_ATK_SLOW, MO_REL_STD, 150, 50, true },
          22, 19000, 100, -3.0f, -0.1f, 1 },

        { "Fuzz Crusher (x32)", "Drive",
          "入力を突っ込んで潰し、S2=390/9k で毛羽立ちの帯域を大きくずらす。", 32,
          true, 6.0f, 100, 90, 45,   95, 2600,   390, 9000,
          { 0,0,0.5f,  80,80,80,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 28, 100, true },
          { 0,0,0.5f,  80,80,80,  MO_FULL,  MO_FULL,  MO_ATK_FAST, MO_REL_FAST, 28, 100, true },
          30, 16000, 100, -9.0f, -0.3f, 0 },

        // ================= UTILITY ========================================
        { "INIT (Bypass)", "Utility",
          "全パラメータをニュートラルに。音作りをゼロから始めるとき用。", 2,
          false, 0, 0, 0, 0,   88, 2500,   88, 2500,
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

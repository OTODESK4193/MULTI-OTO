#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <atomic>
#include "Crossover.h"
#include "DynamicsNode.h"
#include "ADAASaturator.h"

// ============================================================================
//  StageMeter — DSP → GUI ロックフリー受け渡し構造体
// ============================================================================
struct StageMeter {
    std::atomic<float> envDb[3]  = { {-60.f}, {-60.f}, {-60.f} };  // 入力レベル (dB)
    std::atomic<float> gainDb[3] = { {0.f}, {0.f}, {0.f} };         // ゲイン変化量 (dB)
};

struct EngineParams {
    float inGain, drive, odd, even;
    float xLow,  xHigh;    // Stage 1 のクロスオーバー
    float xLow2, xHigh2;   // Stage 2 のクロスオーバー
    float s1_gain[3], s1_depth[3], s1_up[3], s1_down[3], s1_time, s1_mix, s1_atk[3], s1_rel[3];
    float s2_gain[3], s2_depth[3], s2_up[3], s2_down[3], s2_time, s2_mix, s2_atk[3], s2_rel[3];
    float post_hpf, post_lpf, dryWet, outGain, limitCeil, limitRelease;
    int total_ott_count, phase_mode, limitMode;   // limitMode: 0 = LIMIT, 1 = CLIP
    bool predrive_on, s1_on, s2_on;
};

class EngineCore {
public:
    EngineCore();
    void prepare(double sampleRate, int samplesPerBlock);
    void updateParameters(const EngineParams& p);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

private:
    static constexpr int MAX_NODES = 128;

    void processChunk(float* left, float* right, int numSamples);
    void writeMeters();

    juce::SmoothedValue<float> driveSmoother, oddSmoother, evenSmoother, inGainSmoother, outGainSmoother;
    EngineParams currentParams {};   // updateParameters 前に process されても安全なようゼロ初期化

    std::vector<Crossover> crossovers;
    std::vector<Crossover> dryCrossovers;
    std::vector<DynamicsNode> nodes;

    juce::dsp::StateVariableTPTFilter<float> preLpfL, preLpfR, postHpfL, postHpfR, postLpfL, postLpfR;
    juce::dsp::IIR::Filter<float> dcBlockL, dcBlockR;

    std::vector<juce::dsp::SIMDRegister<float>> simdBuffer;
    juce::AudioBuffer<float> dryBuffer;
    ADAASaturator satL, satR;

public:
    // GUI がポーリングするメーター出力
    StageMeter s1Meter, s2Meter;

private:
    float limiterEnvL = 0, limiterEnvR = 0, limiterReleaseCoef = 0, currentLimitThreshold = 0.988f;
    double currentSampleRate = 48000.0;
    int  maxBlockSize = 512;
    int  activeCount  = -1;
    int  cachedHalf   = -1;
    float cachedLimitRelease = -1.0f;
    float cachedXLow1 = -1.0f, cachedXHigh1 = -1.0f;
    float cachedXLow2 = -1.0f, cachedXHigh2 = -1.0f;
    std::atomic<bool> isPrepared{ false };
};

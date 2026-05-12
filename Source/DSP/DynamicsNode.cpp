#include "DynamicsNode.h"
#include "FastMath.h"
#include <cmath>
#include <algorithm>

DynamicsNode::DynamicsNode() {}

void DynamicsNode::prepare(double sr, int samplesPerBlock) {
    juce::ignoreUnused(samplesPerBlock);
    currentSampleRate = sr;
    reset();
}

void DynamicsNode::reset() {
    envelope = Vector(0.0f);
}

void DynamicsNode::setParameters(const float* gains, const float* depths, float macroTime, const float* attacks, const float* releases) {
    // Macro Time (10% 〜 1000%) を基準となるスケール係数 (0.1 〜 10.0) に変換
    float timeScale = macroTime * 0.01f;

    alignas(32) float aArr[8];
    alignas(32) float rArr[8];
    alignas(32) float dArr[8];
    alignas(32) float gArr[8];

    // 0=Low, 1=Mid, 2=High
    for (int i = 0; i < 3; ++i) {
        // 設定されたAtk/RelにMacro Timeを掛け合わせる
        float finalAtkMs = std::max(0.1f, attacks[i] * timeScale);
        float finalRelMs = std::max(1.0f, releases[i] * timeScale);

        // ミリ秒からフィルタ係数への変換
        float aC = 1.0f - std::exp(-1.0f / (finalAtkMs * 0.001f * static_cast<float>(currentSampleRate)));
        float rC = 1.0f - std::exp(-1.0f / (finalRelMs * 0.001f * static_cast<float>(currentSampleRate)));

        // LchとRchに同じ係数を配置
        aArr[i * 2] = aC; aArr[i * 2 + 1] = aC;
        rArr[i * 2] = rC; rArr[i * 2 + 1] = rC;

        // Depth (0% 〜 100% を 0.0 〜 1.0 に)
        float dC = depths[i] * 0.01f;
        dArr[i * 2] = dC; dArr[i * 2 + 1] = dC;

        // Gain (dB からリニア倍率へ変換)
        float gC = std::pow(10.0f, gains[i] / 20.0f);
        gArr[i * 2] = gC; gArr[i * 2 + 1] = gC;
    }

    // パディング（未使用レーンの安全処理）
    aArr[6] = 1.0f; aArr[7] = 1.0f;
    rArr[6] = 1.0f; rArr[7] = 1.0f;
    dArr[6] = 0.0f; dArr[7] = 0.0f;
    gArr[6] = 1.0f; gArr[7] = 1.0f;

    attackCoef = Vector::fromRawArray(aArr);
    releaseCoef = Vector::fromRawArray(rArr);
    bandDepth = Vector::fromRawArray(dArr);
    bandGain = Vector::fromRawArray(gArr);
}

void DynamicsNode::process(Vector* simmData, int numSamples) {
    alignas(32) float rawAtk[8]; attackCoef.copyToRawArray(rawAtk);
    alignas(32) float rawRel[8]; releaseCoef.copyToRawArray(rawRel);
    alignas(32) float rawDepth[8]; bandDepth.copyToRawArray(rawDepth);
    alignas(32) float rawGain[8]; bandGain.copyToRawArray(rawGain);

    for (int i = 0; i < numSamples; ++i) {
        Vector input = simmData[i];

        alignas(32) float rawInput[8]; input.copyToRawArray(rawInput);
        alignas(32) float rawEnv[8]; envelope.copyToRawArray(rawEnv);
        alignas(32) float rawOutGain[8];

        for (int ch = 0; ch < 6; ++ch) {
            float inAbs = std::abs(rawInput[ch]);

            // 帯域独立のエンベロープフォロワー
            float coeff = (inAbs > rawEnv[ch]) ? rawAtk[ch] : rawRel[ch];
            rawEnv[ch] = rawEnv[ch] + coeff * (inAbs - rawEnv[ch]);

            float envDb = FastMath::fast_log2(rawEnv[ch]) * 6.0205f;

            // 圧縮計算とDepthの適用
            float overThreshold = std::max(0.0f, envDb - downwardThresholdDb);
            float downGainDb = -overThreshold * (1.0f - (1.0f / downwardRatio)) * rawDepth[ch];

            float underThreshold = std::max(0.0f, upwardThresholdDb - envDb);
            float upGainDb = underThreshold * (1.0f - upwardRatio) * rawDepth[ch];

            float totalGainDb = downGainDb + upGainDb;

            // 最終ゲイン(Linear)とBandGainの乗算
            rawOutGain[ch] = FastMath::fast_exp2(totalGainDb * 0.16609f) * rawGain[ch];
        }

        rawOutGain[6] = 1.0f; rawOutGain[7] = 1.0f;
        rawEnv[6] = 0.0f;  rawEnv[7] = 0.0f;

        envelope = Vector::fromRawArray(rawEnv);
        Vector linearGain = Vector::fromRawArray(rawOutGain);

        simmData[i] = input * linearGain;
    }
}
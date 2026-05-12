#include "DynamicsNode.h"
#include "FastMath.h"
#include <cmath>
#include <algorithm>

DynamicsNode::DynamicsNode() {}

void DynamicsNode::prepare(double sr, int samplesPerBlock) {
    juce::ignoreUnused(samplesPerBlock); // C4100警告を抑制
    currentSampleRate = sr;
    reset();

    // アタック 5ms, リリース 50ms
    float attackTime = 0.005f;
    float releaseTime = 0.050f;

    attackCoef = 1.0f - std::exp(-1.0f / (attackTime * static_cast<float>(currentSampleRate)));
    releaseCoef = 1.0f - std::exp(-1.0f / (releaseTime * static_cast<float>(currentSampleRate)));
}

void DynamicsNode::reset() {
    envelope = Vector(0.0f);
}

void DynamicsNode::process(Vector* simmData, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        Vector input = simmData[i];

        alignas(32) float rawInput[8];
        input.copyToRawArray(rawInput);

        alignas(32) float rawEnv[8];
        envelope.copyToRawArray(rawEnv);

        alignas(32) float rawGain[8];

        for (int ch = 0; ch < 6; ++ch) {
            // 1. 全波整流 (絶対値)
            float inAbs = std::abs(rawInput[ch]);

            // 2. エンベロープ・フォロワー
            // MSVCの /arch:AVX2 /O2 環境では、この三項演算子は
            // 自動的に分岐なしのブレンド命令(_mm256_blendv_ps)にコンパイルされます。
            float coeff = (inAbs > rawEnv[ch]) ? attackCoef : releaseCoef;
            rawEnv[ch] = rawEnv[ch] + coeff * (inAbs - rawEnv[ch]);

            // 3. Fast Mathによるゲイン計算
            float envDb = FastMath::fast_log2(rawEnv[ch]) * 6.0205f;

            // Downward Compression (音を潰す)
            float overThreshold = std::max(0.0f, envDb - downwardThresholdDb);
            float downGainDb = -overThreshold * (1.0f - (1.0f / downwardRatio)) * downwardDepth;

            // Upward Compression (音を持ち上げる)
            float underThreshold = std::max(0.0f, upwardThresholdDb - envDb);
            float upGainDb = underThreshold * (1.0f - upwardRatio) * upwardDepth;

            // トータルゲインの合算
            float totalGainDb = downGainDb + upGainDb;

            // dBからリニアへの変換 (Fast Math)
            rawGain[ch] = FastMath::fast_exp2(totalGainDb * 0.16609f);
        }

        // パディング（未使用レーン）の安全処理
        rawGain[6] = 1.0f; rawGain[7] = 1.0f;
        rawEnv[6] = 0.0f;  rawEnv[7] = 0.0f;

        // 配列からSIMDレジスタへ書き戻し
        envelope = Vector::fromRawArray(rawEnv);
        Vector linearGain = Vector::fromRawArray(rawGain);

        // 4. 入力にゲインを乗算 (6バンド一括処理)
        simmData[i] = input * linearGain;
    }
}
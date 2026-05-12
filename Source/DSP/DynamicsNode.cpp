#include "DynamicsNode.h"
#include <cmath>

DynamicsNode::DynamicsNode() {}

void DynamicsNode::prepare(double sr, int samplesPerBlock) {
    currentSampleRate = sr;
    reset();

    // プロトタイプ用の暫定アタック・リリースタイム（後日パラメータ化）
    float attackTime = 0.005f;  // 5ms
    float releaseTime = 0.050f; // 50ms

    float aCoef = std::exp(-1.0f / (attackTime * static_cast<float>(currentSampleRate)));
    float rCoef = std::exp(-1.0f / (releaseTime * static_cast<float>(currentSampleRate)));

    // AVX2の8レーン全てに同じ係数を配置 (6ストリーム+2パディング)
    alignas(32) float aArr[8] = { aCoef, aCoef, aCoef, aCoef, aCoef, aCoef, 0.0f, 0.0f };
    alignas(32) float rArr[8] = { rCoef, rCoef, rCoef, rCoef, rCoef, rCoef, 0.0f, 0.0f };

    attackCoef = Vector::fromRawArray(aArr);
    releaseCoef = Vector::fromRawArray(rArr);
}

void DynamicsNode::reset() {
    envelope = Vector(0.0f);
}

void DynamicsNode::process(Vector* simmData, int numSamples) {
    Vector one(1.0f);

    for (int i = 0; i < numSamples; ++i) {
        Vector input = simmData[i];

        // 1. レベル検出（プロトタイプのため近似RMSとして自乗値を使用）
        Vector square = input * input;

        // 2. エンベロープフォロワーの更新 (固定スムージング)
        envelope = envelope * releaseCoef + square * (one - releaseCoef);

        // 3. ゲイン計算 (プロトタイプは一旦パススルー、後にLUT-ADAA等を追加)
        Vector gain = one;

        // 4. 入力に乗算（6バンド同時処理）
        simmData[i] = input * gain;
    }
}
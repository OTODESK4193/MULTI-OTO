#include "EngineCore.h"
#include <cmath>
#include <algorithm>

EngineCore::EngineCore() {
    nodes.resize(NUM_NODES);
}

void EngineCore::prepare(double sampleRate, int samplesPerBlock) {
    juce::dsp::ProcessSpec spec{ sampleRate, static_cast<juce::uint32>(samplesPerBlock), 2 };
    crossover.prepare(spec);

    for (auto& node : nodes) {
        node.prepare(sampleRate, samplesPerBlock);
    }
    simdBuffer.resize(static_cast<size_t>(samplesPerBlock));

    // リミッターのリリース時間（約50ms）の係数を計算
    limiterReleaseCoef = std::exp(-1.0f / (0.050f * static_cast<float>(sampleRate)));
    reset();
}

void EngineCore::reset() {
    crossover.reset();
    for (auto& node : nodes) {
        node.reset();
    }
    limiterEnvL = 0.0f;
    limiterEnvR = 0.0f;
}

void EngineCore::process(juce::AudioBuffer<float>& buffer) {
    const int numSamples = buffer.getNumSamples();
    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    // =========================================================================
    // STEP 1: 分割 & パッキング (AoS → SoA 変換)
    // =========================================================================
    for (int i = 0; i < numSamples; ++i) {
        float lL, lR, mL, mR, hL, hR;
        crossover.process(left[i], right[i], lL, lR, mL, mR, hL, hR);
        alignas(32) float raw[8] = { lL, lR, mL, mR, hL, hR, 0.0f, 0.0f };
        simdBuffer[static_cast<size_t>(i)] = juce::dsp::SIMDRegister<float>::fromRawArray(raw);
    }

    // =========================================================================
    // STEP 2: マルチステージ OTT エンジン
    // =========================================================================
    for (auto& node : nodes) {
        node.process(simdBuffer.data(), numSamples);
    }

    // =========================================================================
    // STEP 3: アンパッキング & 再合成 & Safety Limiter
    // =========================================================================
    for (int i = 0; i < numSamples; ++i) {
        alignas(32) float raw[8];
        simdBuffer[static_cast<size_t>(i)].copyToRawArray(raw);

        // 帯域の再合成
        float outL = raw[0] + raw[2] + raw[4];
        float outR = raw[1] + raw[3] + raw[5];

        // --- Safety Limiter 処理 ---
        float absL = std::abs(outL);
        float absR = std::abs(outR);

        // アタックは即座(0ms)、リリースは滑らかに減衰
        limiterEnvL = (absL > limiterEnvL) ? absL : limiterEnvL * limiterReleaseCoef;
        limiterEnvR = (absR > limiterEnvR) ? absR : limiterEnvR * limiterReleaseCoef;

        // エンベロープがスレッショルドを超えた場合のみ、ゲインを下げて頭を押さえつける
        float gainL = (limiterEnvL > LIMITER_THRESHOLD) ? (LIMITER_THRESHOLD / limiterEnvL) : 1.0f;
        float gainR = (limiterEnvR > LIMITER_THRESHOLD) ? (LIMITER_THRESHOLD / limiterEnvR) : 1.0f;

        left[i] = outL * gainL;
        right[i] = outR * gainR;
    }
}
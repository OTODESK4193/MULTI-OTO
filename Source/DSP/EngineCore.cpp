#include "EngineCore.h"

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
}

void EngineCore::reset() {
    crossover.reset();
    for (auto& node : nodes) {
        node.reset();
    }
}

void EngineCore::process(juce::AudioBuffer<float>& buffer) {
    const int numSamples = buffer.getNumSamples();
    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    // =========================================================================
    // STEP 1: 分割 & パッキング (AoS → SoA 変換)
    // DAWからのステレオ信号を3バンドに分け、AVXレジスタにパッキングします
    // =========================================================================
    for (int i = 0; i < numSamples; ++i) {
        float lL, lR, mL, mR, hL, hR;
        crossover.process(left[i], right[i], lL, lR, mL, mR, hL, hR);

        // 32バイト境界にアライメントされた配列を作成 [L_Low, R_Low, L_Mid, R_Mid, L_High, R_High, Pad, Pad]
        alignas(32) float raw[8] = { lL, lR, mL, mR, hL, hR, 0.0f, 0.0f };
        simdBuffer[static_cast<size_t>(i)] = juce::dsp::SIMDRegister<float>::fromRawArray(raw);
    }

    // =========================================================================
    // STEP 2: マルチステージ OTT エンジン
    // SoA化された状態のまま、64個（今回は2個）のコンプレッサーを駆け抜けます
    // =========================================================================
    for (auto& node : nodes) {
        node.process(simdBuffer.data(), numSamples);
    }

    // =========================================================================
    // STEP 3: アンパッキング & 再合成 (SoA → AoS 変換)
    // 処理済みのSIMDレジスタを展開し、帯域を足し合わせてDAWに返します
    // =========================================================================
    for (int i = 0; i < numSamples; ++i) {
        alignas(32) float raw[8];
        simdBuffer[static_cast<size_t>(i)].copyToRawArray(raw);

        left[i] = raw[0] + raw[2] + raw[4]; // lL + mL + hL
        right[i] = raw[1] + raw[3] + raw[5]; // lR + mR + hR
    }
}
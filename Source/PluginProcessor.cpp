#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DSP/EngineCore.h"
#include "DSP/ModMatrix.h"
#include "Presets/PresetData.h"
#include "GUI/ColorPalette.h"

juce::AudioProcessorValueTreeState::ParameterLayout MultiOtoAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto addFloat = [&](const juce::String& id, const juce::String& name, float min, float max, float def) {
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(id, 1), name, juce::NormalisableRange<float>(min, max, 0.1f), def));
        };
    auto addFreq = [&](const juce::String& id, const juce::String& name, float min, float max, float def) {
        auto range = juce::NormalisableRange<float>(min, max, 1.0f);
        range.setSkewForCentre(std::sqrt(min * max));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(id, 1), name, range, def));
        };
    auto addBool = [&](const juce::String& id, const juce::String& name, bool def) {
        layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID(id, 1), name, def));
        };

    // 【変更】128を追加
    juce::StringArray ottChoices = { "2", "4", "8", "16", "32", "64", "128" };
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("total_ott", 1), "Total OTT Count", ottChoices, 0));

    addBool("predrive_on", "PreDrive Bypass", true);
    addBool("s1_on", "Stage 1 Bypass", true);
    addBool("s2_on", "Stage 2 Bypass", true);

    addFloat("in_gain", "Input Gain", -24.0f, 24.0f, 0.0f);
    addFloat("drive", "Drive Amount", 0.0f, 100.0f, 0.0f);
    addFloat("odd_blend", "Odd Harmonics", 0.0f, 100.0f, 0.0f);
    addFloat("even_blend", "Even Harmonics", 0.0f, 100.0f, 0.0f);

    // Stage 1 のクロスオーバー (旧バージョンとの互換のため ID は据え置き)
    addFreq("xover_low", "S1 Low Freq", 20.0f, 1000.0f, 88.0f);
    addFreq("xover_high", "S1 High Freq", 1000.0f, 20000.0f, 2500.0f);
    // Stage 2 のクロスオーバー (既定では Stage 1 と完全に独立)
    addFreq("s2_xover_low", "S2 Low Freq", 20.0f, 1000.0f, 88.0f);
    addFreq("s2_xover_high", "S2 High Freq", 1000.0f, 20000.0f, 2500.0f);
    // 編集補助。ON のとき Stage1 の LOW X / HIGH X を動かすと Stage2 も一緒に動く。
    // DSP は常に各ステージのパラメータをそのまま使う (音への直接の影響はない)。
    addBool("xover_link", "Crossover Link", false);

    auto buildStageParams = [&](int s) {
        juce::String st = juce::String(s);
        addFloat("s" + st + "_gain_h", "S" + st + " High Gain", -24.0f, 24.0f, 8.6f);
        addFloat("s" + st + "_gain_m", "S" + st + " Mid Gain", -24.0f, 24.0f, 5.8f);
        addFloat("s" + st + "_gain_l", "S" + st + " Low Gain", -24.0f, 24.0f, 8.4f);
        addFloat("s" + st + "_depth_h", "S" + st + " High Depth", 0.0f, 100.0f, 50.0f);
        addFloat("s" + st + "_depth_m", "S" + st + " Mid Depth", 0.0f, 100.0f, 50.0f);
        addFloat("s" + st + "_depth_l", "S" + st + " Low Depth", 0.0f, 100.0f, 50.0f);

        // Upward (弱音引き上げ %)
        addFloat("s" + st + "_up_h", "S" + st + " High Upward", 0.0f, 100.0f, 100.0f);
        addFloat("s" + st + "_up_m", "S" + st + " Mid Upward", 0.0f, 100.0f, 100.0f);
        addFloat("s" + st + "_up_l", "S" + st + " Low Upward", 0.0f, 100.0f, 100.0f);

        // Downward (大音量圧縮 %)
        addFloat("s" + st + "_down_h", "S" + st + " High Downward", 0.0f, 100.0f, 100.0f);
        addFloat("s" + st + "_down_m", "S" + st + " Mid Downward", 0.0f, 100.0f, 100.0f);
        addFloat("s" + st + "_down_l", "S" + st + " Low Downward", 0.0f, 100.0f, 100.0f);

        addFloat("s" + st + "_time", "S" + st + " Macro Time", 10.0f, 1000.0f, 50.0f);
        addFloat("s" + st + "_mix", "S" + st + " Mix", 0.0f, 100.0f, 100.0f);

        addFloat("s" + st + "_atk_h", "S" + st + " High Attack", 0.1f, 100.0f, 13.5f);
        addFloat("s" + st + "_atk_m", "S" + st + " Mid Attack", 0.1f, 100.0f, 22.4f);
        addFloat("s" + st + "_atk_l", "S" + st + " Low Attack", 0.1f, 100.0f, 47.8f);
        addFloat("s" + st + "_rel_h", "S" + st + " High Release", 1.0f, 1000.0f, 132.0f);
        addFloat("s" + st + "_rel_m", "S" + st + " Mid Release", 1.0f, 1000.0f, 282.0f);
        addFloat("s" + st + "_rel_l", "S" + st + " Low Release", 1.0f, 1000.0f, 168.0f);
        };

    buildStageParams(1);
    buildStageParams(2);

    addFreq("post_hpf", "Post HPF", 20.0f, 1000.0f, 20.0f);
    addFreq("post_lpf", "Post LPF", 1000.0f, 20000.0f, 20000.0f);

    addFloat("dry_wet", "Dry / Wet", 0.0f, 100.0f, 100.0f);
    addFloat("out_gain", "Output Gain", -24.0f, 24.0f, 0.0f);
    addFloat("limit_ceil", "Limiter Ceiling", -2.0f, -0.1f, -0.1f);

    // --- CONFIG (リミッター詳細 / 表示テーマ) ---
    addFloat("limit_release", "Limiter Release", 1.0f, 500.0f, 50.0f);
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("limit_mode", 1), "Limiter Mode",
        juce::StringArray{ "LIMIT", "CLIP" }, 0));
    // テーマ名は ColorPalette.h のテーブルが唯一の定義元 (追加時に両方直す必要がない)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("color_theme", 1), "Color Theme",
        MOColors::getThemeNames(), 0));

    juce::StringArray phaseChoices = { "COLOR PHASE", "ALIGN PHASE" };
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("phase_mode", 1), "Phase Mode", phaseChoices, 0));

    // ======================================================================
    //  MOD MATRIX
    // ======================================================================
    auto addChoice = [&](const juce::String& id, const juce::String& name,
                         const juce::StringArray& items, int def) {
        layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID(id, 1), name, items, def));
        };

    for (int i = 1; i <= ModMatrix::kNumLfos; ++i) {
        const juce::String n(i);
        addChoice("lfo" + n + "_wave", "LFO " + n + " Wave", ModMatrix::getWaveNames(), 0);
        addBool  ("lfo" + n + "_sync", "LFO " + n + " Sync", false);
        addChoice("lfo" + n + "_syncrate", "LFO " + n + " Sync Rate", ModMatrix::getSyncRateNames(), 6);

        // 0.01Hz (100秒) 〜 30Hz。低速側を細かく触れるよう強くスキューする
        auto r = juce::NormalisableRange<float>(0.01f, 30.0f, 0.01f);
        r.setSkewForCentre(1.0f);
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID("lfo" + n + "_rate", 1), "LFO " + n + " Rate", r, 1.0f));
    }

    for (int i = 1; i <= ModMatrix::kNumSlots; ++i) {
        const juce::String n(i);
        addChoice("mod" + n + "_src", "MOD " + n + " Source", ModMatrix::getSourceNames(), 0);
        addChoice("mod" + n + "_dst", "MOD " + n + " Dest",   ModMatrix::getDestNames(),   0);
        addFloat ("mod" + n + "_amt", "MOD " + n + " Amount", -100.0f, 100.0f, 0.0f);
        addBool  ("mod" + n + "_uni", "MOD " + n + " Unipolar", false);
    }

    return layout;
}

MultiOtoAudioProcessor::MultiOtoAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    engineCore = std::make_unique<EngineCore>();
    cacheParameterPointers();
}

// ============================================================================
//  パラメータポインタのキャッシュ (コンストラクタで 1 回だけ)
// ============================================================================
void MultiOtoAudioProcessor::cacheParameterPointers() {
    auto g = [this](const char* id) { return apvts.getRawParameterValue(id); };

    rp.totalOtt   = g("total_ott");
    rp.predriveOn = g("predrive_on");
    rp.stageOn[0] = g("s1_on");
    rp.stageOn[1] = g("s2_on");

    rp.inGain = g("in_gain");
    rp.drive  = g("drive");
    rp.odd    = g("odd_blend");
    rp.even   = g("even_blend");

    rp.xLow[0]  = g("xover_low");     rp.xHigh[0] = g("xover_high");
    rp.xLow[1]  = g("s2_xover_low");  rp.xHigh[1] = g("s2_xover_high");

    static const char* band[3] = { "l", "m", "h" };
    for (int st = 0; st < 2; ++st) {
        const juce::String sp = "s" + juce::String(st + 1) + "_";
        for (int b = 0; b < 3; ++b) {
            rp.gain [st][b] = g((sp + "gain_"  + band[b]).toRawUTF8());
            rp.depth[st][b] = g((sp + "depth_" + band[b]).toRawUTF8());
            rp.up   [st][b] = g((sp + "up_"    + band[b]).toRawUTF8());
            rp.down [st][b] = g((sp + "down_"  + band[b]).toRawUTF8());
            rp.atk  [st][b] = g((sp + "atk_"   + band[b]).toRawUTF8());
            rp.rel  [st][b] = g((sp + "rel_"   + band[b]).toRawUTF8());
        }
        rp.time[st] = g((sp + "time").toRawUTF8());
        rp.mix [st] = g((sp + "mix").toRawUTF8());
    }

    rp.postHpf      = g("post_hpf");
    rp.postLpf      = g("post_lpf");
    rp.dryWet       = g("dry_wet");
    rp.outGain      = g("out_gain");
    rp.limitCeil    = g("limit_ceil");
    rp.limitRelease = g("limit_release");
    rp.limitMode    = g("limit_mode");
    rp.phaseMode    = g("phase_mode");

    for (int i = 0; i < ModMatrix::kNumLfos; ++i) {
        const juce::String lp = "lfo" + juce::String(i + 1) + "_";
        rp.lfoWave[i]     = g((lp + "wave").toRawUTF8());
        rp.lfoSync[i]     = g((lp + "sync").toRawUTF8());
        rp.lfoSyncRate[i] = g((lp + "syncrate").toRawUTF8());
        rp.lfoRate[i]     = g((lp + "rate").toRawUTF8());
    }

    for (int i = 0; i < ModMatrix::kNumSlots; ++i) {
        const juce::String mp = "mod" + juce::String(i + 1) + "_";
        rp.modSrc[i] = g((mp + "src").toRawUTF8());
        rp.modDst[i] = g((mp + "dst").toRawUTF8());
        rp.modAmt[i] = g((mp + "amt").toRawUTF8());
        rp.modUni[i] = g((mp + "uni").toRawUTF8());
    }

    // ID の打ち間違いを開発中に確実に検出する
    jassert(rp.totalOtt && rp.phaseMode && rp.limitMode
            && rp.gain[1][2] && rp.rel[1][2] && rp.mix[1]
            && rp.lfoRate[ModMatrix::kNumLfos - 1]
            && rp.modUni[ModMatrix::kNumSlots - 1]);
}

MultiOtoAudioProcessor::~MultiOtoAudioProcessor() = default;

void MultiOtoAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    currentSampleRate = sampleRate;
    engineCore->prepare(sampleRate, samplesPerBlock);
    modMatrix.prepare(sampleRate);
    envFollowState = 0.0f;
}

// ============================================================================
//  MOD MATRIX
// ============================================================================
ModMatrix::Params MultiOtoAudioProcessor::readModParams() const {
    ModMatrix::Params mp;

    for (int i = 0; i < ModMatrix::kNumLfos; ++i) {
        auto& l = mp.lfo[static_cast<size_t>(i)];
        l.wave     = static_cast<int>(rd(rp.lfoWave[i]));
        l.sync     = rd(rp.lfoSync[i]) > 0.5f;
        l.rateSync = static_cast<int>(rd(rp.lfoSyncRate[i]));
        l.rateHz   = rd(rp.lfoRate[i]);
    }

    for (int i = 0; i < ModMatrix::kNumSlots; ++i) {
        auto& sl = mp.slot[static_cast<size_t>(i)];
        sl.src = static_cast<int>(rd(rp.modSrc[i]));
        sl.dst = static_cast<int>(rd(rp.modDst[i]));
        sl.amt = rd(rp.modAmt[i]) * 0.01f;   // % -> -1..+1
        sl.uni = rd(rp.modUni[i]) > 0.5f;
    }

    // ホストのテンポ (取得できなければ 120)
    mp.bpm = 120.0;
    if (auto* ph = const_cast<MultiOtoAudioProcessor*>(this)->getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                mp.bpm = *bpm;

    return mp;
}

void MultiOtoAudioProcessor::updateEnvFollow(const juce::AudioBuffer<float>& buffer) {
    const int n = buffer.getNumSamples();
    const int ch = juce::jmin(2, buffer.getNumChannels());
    if (n <= 0 || ch <= 0) return;

    double sum = 0.0;
    for (int c = 0; c < ch; ++c) {
        const float* d = buffer.getReadPointer(c);
        for (int i = 0; i < n; ++i) sum += static_cast<double>(d[i]) * d[i];
    }

    const float rms = static_cast<float>(std::sqrt(sum / (n * ch)));
    const float db  = 20.0f * std::log10(juce::jmax(rms, 1.0e-6f));
    const float norm = juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f);   // -60dB..0dB -> 0..1

    // ブロックレートで約 30ms の一次遅れ。カクつきを均す。
    const float blockSec = static_cast<float>(n / juce::jmax(1.0, currentSampleRate));
    const float coef = 1.0f - std::exp(-blockSec / 0.030f);
    envFollowState += juce::jlimit(0.0f, 1.0f, coef) * (norm - envFollowState);

    modMatrix.setEnvFollow(envFollowState);
}

void MultiOtoAudioProcessor::releaseResources() {
    engineCore->reset();
    modMatrix.reset();
    envFollowState = 0.0f;
}

bool MultiOtoAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    return true;
}

void MultiOtoAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    if (buffer.getNumSamples() == 0) return;
    juce::ScopedNoDenormals noDenormals;

    // MOD は「処理前の入力」を見て動く。EngineCore を通す前に更新すること。
    updateEnvFollow(buffer);
    modMatrix.processBlock(buffer.getNumSamples(), readModParams());

    EngineParams p;

    // --- パラメータ読み出し (すべてキャッシュ済みポインタ経由。文字列検索なし) ---
    p.total_ott_count = 2 << static_cast<int>(rd(rp.totalOtt));

    p.predrive_on = rd(rp.predriveOn) > 0.5f;
    p.s1_on       = rd(rp.stageOn[0]) > 0.5f;
    p.s2_on       = rd(rp.stageOn[1]) > 0.5f;

    p.inGain = rd(rp.inGain);
    p.drive  = rd(rp.drive);
    p.odd    = rd(rp.odd);
    p.even   = rd(rp.even);

    // Stage 2 の帯域分割は Stage 1 と完全に独立。
    // 2 段で異なる分割をぶつけることで、帯域の重なりから複雑な位相干渉が生まれる。
    p.xLow   = rd(rp.xLow[0]);   p.xHigh  = rd(rp.xHigh[0]);
    p.xLow2  = rd(rp.xLow[1]);   p.xHigh2 = rd(rp.xHigh[1]);

    for (int b = 0; b < 3; ++b) {
        p.s1_gain[b]  = rd(rp.gain [0][b]);  p.s2_gain[b]  = rd(rp.gain [1][b]);
        p.s1_depth[b] = rd(rp.depth[0][b]);  p.s2_depth[b] = rd(rp.depth[1][b]);
        p.s1_up[b]    = rd(rp.up   [0][b]);  p.s2_up[b]    = rd(rp.up   [1][b]);
        p.s1_down[b]  = rd(rp.down [0][b]);  p.s2_down[b]  = rd(rp.down [1][b]);
        p.s1_atk[b]   = rd(rp.atk  [0][b]);  p.s2_atk[b]   = rd(rp.atk  [1][b]);
        p.s1_rel[b]   = rd(rp.rel  [0][b]);  p.s2_rel[b]   = rd(rp.rel  [1][b]);
    }
    p.s1_time = rd(rp.time[0]);  p.s2_time = rd(rp.time[1]);
    p.s1_mix  = rd(rp.mix[0]);   p.s2_mix  = rd(rp.mix[1]);

    p.post_hpf     = rd(rp.postHpf);
    p.post_lpf     = rd(rp.postLpf);
    p.dryWet       = rd(rp.dryWet);
    p.outGain      = rd(rp.outGain);
    p.limitCeil    = rd(rp.limitCeil);
    p.limitRelease = rd(rp.limitRelease);
    p.limitMode    = static_cast<int>(rd(rp.limitMode));
    p.phase_mode   = static_cast<int>(rd(rp.phaseMode));

    // ======================================================================
    //  MOD 適用。パラメータのレンジ内へクランプしてから EngineCore へ渡す。
    //  (ベース値自体は書き換えないので、ノブの表示は動かない = 一般的な作法)
    // ======================================================================
    auto mod = [this](int dst, float base, float lo, float hi) {
        const float m = modMatrix.get(dst);
        if (std::abs(m) < 1.0e-5f) return base;
        return juce::jlimit(lo, hi,
            static_cast<float>(ModMatrix::applyModToValue(dst, base, m)));
        };

    p.s1_time = mod(ModMatrix::DstS1Time, p.s1_time, 10.0f, 1000.0f);
    p.s2_time = mod(ModMatrix::DstS2Time, p.s2_time, 10.0f, 1000.0f);
    p.s1_mix  = mod(ModMatrix::DstS1Mix,  p.s1_mix,  0.0f, 100.0f);
    p.s2_mix  = mod(ModMatrix::DstS2Mix,  p.s2_mix,  0.0f, 100.0f);

    p.xLow   = mod(ModMatrix::DstS1XLow,  p.xLow,   20.0f, 1000.0f);
    p.xHigh  = mod(ModMatrix::DstS1XHigh, p.xHigh,  1000.0f, 20000.0f);
    p.xLow2  = mod(ModMatrix::DstS2XLow,  p.xLow2,  20.0f, 1000.0f);
    p.xHigh2 = mod(ModMatrix::DstS2XHigh, p.xHigh2, 1000.0f, 20000.0f);

    for (int b = 0; b < 3; ++b) {
        p.s1_atk[b] = mod(ModMatrix::DstS1AtkL + b, p.s1_atk[b], 0.1f, 100.0f);
        p.s1_rel[b] = mod(ModMatrix::DstS1RelL + b, p.s1_rel[b], 1.0f, 1000.0f);
        p.s2_atk[b] = mod(ModMatrix::DstS2AtkL + b, p.s2_atk[b], 0.1f, 100.0f);
        p.s2_rel[b] = mod(ModMatrix::DstS2RelL + b, p.s2_rel[b], 1.0f, 1000.0f);
    }

    engineCore->updateParameters(p);
    engineCore->process(buffer);
}

juce::AudioProcessorEditor* MultiOtoAudioProcessor::createEditor() {
    return new MultiOtoAudioProcessorEditor(*this);
}

void MultiOtoAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MultiOtoAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr && xmlState->hasTagName(apvts.state.getType())) {
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));

        // ヘッダーのプリセット名表示を追従させる
        if (onPresetNameChanged)
            juce::MessageManager::callAsync(onPresetNameChanged);
    }
}

// ============================================================================
//  プリセット
// ============================================================================
juce::File MultiOtoAudioProcessor::getPresetRootDirectory() {
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("MULTI-OTO")
                   .getChildFile("Presets");

    if (!dir.exists()) dir.createDirectory();
    return dir;
}

juce::StringArray MultiOtoAudioProcessor::getUserPresetCategories() {
    juce::StringArray cats;
    const auto root = getPresetRootDirectory();
    if (!root.isDirectory()) return cats;

    for (const auto& e : juce::RangedDirectoryIterator(root, false, "*", juce::File::findDirectories))
        cats.addIfNotAlreadyThere(e.getFile().getFileName());

    cats.sort(true);
    return cats;
}

juce::File MultiOtoAudioProcessor::makePresetFile(const juce::String& category,
                                                  const juce::String& name) const {
    // ユーザー入力をそのままパスにすると "../" などで任意の場所へ書けてしまう
    const auto safeCat  = juce::File::createLegalFileName(category.trim());
    const auto safeName = juce::File::createLegalFileName(name.trim());

    auto dir = getPresetRootDirectory();
    if (safeCat.isNotEmpty()) dir = dir.getChildFile(safeCat);

    return dir.getChildFile(safeName + "." + getPresetFileExtension());
}

void MultiOtoAudioProcessor::applyParamValue(const juce::String& paramID, float realValue) {
    if (auto* p = apvts.getParameter(paramID)) {
        const auto& r = p->getNormalisableRange();
        const float v = juce::jlimit(r.start, r.end, realValue);
        p->setValueNotifyingHost(p->convertTo0to1(v));
    }
    else {
        // ID の打ち間違いを開発中に確実に見つけるため
        jassertfalse;
    }
}

bool MultiOtoAudioProcessor::isPersistentParam(const juce::String& id) {
    // 表示テーマと MOD マトリクスは「音色」ではなく「作業環境」。
    // プリセットを切り替えても組んだモジュレーションが消えないようにする。
    return id == "color_theme"
        || id.startsWith("lfo")
        || id.startsWith("mod");
}

void MultiOtoAudioProcessor::resetAllParamsToDefault() {
    for (auto* param : getParameters())
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(param))
            if (!isPersistentParam(p->paramID))
                p->setValueNotifyingHost(p->getDefaultValue());
}

void MultiOtoAudioProcessor::resetModMatrix() {
    for (auto* param : getParameters())
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(param))
            if (p->paramID.startsWith("lfo") || p->paramID.startsWith("mod"))
                p->setValueNotifyingHost(p->getDefaultValue());
}

void MultiOtoAudioProcessor::loadFactoryPreset(int index) {
    const auto& list = PresetData::getFactoryPresets();
    if (index < 0 || index >= static_cast<int>(list.size())) return;

    const auto& preset = list[static_cast<size_t>(index)];

    resetAllParamsToDefault();
    for (const auto& kv : PresetData::toParameterValues(preset))
        applyParamValue(kv.first, kv.second);

    // 推奨 OTT 数を反映する (2,4,8,...,128 → index 0..6)
    setOttCount(preset.suggestedCount);

    setCurrentPresetName(preset.name);
}

void MultiOtoAudioProcessor::setOttCount(int count) {
    int idx = 0;
    for (int c = 2; c < count && idx < 6; c *= 2) ++idx;
    if (auto* p = apvts.getParameter("total_ott"))
        p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(idx)));
}

// ============================================================================
//  RANDOM — メイン画面を「音楽的に」ランダマイズする
//
//  全パラメータを一様乱数で振ると、ほぼ確実に使い物にならない音になる。
//  本機の場合とくに致命的なのが帯域ゲインで、これは段数分だけ累乗されるため、
//  x128 で +8dB を引いてしまうと float が壊れる領域に飛ぶ。
//  そこで
//    ・GAIN は現在の OTT 段数から逆算した「1 段あたりの適正値」を中心に振る
//    ・周波数と時間は対数一様 (低い側と高い側が同じ確率で出るように)
//    ・DEPTH / UP / DN は 1 つの「強度」マクロから派生させて相関を持たせる
//    ・REL は必ず ATK より長く、高域ほど速く (本機の象徴的なスイープが出る)
//    ・Stage2 のクロスオーバーは Stage1 と必ずずらす (本機の肝)
//    ・OUT GAIN は段数に応じて自動で下げ、爆音事故を防ぐ
//  という制約を入れている。
//
//  OTT 数・PHASE MODE・リミッター設定・テーマ・MOD は「構造の選択」なので触らない。
// ============================================================================
void MultiOtoAudioProcessor::randomiseMainParameters() {
    auto& rnd = juce::Random::getSystemRandom();

    auto uni = [&rnd](float lo, float hi) { return lo + rnd.nextFloat() * (hi - lo); };
    auto logU = [&rnd](float lo, float hi) {
        const float a = std::log(lo), b = std::log(hi);
        return std::exp(a + rnd.nextFloat() * (b - a));
        };
    auto chance = [&rnd](float p) { return rnd.nextFloat() < p; };
    auto set = [this](const juce::String& id, float v) { applyParamValue(id, v); };

    // --- 段数から 1 段あたりの適正ゲインを決める ---
    const int ottIdx = juce::jlimit(0, 6, static_cast<int>(rd(rp.totalOtt)));
    static const float gainByIdx[7] = { 8.0f, 5.2f, 3.1f, 2.0f, 1.3f, 0.85f, 0.52f };
    const float gBase = gainByIdx[ottIdx];

    // --- クロスオーバー。Stage2 は Stage1 から必ず離す ---
    const float x1lo = logU(45.0f, 260.0f);
    const float x1hi = logU(1500.0f, 7000.0f);
    float x2lo = logU(45.0f, 500.0f);
    float x2hi = logU(1200.0f, 9000.0f);
    if (std::abs(std::log(x2lo / x1lo)) < 0.4f) x2lo = chance(0.5f) ? x1lo * 2.0f : x1lo * 0.5f;
    if (std::abs(std::log(x2hi / x1hi)) < 0.4f) x2hi = chance(0.5f) ? x1hi * 2.0f : x1hi * 0.5f;

    set("xover_low",     juce::jlimit(20.0f, 1000.0f, x1lo));
    set("xover_high",    juce::jlimit(1000.0f, 20000.0f, x1hi));
    set("s2_xover_low",  juce::jlimit(20.0f, 1000.0f, x2lo));
    set("s2_xover_high", juce::jlimit(1000.0f, 20000.0f, x2hi));

    // --- PRE-DRIVE ---
    const bool driveOn = chance(0.55f);
    set("predrive_on", driveOn ? 1.0f : 0.0f);
    set("in_gain",     0.0f);
    set("drive",       driveOn ? uni(20.0f, 90.0f) : 0.0f);
    set("odd_blend",   driveOn ? uni(25.0f, 100.0f) : 0.0f);
    set("even_blend",  (driveOn && chance(0.5f)) ? uni(10.0f, 70.0f) : 0.0f);

    // --- 各ステージ ---
    static const char* band[3] = { "l", "m", "h" };
    for (int st = 1; st <= 2; ++st) {
        const juce::String sp = "s" + juce::String(st) + "_";

        // ステージ全体の「効きの強さ」。ここから各バンドを派生させる
        const float intensity = uni(0.35f, 1.0f);

        // 帯域ゲイン: LOW と HIGH をやや高く、MID を低めにするのが OTT の定石
        const float gShape[3] = { uni(0.80f, 1.30f), uni(0.55f, 0.95f), uni(0.85f, 1.35f) };

        // ATK/REL: 高域ほど速く。REL は必ず ATK より長い。
        const float atkL = logU(8.0f, 80.0f);
        const float atk[3] = { atkL, atkL * uni(0.35f, 0.80f), atkL * uni(0.12f, 0.55f) };
        const float relL = logU(60.0f, 600.0f);
        const float rel[3] = { relL, relL * uni(0.40f, 1.20f), relL * uni(0.15f, 0.70f) };

        for (int b = 0; b < 3; ++b) {
            const juce::String sfx(band[b]);
            set(sp + "gain_"  + sfx, juce::jlimit(-24.0f, 24.0f, gBase * gShape[b]));
            set(sp + "depth_" + sfx, juce::jlimit(0.0f, 100.0f, intensity * 100.0f * uni(0.75f, 1.15f)));
            set(sp + "up_"    + sfx, uni(50.0f, 100.0f));
            set(sp + "down_"  + sfx, uni(60.0f, 100.0f));
            set(sp + "atk_"   + sfx, juce::jlimit(0.1f, 100.0f, atk[b]));
            set(sp + "rel_"   + sfx, juce::jlimit(1.0f, 1000.0f, juce::jmax(rel[b], atk[b] * 1.5f)));
        }

        set(sp + "time", juce::jlimit(10.0f, 1000.0f, logU(15.0f, 400.0f)));
        set(sp + "mix",  uni(50.0f, 100.0f));
        // Stage2 だけたまに切って「1 段だけ」の素直な音も出るようにする
        set(sp + "on", (st == 2 && chance(0.15f)) ? 0.0f : 1.0f);
    }

    // --- マスター ---
    set("post_hpf", logU(20.0f, 60.0f));
    set("post_lpf", logU(12000.0f, 20000.0f));
    set("dry_wet",  uni(85.0f, 100.0f));

    // 段数が多いほど下げる。カスケードの合計ゲインに追従させて爆音を防ぐ。
    set("out_gain", juce::jlimit(-24.0f, 24.0f,
                                 -(2.5f + static_cast<float>(ottIdx) * 1.6f) + uni(-1.0f, 1.0f)));

    setCurrentPresetName("RANDOM");
}

void MultiOtoAudioProcessor::resetToInit() {
    resetAllParamsToDefault();
    setCurrentPresetName({});
}

bool MultiOtoAudioProcessor::savePreset(const juce::String& category,
                                        const juce::String& name,
                                        juce::String& errorOut) {
    if (name.trim().isEmpty()) {
        errorOut = "Preset name cannot be empty.";
        return false;
    }
    if (juce::File::createLegalFileName(name.trim()).isEmpty()) {
        errorOut = "Preset name contains no usable characters.";
        return false;
    }

    const auto target = makePresetFile(category, name);

    if (!target.getParentDirectory().createDirectory()) {
        errorOut = "Could not create the preset folder.";
        return false;
    }

    juce::XmlElement xml("MultiOtoPreset");
    xml.setAttribute("version", 1);
    xml.setAttribute("plugin", MULTIOTO_VERSION);
    xml.setAttribute("name", name.trim());
    xml.setAttribute("category", category.trim());

    auto state = apvts.copyState();
    state.setProperty("presetName", name.trim(), nullptr);
    if (auto stateXml = state.createXml())
        xml.addChildElement(stateXml.release());

    if (!xml.writeTo(target)) {
        errorOut = "Could not write the preset file. Check folder permissions.";
        return false;
    }

    setCurrentPresetName(name.trim());
    return true;
}

bool MultiOtoAudioProcessor::loadPresetFile(const juce::File& presetFile, juce::String& errorOut) {
    if (!presetFile.existsAsFile()) {
        errorOut = "Preset file not found.";
        return false;
    }

    auto xml = juce::XmlDocument::parse(presetFile);
    if (xml == nullptr || !xml->hasTagName("MultiOtoPreset")) {
        errorOut = "This file is not a MULTI-OTO preset.";
        return false;
    }

    auto* stateXml = xml->getChildByName(apvts.state.getType());
    if (stateXml == nullptr) {
        errorOut = "The preset file is missing its parameter data.";
        return false;
    }

    // 表示テーマと MOD はプリセットで上書きせず、現在の設定を持ち越す。
    // replaceState は状態を丸ごと差し替えるので、正規化値で退避しておく。
    std::vector<std::pair<juce::RangedAudioParameter*, float>> keep;
    for (auto* param : getParameters())
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(param))
            if (isPersistentParam(p->paramID))
                keep.emplace_back(p, p->getValue());

    apvts.replaceState(juce::ValueTree::fromXml(*stateXml));

    for (const auto& kv : keep)
        kv.first->setValueNotifyingHost(kv.second);

    setCurrentPresetName(presetFile.getFileNameWithoutExtension());
    return true;
}

juce::String MultiOtoAudioProcessor::getCurrentPresetName() const {
    return apvts.state.getProperty("presetName", juce::String()).toString();
}

void MultiOtoAudioProcessor::setCurrentPresetName(const juce::String& n) {
    apvts.state.setProperty("presetName", n, nullptr);

    // setStateInformation はメッセージスレッド以外から呼ばれ得るので必ず投げ直す
    if (onPresetNameChanged)
        juce::MessageManager::callAsync(onPresetNameChanged);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new MultiOtoAudioProcessor();
}
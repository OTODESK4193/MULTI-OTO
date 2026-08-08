#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DSP/EngineCore.h"
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

    return layout;
}

MultiOtoAudioProcessor::MultiOtoAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    engineCore = std::make_unique<EngineCore>();
}

MultiOtoAudioProcessor::~MultiOtoAudioProcessor() = default;

void MultiOtoAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    engineCore->prepare(sampleRate, samplesPerBlock);
}

void MultiOtoAudioProcessor::releaseResources() {
    engineCore->reset();
}

bool MultiOtoAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    return true;
}

void MultiOtoAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    if (buffer.getNumSamples() == 0) return;
    juce::ScopedNoDenormals noDenormals;

    EngineParams p;

    int ottIdx = static_cast<int>(apvts.getRawParameterValue("total_ott")->load(std::memory_order_relaxed));
    p.total_ott_count = 2 << ottIdx;

    p.predrive_on = apvts.getRawParameterValue("predrive_on")->load(std::memory_order_relaxed) > 0.5f;
    p.s1_on = apvts.getRawParameterValue("s1_on")->load(std::memory_order_relaxed) > 0.5f;
    p.s2_on = apvts.getRawParameterValue("s2_on")->load(std::memory_order_relaxed) > 0.5f;

    p.inGain = apvts.getRawParameterValue("in_gain")->load(std::memory_order_relaxed);
    p.drive = apvts.getRawParameterValue("drive")->load(std::memory_order_relaxed);
    p.odd = apvts.getRawParameterValue("odd_blend")->load(std::memory_order_relaxed);
    p.even = apvts.getRawParameterValue("even_blend")->load(std::memory_order_relaxed);
    p.xLow = apvts.getRawParameterValue("xover_low")->load(std::memory_order_relaxed);
    p.xHigh = apvts.getRawParameterValue("xover_high")->load(std::memory_order_relaxed);

    // Stage 2 の帯域分割は Stage 1 と完全に独立。
    // 2 段で異なる分割をぶつけることで、帯域の重なりから複雑な位相干渉が生まれる。
    p.xLow2  = apvts.getRawParameterValue("s2_xover_low")->load(std::memory_order_relaxed);
    p.xHigh2 = apvts.getRawParameterValue("s2_xover_high")->load(std::memory_order_relaxed);

    p.s1_gain[0] = apvts.getRawParameterValue("s1_gain_l")->load(std::memory_order_relaxed);
    p.s1_gain[1] = apvts.getRawParameterValue("s1_gain_m")->load(std::memory_order_relaxed);
    p.s1_gain[2] = apvts.getRawParameterValue("s1_gain_h")->load(std::memory_order_relaxed);
    p.s1_depth[0] = apvts.getRawParameterValue("s1_depth_l")->load(std::memory_order_relaxed);
    p.s1_depth[1] = apvts.getRawParameterValue("s1_depth_m")->load(std::memory_order_relaxed);
    p.s1_depth[2] = apvts.getRawParameterValue("s1_depth_h")->load(std::memory_order_relaxed);

    p.s1_up[0] = apvts.getRawParameterValue("s1_up_l")->load(std::memory_order_relaxed);
    p.s1_up[1] = apvts.getRawParameterValue("s1_up_m")->load(std::memory_order_relaxed);
    p.s1_up[2] = apvts.getRawParameterValue("s1_up_h")->load(std::memory_order_relaxed);
    p.s1_down[0] = apvts.getRawParameterValue("s1_down_l")->load(std::memory_order_relaxed);
    p.s1_down[1] = apvts.getRawParameterValue("s1_down_m")->load(std::memory_order_relaxed);
    p.s1_down[2] = apvts.getRawParameterValue("s1_down_h")->load(std::memory_order_relaxed);

    p.s1_time = apvts.getRawParameterValue("s1_time")->load(std::memory_order_relaxed);
    p.s1_mix = apvts.getRawParameterValue("s1_mix")->load(std::memory_order_relaxed);
    p.s1_atk[0] = apvts.getRawParameterValue("s1_atk_l")->load(std::memory_order_relaxed);
    p.s1_atk[1] = apvts.getRawParameterValue("s1_atk_m")->load(std::memory_order_relaxed);
    p.s1_atk[2] = apvts.getRawParameterValue("s1_atk_h")->load(std::memory_order_relaxed);
    p.s1_rel[0] = apvts.getRawParameterValue("s1_rel_l")->load(std::memory_order_relaxed);
    p.s1_rel[1] = apvts.getRawParameterValue("s1_rel_m")->load(std::memory_order_relaxed);
    p.s1_rel[2] = apvts.getRawParameterValue("s1_rel_h")->load(std::memory_order_relaxed);

    p.s2_gain[0] = apvts.getRawParameterValue("s2_gain_l")->load(std::memory_order_relaxed);
    p.s2_gain[1] = apvts.getRawParameterValue("s2_gain_m")->load(std::memory_order_relaxed);
    p.s2_gain[2] = apvts.getRawParameterValue("s2_gain_h")->load(std::memory_order_relaxed);
    p.s2_depth[0] = apvts.getRawParameterValue("s2_depth_l")->load(std::memory_order_relaxed);
    p.s2_depth[1] = apvts.getRawParameterValue("s2_depth_m")->load(std::memory_order_relaxed);
    p.s2_depth[2] = apvts.getRawParameterValue("s2_depth_h")->load(std::memory_order_relaxed);

    p.s2_up[0] = apvts.getRawParameterValue("s2_up_l")->load(std::memory_order_relaxed);
    p.s2_up[1] = apvts.getRawParameterValue("s2_up_m")->load(std::memory_order_relaxed);
    p.s2_up[2] = apvts.getRawParameterValue("s2_up_h")->load(std::memory_order_relaxed);
    p.s2_down[0] = apvts.getRawParameterValue("s2_down_l")->load(std::memory_order_relaxed);
    p.s2_down[1] = apvts.getRawParameterValue("s2_down_m")->load(std::memory_order_relaxed);
    p.s2_down[2] = apvts.getRawParameterValue("s2_down_h")->load(std::memory_order_relaxed);
    p.s2_time = apvts.getRawParameterValue("s2_time")->load(std::memory_order_relaxed);
    p.s2_mix = apvts.getRawParameterValue("s2_mix")->load(std::memory_order_relaxed);
    p.s2_atk[0] = apvts.getRawParameterValue("s2_atk_l")->load(std::memory_order_relaxed);
    p.s2_atk[1] = apvts.getRawParameterValue("s2_atk_m")->load(std::memory_order_relaxed);
    p.s2_atk[2] = apvts.getRawParameterValue("s2_atk_h")->load(std::memory_order_relaxed);
    p.s2_rel[0] = apvts.getRawParameterValue("s2_rel_l")->load(std::memory_order_relaxed);
    p.s2_rel[1] = apvts.getRawParameterValue("s2_rel_m")->load(std::memory_order_relaxed);
    p.s2_rel[2] = apvts.getRawParameterValue("s2_rel_h")->load(std::memory_order_relaxed);

    p.post_hpf = apvts.getRawParameterValue("post_hpf")->load(std::memory_order_relaxed);
    p.post_lpf = apvts.getRawParameterValue("post_lpf")->load(std::memory_order_relaxed);
    p.dryWet = apvts.getRawParameterValue("dry_wet")->load(std::memory_order_relaxed);
    p.outGain = apvts.getRawParameterValue("out_gain")->load(std::memory_order_relaxed);
    p.limitCeil = apvts.getRawParameterValue("limit_ceil")->load(std::memory_order_relaxed);
    p.limitRelease = apvts.getRawParameterValue("limit_release")->load(std::memory_order_relaxed);
    p.limitMode = static_cast<int>(apvts.getRawParameterValue("limit_mode")->load(std::memory_order_relaxed));
    p.phase_mode = static_cast<int>(apvts.getRawParameterValue("phase_mode")->load(std::memory_order_relaxed));

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

void MultiOtoAudioProcessor::resetAllParamsToDefault() {
    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(param))
            if (rp->paramID != "color_theme")   // 表示テーマはユーザーの好みなので維持
                rp->setValueNotifyingHost(rp->getDefaultValue());
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

    // 表示テーマはプリセットに含めず、ユーザーの好みを維持する
    float keepTheme = 0.0f;
    if (auto* th = apvts.getRawParameterValue("color_theme"))
        keepTheme = th->load();

    apvts.replaceState(juce::ValueTree::fromXml(*stateXml));

    if (auto* themeParam = apvts.getParameter("color_theme"))
        themeParam->setValueNotifyingHost(themeParam->convertTo0to1(keepTheme));

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
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DSP/EngineCore.h"

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

    juce::StringArray ottChoices = { "2", "4", "8", "16", "32", "64" };
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("total_ott", 1), "Total OTT Count", ottChoices, 0));

    addBool("predrive_on", "PreDrive Bypass", true);
    addBool("s1_on", "Stage 1 Bypass", true);
    addBool("s2_on", "Stage 2 Bypass", true);

    addFloat("in_gain", "Input Gain", -24.0f, 24.0f, 0.0f);
    addFloat("drive", "Drive Amount", 0.0f, 100.0f, 0.0f);
    addFloat("odd_blend", "Odd Harmonics", 0.0f, 100.0f, 50.0f);
    addFloat("even_blend", "Even Harmonics", 0.0f, 100.0f, 50.0f);

    addFreq("xover_low", "Low Freq", 20.0f, 1000.0f, 88.0f);
    addFreq("xover_high", "High Freq", 1000.0f, 20000.0f, 2500.0f);

    auto buildStageParams = [&](int s) {
        juce::String st = juce::String(s);
        addFloat("s" + st + "_gain_h", "S" + st + " High Gain", -24.0f, 24.0f, 0.0f);
        addFloat("s" + st + "_gain_m", "S" + st + " Mid Gain", -24.0f, 24.0f, 0.0f);
        addFloat("s" + st + "_gain_l", "S" + st + " Low Gain", -24.0f, 24.0f, 0.0f);
        addFloat("s" + st + "_depth_h", "S" + st + " High Depth", 0.0f, 100.0f, 100.0f);
        addFloat("s" + st + "_depth_m", "S" + st + " Mid Depth", 0.0f, 100.0f, 100.0f);
        addFloat("s" + st + "_depth_l", "S" + st + " Low Depth", 0.0f, 100.0f, 100.0f);
        addFloat("s" + st + "_time", "S" + st + " Macro Time", 10.0f, 1000.0f, 100.0f);
        addFloat("s" + st + "_mix", "S" + st + " Mix", 0.0f, 100.0f, 100.0f);

        // 【最重要】Ableton OTTの純正タイム設定へ変更
        addFloat("s" + st + "_atk_h", "S" + st + " High Attack", 0.1f, 100.0f, 14.5f);
        addFloat("s" + st + "_atk_m", "S" + st + " Mid Attack", 0.1f, 100.0f, 26.5f);
        addFloat("s" + st + "_atk_l", "S" + st + " Low Attack", 0.1f, 100.0f, 57.0f);
        addFloat("s" + st + "_rel_h", "S" + st + " High Release", 1.0f, 1000.0f, 41.5f);
        addFloat("s" + st + "_rel_m", "S" + st + " Mid Release", 1.0f, 1000.0f, 119.0f);
        addFloat("s" + st + "_rel_l", "S" + st + " Low Release", 1.0f, 1000.0f, 249.0f);
        };

    buildStageParams(1);
    buildStageParams(2);

    addFreq("post_hpf", "Post HPF", 20.0f, 1000.0f, 20.0f);
    addFreq("post_lpf", "Post LPF", 1000.0f, 20000.0f, 20000.0f);

    addFloat("dry_wet", "Dry / Wet", 0.0f, 100.0f, 100.0f);
    addFloat("out_gain", "Output Gain", -24.0f, 24.0f, 0.0f);
    addFloat("limit_ceil", "Limiter Ceiling", -2.0f, -0.1f, -0.1f);

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

    p.s1_gain[0] = apvts.getRawParameterValue("s1_gain_l")->load(std::memory_order_relaxed);
    p.s1_gain[1] = apvts.getRawParameterValue("s1_gain_m")->load(std::memory_order_relaxed);
    p.s1_gain[2] = apvts.getRawParameterValue("s1_gain_h")->load(std::memory_order_relaxed);
    p.s1_depth[0] = apvts.getRawParameterValue("s1_depth_l")->load(std::memory_order_relaxed);
    p.s1_depth[1] = apvts.getRawParameterValue("s1_depth_m")->load(std::memory_order_relaxed);
    p.s1_depth[2] = apvts.getRawParameterValue("s1_depth_h")->load(std::memory_order_relaxed);
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
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new MultiOtoAudioProcessor();
}
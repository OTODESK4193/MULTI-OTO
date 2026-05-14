#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DSP/EngineCore.h"

juce::AudioProcessorValueTreeState::ParameterLayout MultiOtoAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto addFloat = [&](const juce::String& id, const juce::String& name, float min, float max, float def) {
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(id, 1), name, juce::NormalisableRange<float>(min, max, 0.1f), def));
        };

    auto addFreq = [&](const juce::String& id, const juce::String& name, float min, float max, float def) {
        auto range = juce::NormalisableRange<float>(min, max, 1.0f);
        range.setSkewForCentre(std::sqrt(min * max));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(id, 1), name, range, def));
        };

    addFloat("total_ott", "Total OTT Count", 1.0f, 64.0f, 2.0f);
    addFloat("in_gain", "Input Gain", -24.0f, 24.0f, 0.0f);
    addFloat("drive", "Drive Amount", 0.0f, 100.0f, 0.0f);
    addFloat("odd_blend", "Odd Harmonics", 0.0f, 100.0f, 50.0f);
    addFloat("even_blend", "Even Harmonics", 0.0f, 100.0f, 50.0f);

    addFreq("xover_low", "Low Freq", 20.0f, 1000.0f, 88.0f);
    addFreq("xover_high", "High Freq", 1000.0f, 20000.0f, 2500.0f);

    addFloat("s1_gain_h", "S1 High Gain", -24.0f, 24.0f, 0.0f);
    addFloat("s1_gain_m", "S1 Mid Gain", -24.0f, 24.0f, 0.0f);
    addFloat("s1_gain_l", "S1 Low Gain", -24.0f, 24.0f, 0.0f);
    addFloat("s1_depth_h", "S1 High Depth", 0.0f, 100.0f, 100.0f);
    addFloat("s1_depth_m", "S1 Mid Depth", 0.0f, 100.0f, 100.0f);
    addFloat("s1_depth_l", "S1 Low Depth", 0.0f, 100.0f, 100.0f);
    addFloat("s1_time", "S1 Macro Time", 10.0f, 1000.0f, 100.0f);
    addFloat("s1_atk_h", "S1 High Attack", 0.1f, 100.0f, 5.0f);
    addFloat("s1_atk_m", "S1 Mid Attack", 0.1f, 100.0f, 5.0f);
    addFloat("s1_atk_l", "S1 Low Attack", 0.1f, 100.0f, 5.0f);
    addFloat("s1_rel_h", "S1 High Release", 1.0f, 1000.0f, 50.0f);
    addFloat("s1_rel_m", "S1 Mid Release", 1.0f, 1000.0f, 50.0f);
    addFloat("s1_rel_l", "S1 Low Release", 1.0f, 1000.0f, 50.0f);

    addFloat("s2_gain_h", "S2 High Gain", -24.0f, 24.0f, 0.0f);
    addFloat("s2_gain_m", "S2 Mid Gain", -24.0f, 24.0f, 0.0f);
    addFloat("s2_gain_l", "S2 Low Gain", -24.0f, 24.0f, 0.0f);
    addFloat("s2_depth_h", "S2 High Depth", 0.0f, 100.0f, 100.0f);
    addFloat("s2_depth_m", "S2 Mid Depth", 0.0f, 100.0f, 100.0f);
    addFloat("s2_depth_l", "S2 Low Depth", 0.0f, 100.0f, 100.0f);
    addFloat("s2_time", "S2 Macro Time", 10.0f, 1000.0f, 100.0f);
    addFloat("s2_atk_h", "S2 High Attack", 0.1f, 100.0f, 5.0f);
    addFloat("s2_atk_m", "S2 Mid Attack", 0.1f, 100.0f, 5.0f);
    addFloat("s2_atk_l", "S2 Low Attack", 0.1f, 100.0f, 5.0f);
    addFloat("s2_rel_h", "S2 High Release", 1.0f, 1000.0f, 50.0f);
    addFloat("s2_rel_m", "S2 Mid Release", 1.0f, 1000.0f, 50.0f);
    addFloat("s2_rel_l", "S2 Low Release", 1.0f, 1000.0f, 50.0f);

    addFreq("post_hpf", "Post HPF", 20.0f, 1000.0f, 20.0f);
    addFreq("post_lpf", "Post LPF", 1000.0f, 20000.0f, 20000.0f);

    addFloat("dry_wet", "Dry / Wet", 0.0f, 100.0f, 100.0f);
    addFloat("out_gain", "Output Gain", -24.0f, 24.0f, 0.0f);
    addFloat("limit_ceil", "Limiter Ceiling", -2.0f, -0.1f, -0.1f);

    juce::StringArray phaseChoices = { "Color", "Align" };
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("phase_mode", 1), "Phase Mode", phaseChoices, 0));

    return layout;
}

MultiOtoAudioProcessor::MultiOtoAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    engineCore = std::make_unique<EngineCore>();
}

MultiOtoAudioProcessor::~MultiOtoAudioProcessor() = default;

void MultiOtoAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    currentSampleRate = sampleRate;
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
    if (currentSampleRate == 0.0 || buffer.getNumSamples() == 0 || std::abs(getSampleRate() - currentSampleRate) > 0.1) {
        buffer.clear();
        return;
    }

    juce::ScopedNoDenormals noDenormals;

    EngineParams p;
    p.total_ott = static_cast<int>(apvts.getRawParameterValue("total_ott")->load(std::memory_order_relaxed));
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
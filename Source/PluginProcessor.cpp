#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DSP/EngineCore.h"

MultiOtoAudioProcessor::MultiOtoAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
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

void MultiOtoAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals; // 非正規化数によるCPUスパイク防止
    engineCore->process(buffer);
}

juce::AudioProcessorEditor* MultiOtoAudioProcessor::createEditor() {
    return new MultiOtoAudioProcessorEditor(*this);
}

// JUCEプラグインのエントリーポイント
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new MultiOtoAudioProcessor();
}
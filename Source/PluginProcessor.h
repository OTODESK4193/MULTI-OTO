#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>

class EngineCore;

class MultiOtoAudioProcessor : public juce::AudioProcessor {
public:
    MultiOtoAudioProcessor();
    ~MultiOtoAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MULTI-OTO"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    /** GUI がメーター値を読み取るためのアクセサ */
    EngineCore* getEngineCore() const { return engineCore.get(); }

    // ======================================================================
    //  プリセット
    //  ・FACTORY はバイナリ内蔵 (PresetData.h)。ディスクには書き出さない。
    //  ・ユーザープリセットは %APPDATA%\MULTI-OTO\Presets\<Category>\<Name>.motopreset
    //  ・OTT COUNT (total_ott) はプリセットに含めず、常に現在値を維持する。
    // ======================================================================
    static juce::File        getPresetRootDirectory();
    static juce::StringArray getUserPresetCategories();
    static juce::String      getPresetFileExtension() { return "motopreset"; }

    juce::File makePresetFile(const juce::String& category, const juce::String& name) const;

    bool savePreset(const juce::String& category, const juce::String& name, juce::String& errorOut);
    bool loadPresetFile(const juce::File& presetFile, juce::String& errorOut);
    void loadFactoryPreset(int index);
    void resetToInit();

    /** 2,4,8,...,128 の実数を total_ott のインデックスへ変換して設定する */
    void setOttCount(int count);

    juce::String getCurrentPresetName() const;
    void         setCurrentPresetName(const juce::String& n);

    /** GUI へ「プリセット名が変わった」ことを知らせるコールバック */
    std::function<void()> onPresetNameChanged;

private:
    std::unique_ptr<EngineCore> engineCore;
    double currentSampleRate = 0.0; // Ableton Live Fail-safe

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    /** 1 パラメータへ実値 (正規化前) を書き込む */
    void applyParamValue(const juce::String& paramID, float realValue);
    /** total_ott を除く全パラメータを既定値へ戻す */
    void resetAllParamsToDefault();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiOtoAudioProcessor)
};

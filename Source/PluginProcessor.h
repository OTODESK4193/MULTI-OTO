#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/ModMatrix.h"
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
    /** アップワード・コンプは入力が止まったあとも長く鳴り続ける。
        0 を返すとオフラインバウンスで尾を切られるため、余裕を持って申告する。 */
    double getTailLengthSeconds() const override { return 4.0; }

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

    /** GUI が LFO の現在値を読むためのアクセサ (ロックフリー) */
    const ModMatrix& getModMatrix() const { return modMatrix; }

    // ======================================================================
    //  プリセット
    //  ・FACTORY はバイナリ内蔵 (PresetData.h)。ディスクには書き出さない。
    //  ・ユーザープリセットは %APPDATA%\MULTI-OTO\Presets\<Category>\<Name>.motopreset
    //  ・プリセットは OTT COUNT も含めて全パラメータを復元する。
    //    表示テーマ (color_theme) だけはユーザーの好みとして保持する。
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

    ModMatrix modMatrix;
    float envFollowState = 0.0f;

    // ======================================================================
    //  パラメータポインタのキャッシュ
    //
    //  APVTS::getRawParameterValue() は StringRef をキーにした std::map 検索で、
    //  1 ブロックに 120 回以上引くのは無駄が大きい。さらに MOD の ID は
    //  "lfo" + n + "_wave" のように実行時に juce::String を組み立てていたため、
    //  オーディオスレッドでヒープ確保が起きていた (リアルタイム安全性の違反)。
    //  コンストラクタで一度だけ引いて、以降はポインタ参照だけで済ませる。
    // ======================================================================
    struct RawParams
    {
        std::atomic<float>* totalOtt   = nullptr;
        std::atomic<float>* predriveOn = nullptr;
        std::atomic<float>* stageOn[2] = {};

        std::atomic<float>* inGain = nullptr;
        std::atomic<float>* drive  = nullptr;
        std::atomic<float>* odd    = nullptr;
        std::atomic<float>* even   = nullptr;

        std::atomic<float>* xLow[2]  = {};
        std::atomic<float>* xHigh[2] = {};

        std::atomic<float>* gain[2][3]  = {};
        std::atomic<float>* depth[2][3] = {};
        std::atomic<float>* up[2][3]    = {};
        std::atomic<float>* down[2][3]  = {};
        std::atomic<float>* atk[2][3]   = {};
        std::atomic<float>* rel[2][3]   = {};
        std::atomic<float>* time[2]     = {};
        std::atomic<float>* mix[2]      = {};

        std::atomic<float>* postHpf      = nullptr;
        std::atomic<float>* postLpf      = nullptr;
        std::atomic<float>* dryWet       = nullptr;
        std::atomic<float>* outGain      = nullptr;
        std::atomic<float>* limitCeil    = nullptr;
        std::atomic<float>* limitRelease = nullptr;
        std::atomic<float>* limitMode    = nullptr;
        std::atomic<float>* phaseMode    = nullptr;

        std::atomic<float>* lfoWave[ModMatrix::kNumLfos]     = {};
        std::atomic<float>* lfoSync[ModMatrix::kNumLfos]     = {};
        std::atomic<float>* lfoSyncRate[ModMatrix::kNumLfos] = {};
        std::atomic<float>* lfoRate[ModMatrix::kNumLfos]     = {};

        std::atomic<float>* modSrc[ModMatrix::kNumSlots] = {};
        std::atomic<float>* modDst[ModMatrix::kNumSlots] = {};
        std::atomic<float>* modAmt[ModMatrix::kNumSlots] = {};
        std::atomic<float>* modUni[ModMatrix::kNumSlots] = {};
    };
    RawParams rp;

    void cacheParameterPointers();

    /** null 安全な読み出し。キャッシュ漏れがあっても落ちない。 */
    static float rd(const std::atomic<float>* p) noexcept
    {
        return (p != nullptr) ? p->load(std::memory_order_relaxed) : 0.0f;
    }

    /** APVTS から MOD の設定を読み出す (文字列を作らない) */
    ModMatrix::Params readModParams() const;
    /** 入力レベルを検出して ENV FOLLOW ソースへ渡す */
    void updateEnvFollow(const juce::AudioBuffer<float>& buffer);

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    /** 1 パラメータへ実値 (正規化前) を書き込む */
    void applyParamValue(const juce::String& paramID, float realValue);
    /** color_theme を除く全パラメータを既定値へ戻す */
    void resetAllParamsToDefault();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiOtoAudioProcessor)
};

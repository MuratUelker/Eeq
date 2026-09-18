#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/Equalizer.h"
#include "DSP/SpectrumAnalyzer.h"
#include <deque>

struct EQSnapshot
{
    std::array<float, MAX_BANDS> freqs{};
    std::array<float, MAX_BANDS> gains{};
    std::array<float, MAX_BANDS> qs{};
    std::array<int, MAX_BANDS> types{};
    std::array<int, MAX_BANDS> slopes{};
    std::array<bool, MAX_BANDS> actives{};
    std::array<int, MAX_BANDS> channelModes{};
    std::array<bool, MAX_BANDS> dynEnabled{};
    std::array<float, MAX_BANDS> dynRange{};
    std::array<float, MAX_BANDS> dynThreshold{};
    std::array<bool, MAX_BANDS> scTriggers{};
    std::array<bool, MAX_BANDS> solos{};
    std::array<bool, MAX_BANDS> bypasses{};
    float gainScale = 1.0f;
};

struct CollisionData {
    float peakFreq = -1.0f;
    float peakGain = -100.0f;
    bool detected = false;
};

class EeqProcessor : public juce::AudioProcessor
{
public:
    EeqProcessor();
    ~EeqProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    int getLatencySamples() const;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }

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

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    Equalizer& getEqualizer() { return equalizer; }
    SpectrumAnalyzer& getSpectrumAnalyzer() { return spectrum; }
    SpectrumAnalyzer& getSidechainSpectrum() { return sidechainSpectrum; }

    // Processing modes
    ProcessingMode getProcessingMode() const { return currentMode; }
    void setProcessingMode(ProcessingMode mode) { currentMode = mode; }

    // Linear Phase Resolution
    LinearPhaseResolution getLinearPhaseResolution() const { return lpResolution; }
    void setLinearPhaseResolution(LinearPhaseResolution res) { lpResolution = res; equalizer.setLinearPhaseResolution(res); }

    // Natural Phase Resolution
    NaturalPhaseResolution getNaturalPhaseResolution() const { return npResolution; }
    void setNaturalPhaseResolution(NaturalPhaseResolution res) { npResolution = res; equalizer.setNaturalPhaseResolution(res); }

    // Phase Invert
    bool isPhaseInverted() const { return phaseInverted; }
    void setPhaseInverted(bool inv) { phaseInverted = inv; }

    // Auto Gain
    bool isAutoGainEnabled() const { return autoGainEnabled; }
    void setAutoGainEnabled(bool enabled) { autoGainEnabled = enabled; }

    // Advanced Auto Gain
    bool isAutoGainAdvanced() const { return autoGainAdvanced; }
    void setAutoGainAdvanced(bool enabled) { autoGainAdvanced = enabled; }
    float getAutoGainChannelWeight() const { return autoGainChannelWeight; }
    void setAutoGainChannelWeight(float weight) { autoGainChannelWeight = juce::jlimit(0.0f, 1.0f, weight); }

    // Output Pan
    float getOutputPan() const { return outputPan; }
    void setOutputPan(float pan) { outputPan = pan; }

    // Undo/Redo
    void pushUndoState();
    void undo();
    void redo();
    bool canUndo() const { return !undoStack.empty(); }
    bool canRedo() const { return !redoStack.empty(); }

    // A/B Comparison
    void copyAtoB() { stateB = captureState(); }
    void copyBtoA() { stateA = captureState(); applyState(stateB); }
    void switchToA() { applyState(stateA); }
    void switchToB() { applyState(stateB); }
    bool isStateBActive() const { return stateBActive; }
    void setStateBActive(bool active) { stateBActive = active; }

    static constexpr int MAX_BANDS = Equalizer::NUM_BANDS;

    juce::String getBandParamId(int band, const juce::String& suffix) const;

    // Sidechain
    bool hasSidechain() const { return true; }

    // EQ Match
    void applyEQMatch();
    void applyEQMatchExternal();
    bool isEQMatchActive() const { return eqMatchActive; }
    void setEQMatchActive(bool active) { eqMatchActive = active; }

    // Gain scale
    float getGainScale() const { return gainScale; }
    void setGainScale(float scale) { gainScale = scale; }

    // Display range
    float getDisplayRange() const { return displayRange; }
    void setDisplayRange(float range) { displayRange = range; }

    // Output meter
    float getOutputLevelL() const { return outputLevelL; }
    float getOutputLevelR() const { return outputLevelR; }

    // Sidechain Filter
    float getSCFilterFreq() const { return scFilterFreq; }
    void setSCFilterFreq(float freq) { scFilterFreq = juce::jlimit(20.0f, 22000.0f, freq); }
    float getSCFilterQ() const { return scFilterQ; }
    void setSCFilterQ(float q) { scFilterQ = juce::jlimit(0.1f, 10.0f, q); }
    int getSCFilterType() const { return scFilterType; }
    void setSCFilterType(int type) { scFilterType = juce::jlimit(0, 2, type); }
    bool isSCFilterEnabled() const { return scFilterEnabled; }
    void setSCFilterEnabled(bool enabled) { scFilterEnabled = enabled; }

    // MIDI Learn
    struct MidiMapping { int band; juce::String param; int cc; int channel; };
    void setMidiLearnActive(bool active);
    bool isMidiLearnActive() const { return midiLearnActive; }
    void setMidiLearnTarget(int band, const juce::String& param);
    void applyMidiMapping(const MidiMapping& mapping, float value);
    void addMidiMapping(int band, const juce::String& param, int cc, int channel);
    void clearMidiMappings();
    const std::vector<MidiMapping>& getMidiMappings() const { return midiMappings; }

    // User presets
    void saveUserPreset(const juce::String& name);
    void deleteUserPreset(const juce::String& name);
    juce::StringArray getUserPresetNames() const;
    void loadUserPreset(const juce::String& name);
    juce::File getUserPresetFolder() const;

    // Band clipboard
    void copyBandToClipboard(int bandIndex);
    void pasteBandFromClipboard(int bandIndex);

private:
    Equalizer equalizer;
    SpectrumAnalyzer spectrum;
    SpectrumAnalyzer sidechainSpectrum;
    BiquadFilter scFilterL;
    BiquadFilter scFilterR;
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    double currentSampleRate = 44100.0;
    ProcessingMode currentMode = ProcessingMode::ZeroLatency;
    LinearPhaseResolution lpResolution = LinearPhaseResolution::High;
    NaturalPhaseResolution npResolution = NaturalPhaseResolution::High;
    bool phaseInverted = false;
    bool autoGainEnabled = true;
    bool autoGainAdvanced = false;
    float autoGainChannelWeight = 0.5f; // 0.0 = side only, 0.5 = equal, 1.0 = mid only
    float outputPan = 0.0f;
    float gainScale = 1.0f;
    float displayRange = 30.0f;
    float outputLevelL = -60.0f;
    float outputLevelR = -60.0f;
    bool eqMatchActive = false;

    // Sidechain Filter
    float scFilterFreq = 1000.0f;
    float scFilterQ = 0.707f;
    int scFilterType = 0; // 0=Bell, 1=LowCut, 2=HighCut
    bool scFilterEnabled = false;

    // MIDI Learn
    bool midiLearnActive = false;
    int selectedBandForMidiLearn = -1;
    juce::String selectedParamForMidiLearn;
    std::vector<MidiMapping> midiMappings;

    // Undo/Redo
    static constexpr int MAX_UNDO = 50;
    std::deque<EQSnapshot> undoStack;
    std::deque<EQSnapshot> redoStack;

    // A/B
    EQSnapshot stateA{};
    EQSnapshot stateB{};
    bool stateBActive = false;

    // Collision detection
    CollisionData collisionData;

    // Band clipboard
    struct ClipboardBand
    {
        float freq = 1000.0f;
        float gain = 0.0f;
        float q = 0.707f;
        int type = 0;
        bool active = false;
        int channelMode = 0;
        bool dynamicEnabled = false;
        float dynRange = 0.0f;
        float dynThreshold = -20.0f;
        bool dynAutoThreshold = true;
        bool scTrigger = false;
        int slope = 3;
        bool phaseInverted = false;
        bool bypassed = false;
        bool soloed = false;
    };
    ClipboardBand clipboardBand;
    bool clipboardValid = false;

    // Instance List / Inter-plugin communication
    struct InstanceInfo {
        uintptr_t instanceId = 0;
        juce::String name;
        std::array<float, 4096> spectrum{};
        bool hasSpectrum = false;
        bool isVisible = true;
        float peakFreq = -1.0f;
        float peakGain = -100.0f;
    };

public:
    // Startup / diagnostics logging (~/Library/Logs/Eeq.log)
    static juce::File getLogFile();
    static void writeLog(const juce::String& message);

    static std::vector<InstanceInfo*>& getInstanceList();
    void registerInstance();
    void unregisterInstance();
    void broadcastSpectrum(const std::array<float, 4096>& spectrum);
    void setInstanceName(const juce::String& name);
    const juce::String& getInstanceName() const;
    const std::vector<InstanceInfo*>& getVisibleInstances() const;

private:
    // Collision detection
    void detectCollision(const std::array<float, 4096>& spectrum);
    bool hasSpectrum() const;
    const CollisionData& getCollisionData() const { return collisionData; }

private:
    EQSnapshot captureState();
    void applyState(const EQSnapshot& state);
    void saveStateToFile();
    void loadStateFromFile();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EeqProcessor)
};

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

    // Phase Invert
    bool isPhaseInverted() const { return phaseInverted; }
    void setPhaseInverted(bool inv) { phaseInverted = inv; }

    // Auto Gain
    bool isAutoGainEnabled() const { return autoGainEnabled; }
    void setAutoGainEnabled(bool enabled) { autoGainEnabled = enabled; }

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

private:
    Equalizer equalizer;
    SpectrumAnalyzer spectrum;
    SpectrumAnalyzer sidechainSpectrum;
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    double currentSampleRate = 44100.0;
    ProcessingMode currentMode = ProcessingMode::ZeroLatency;
    LinearPhaseResolution lpResolution = LinearPhaseResolution::High;
    bool phaseInverted = false;
    bool autoGainEnabled = true;
    float outputPan = 0.0f;
    float gainScale = 1.0f;
    float displayRange = 30.0f;
    float outputLevelL = -60.0f;
    float outputLevelR = -60.0f;
    bool eqMatchActive = false;

    // Undo/Redo
    static constexpr int MAX_UNDO = 50;
    std::deque<EQSnapshot> undoStack;
    std::deque<EQSnapshot> redoStack;

    // A/B
    EQSnapshot stateA{};
    EQSnapshot stateB{};
    bool stateBActive = false;

    EQSnapshot captureState();
    void applyState(const EQSnapshot& state);
    void saveStateToFile();
    void loadStateFromFile();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EeqProcessor)
};

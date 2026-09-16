#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/Equalizer.h"
#include "DSP/SpectrumAnalyzer.h"

class EeqProcessor : public juce::AudioProcessor
{
public:
    EeqProcessor();
    ~EeqProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

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

    Equalizer& getEqualizer() { return equalizer; }
    SpectrumAnalyzer& getSpectrumAnalyzer() { return spectrum; }

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    static constexpr int NUM_PARAMS_PER_BAND = 4;
    static constexpr int MAX_BANDS = Equalizer::NUM_BANDS;

    juce::String getBandParamId(int band, const juce::String& suffix) const;

private:
    Equalizer equalizer;
    SpectrumAnalyzer spectrum;
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EeqProcessor)
};

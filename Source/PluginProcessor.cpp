#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::String EeqProcessor::getBandParamId(int band, const juce::String& suffix) const
{
    return "band" + juce::String(band + 1) + "_" + suffix;
}

juce::AudioProcessorValueTreeState::ParameterLayout EeqProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const juce::StringArray filterTypes = {
        "Bell", "LowShelf", "HighShelf", "LowCut", "HighCut", "Notch", "BandPass", "FlatTilt"
    };

    for (int i = 0; i < MAX_BANDS; ++i)
    {
        auto id = juce::String(i + 1);

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"b" + id + "_freq", 1},
            "Band " + id + " Freq",
            juce::NormalisableRange<float>(20.0f, 22000.0f, 0.1f, 0.3f), 1000.0f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"b" + id + "_gain", 1},
            "Band " + id + " Gain",
            juce::NormalisableRange<float>(-30.0f, 30.0f, 0.01f), 0.0f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"b" + id + "_q", 1},
            "Band " + id + " Q",
            juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.4f), 0.707f));

        layout.add(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"b" + id + "_type", 1},
            "Band " + id + " Type",
            filterTypes, 0));

        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"b" + id + "_active", 1},
            "Band " + id + " Active",
            false));
    }

    return layout;
}

EeqProcessor::EeqProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, juce::Identifier("EeqState"), createLayout())
{
}

EeqProcessor::~EeqProcessor() {}

void EeqProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    equalizer.prepare(sampleRate, samplesPerBlock);
    spectrum.prepare(sampleRate);
}

void EeqProcessor::releaseResources() {}

void EeqProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto numSamples = buffer.getNumSamples();

    for (int i = 0; i < MAX_BANDS; ++i)
    {
        auto id = juce::String(i + 1);

        float freq = apvts.getRawParameterValue("b" + id + "_freq")->load();
        float gain = apvts.getRawParameterValue("b" + id + "_gain")->load();
        float q = apvts.getRawParameterValue("b" + id + "_q")->load();
        int typeIdx = (int)apvts.getRawParameterValue("b" + id + "_type")->load();
        bool active = apvts.getRawParameterValue("b" + id + "_active")->load() > 0.5f;

        FilterType types[] = {
            FilterType::Bell, FilterType::LowShelf, FilterType::HighShelf,
            FilterType::LowCut, FilterType::HighCut, FilterType::Notch,
            FilterType::BandPass, FilterType::FlatTilt
        };

        BandState state;
        state.freq = freq;
        state.gain = gain;
        state.q = q;
        state.type = types[typeIdx];
        state.active = active;
        state.bypassed = false;
        equalizer.setBand(i, state);
    }

    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : left;

    equalizer.process(left, right, numSamples);
    spectrum.pushSamples(left, numSamples);
}

juce::AudioProcessorEditor* EeqProcessor::createEditor()
{
    return new EeqEditor(*this);
}

void EeqProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void EeqProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EeqProcessor();
}

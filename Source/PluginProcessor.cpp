#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <vector>
#include <algorithm>

juce::String EeqProcessor::getBandParamId(int band, const juce::String& suffix) const
{
    return "b" + juce::String(band + 1) + "_" + suffix;
}

juce::AudioProcessorValueTreeState::ParameterLayout EeqProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const juce::StringArray filterTypes = {
        "Bell", "LowShelf", "HighShelf", "LowCut", "HighCut", "Notch", "BandPass", "FlatTilt", "TiltShelf"
    };
    const juce::StringArray channelModes = {"Stereo", "Left", "Right", "Mid", "Side"};

    for (int i = 0; i < MAX_BANDS; ++i)
    {
        auto id = juce::String(i + 1);
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"b" + id + "_freq", 1}, "Band " + id + " Freq",
            juce::NormalisableRange<float>(20.0f, 22000.0f, 0.1f, 0.3f), 1000.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"b" + id + "_gain", 1}, "Band " + id + " Gain",
            juce::NormalisableRange<float>(-30.0f, 30.0f, 0.01f), 0.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"b" + id + "_q", 1}, "Band " + id + " Q",
            juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.4f), 0.707f));
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"b" + id + "_type", 1}, "Band " + id + " Type", filterTypes, 0));
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"b" + id + "_active", 1}, "Band " + id + " Active", false));
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"b" + id + "_ch", 1}, "Band " + id + " Channel", channelModes, 0));
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"b" + id + "_dyn", 1}, "Band " + id + " Dynamic", false));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"b" + id + "_dynRange", 1}, "Band " + id + " Dyn Range",
            juce::NormalisableRange<float>(-30.0f, 30.0f, 0.1f), 0.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"b" + id + "_dynThresh", 1}, "Band " + id + " Dyn Threshold",
            juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -20.0f));
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"b" + id + "_dynAuto", 1}, "Band " + id + " Auto Threshold", true));
    }

    for (int i = 0; i < MAX_BANDS; ++i)
    {
        auto id = juce::String(i + 1);
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"b" + id + "_solo", 1}, "Band " + id + " Solo", false));
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"b" + id + "_bypass", 1}, "Band " + id + " Bypass", false));
    }

    return layout;
}

EeqProcessor::EeqProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)
          .withInput("Sidechain", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, juce::Identifier("EeqState"), createLayout())
{
}

EeqProcessor::~EeqProcessor() {}

void EeqProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    equalizer.prepare(sampleRate, samplesPerBlock);
    spectrum.prepare(sampleRate);
    sidechainSpectrum.prepare(sampleRate);
    undoStack.clear();
    redoStack.clear();
}

void EeqProcessor::releaseResources() {}

int EeqProcessor::getLatencySamples() const
{
    if (currentMode == ProcessingMode::LinearPhase)
        return 2048;
    if (currentMode == ProcessingMode::NaturalPhase)
        return 64;
    return 0;
}

void EeqProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    // Update EQ bands from APVTS
    for (int i = 0; i < MAX_BANDS; ++i)
    {
        auto id = juce::String(i + 1);
        BandState state;
        state.freq = apvts.getRawParameterValue("b" + id + "_freq")->load();
        state.gain = apvts.getRawParameterValue("b" + id + "_gain")->load();
        state.q = apvts.getRawParameterValue("b" + id + "_q")->load();
        int typeIdx = (int)apvts.getRawParameterValue("b" + id + "_type")->load();
        state.active = apvts.getRawParameterValue("b" + id + "_active")->load() > 0.5f;
        state.soloed = apvts.getRawParameterValue("b" + id + "_solo")->load() > 0.5f;
        state.bypassed = apvts.getRawParameterValue("b" + id + "_bypass")->load() > 0.5f;
        int chIdx = (int)apvts.getRawParameterValue("b" + id + "_ch")->load();
        state.dynamic.enabled = apvts.getRawParameterValue("b" + id + "_dyn")->load() > 0.5f;
        state.dynamic.dynamicRange = apvts.getRawParameterValue("b" + id + "_dynRange")->load();
        state.dynamic.threshold = apvts.getRawParameterValue("b" + id + "_dynThresh")->load();
        state.dynamic.autoThreshold = apvts.getRawParameterValue("b" + id + "_dynAuto")->load() > 0.5f;

        FilterType types[] = {FilterType::Bell, FilterType::LowShelf, FilterType::HighShelf,
                              FilterType::LowCut, FilterType::HighCut, FilterType::Notch,
                              FilterType::BandPass, FilterType::FlatTilt, FilterType::TiltShelf};
        state.type = types[typeIdx % 9];

        ChannelMode modes[] = {ChannelMode::Stereo, ChannelMode::Left, ChannelMode::Right,
                               ChannelMode::Mid, ChannelMode::Side};
        state.channelMode = modes[chIdx % 5];

        equalizer.setBand(i, state);
    }

    equalizer.setGainScale(gainScale);
    equalizer.setProcessingMode(currentMode);

    if (numChannels >= 2)
    {
        auto* left = buffer.getWritePointer(0);
        auto* right = buffer.getWritePointer(1);

        if (currentMode == ProcessingMode::LinearPhase)
            equalizer.processLinearPhase(left, right, numSamples);
        else
            equalizer.process(left, right, numSamples);

        // Auto Gain compensation
        if (autoGainEnabled)
        {
            float eqGain = 0.0f;
            for (int i = 0; i < MAX_BANDS; ++i)
                if (equalizer.getBand(i).active)
                    eqGain += std::abs(equalizer.getBand(i).gain);
            float compensation = -eqGain / (float)MAX_BANDS * 0.3f;
            for (int s = 0; s < numSamples; ++s)
            {
                left[s] *= std::pow(10.0f, compensation / 20.0f);
                right[s] *= std::pow(10.0f, compensation / 20.0f);
            }
        }

        // Phase invert
        if (phaseInverted)
        {
            for (int s = 0; s < numSamples; ++s)
            {
                left[s] = -left[s];
                right[s] = -right[s];
            }
        }

        // Output pan
        if (std::abs(outputPan) > 0.01f)
        {
            float leftGain = std::min(1.0f, 1.0f - outputPan);
            float rightGain = std::min(1.0f, 1.0f + outputPan);
            for (int s = 0; s < numSamples; ++s)
            {
                left[s] *= leftGain;
                right[s] *= rightGain;
            }
        }

        spectrum.pushSamples(left, numSamples);

        // Output meter levels (RMS)
        float sumL = 0.0f, sumR = 0.0f;
        for (int s = 0; s < numSamples; ++s)
        {
            sumL += left[s] * left[s];
            sumR += right[s] * right[s];
        }
        outputLevelL = 20.0f * std::log10(std::sqrt(sumL / (float)numSamples) + 1e-10f);
        outputLevelR = 20.0f * std::log10(std::sqrt(sumR / (float)numSamples) + 1e-10f);
    }

    // Process sidechain input if available
    if (getTotalNumInputChannels() > 2)
    {
        auto* scLeft = buffer.getWritePointer(2);
        sidechainSpectrum.pushSamples(scLeft, numSamples);
    }
}


EQSnapshot EeqProcessor::captureState()
{
    EQSnapshot s;
    for (int i = 0; i < MAX_BANDS; ++i)
    {
        auto id = juce::String(i + 1);
        s.freqs[i] = apvts.getRawParameterValue("b" + id + "_freq")->load();
        s.gains[i] = apvts.getRawParameterValue("b" + id + "_gain")->load();
        s.qs[i] = apvts.getRawParameterValue("b" + id + "_q")->load();
        s.types[i] = (int)apvts.getRawParameterValue("b" + id + "_type")->load();
        s.actives[i] = apvts.getRawParameterValue("b" + id + "_active")->load() > 0.5f;
        s.channelModes[i] = (int)apvts.getRawParameterValue("b" + id + "_ch")->load();
        s.dynEnabled[i] = apvts.getRawParameterValue("b" + id + "_dyn")->load() > 0.5f;
        s.dynRange[i] = apvts.getRawParameterValue("b" + id + "_dynRange")->load();
        s.dynThreshold[i] = apvts.getRawParameterValue("b" + id + "_dynThresh")->load();
        s.solos[i] = apvts.getRawParameterValue("b" + id + "_solo")->load() > 0.5f;
        s.bypasses[i] = apvts.getRawParameterValue("b" + id + "_bypass")->load() > 0.5f;
    }
    s.gainScale = gainScale;
    return s;
}

void EeqProcessor::applyState(const EQSnapshot& s)
{
    for (int i = 0; i < MAX_BANDS; ++i)
    {
        auto id = juce::String(i + 1);
        apvts.getParameter("b" + id + "_freq")->setValueNotifyingHost(
            apvts.getParameter("b" + id + "_freq")->convertTo0to1(s.freqs[i]));
        apvts.getParameter("b" + id + "_gain")->setValueNotifyingHost(
            apvts.getParameter("b" + id + "_gain")->convertTo0to1(s.gains[i]));
        apvts.getParameter("b" + id + "_q")->setValueNotifyingHost(
            apvts.getParameter("b" + id + "_q")->convertTo0to1(s.qs[i]));
        apvts.getParameter("b" + id + "_type")->setValueNotifyingHost(
            apvts.getParameter("b" + id + "_type")->convertTo0to1(s.types[i]));
        apvts.getParameter("b" + id + "_active")->setValueNotifyingHost(s.actives[i] ? 1.0f : 0.0f);
        apvts.getParameter("b" + id + "_ch")->setValueNotifyingHost(
            apvts.getParameter("b" + id + "_ch")->convertTo0to1(s.channelModes[i]));
        apvts.getParameter("b" + id + "_dyn")->setValueNotifyingHost(s.dynEnabled[i] ? 1.0f : 0.0f);
        apvts.getParameter("b" + id + "_dynRange")->setValueNotifyingHost(
            apvts.getParameter("b" + id + "_dynRange")->convertTo0to1(s.dynRange[i]));
        apvts.getParameter("b" + id + "_dynThresh")->setValueNotifyingHost(
            apvts.getParameter("b" + id + "_dynThresh")->convertTo0to1(s.dynThreshold[i]));
        apvts.getParameter("b" + id + "_solo")->setValueNotifyingHost(s.solos[i] ? 1.0f : 0.0f);
        apvts.getParameter("b" + id + "_bypass")->setValueNotifyingHost(s.bypasses[i] ? 1.0f : 0.0f);
    }
    gainScale = s.gainScale;
}

void EeqProcessor::pushUndoState()
{
    undoStack.push_back(captureState());
    if ((int)undoStack.size() > MAX_UNDO)
        undoStack.pop_front();
    redoStack.clear();
}

void EeqProcessor::undo()
{
    if (undoStack.empty()) return;
    redoStack.push_back(captureState());
    applyState(undoStack.back());
    undoStack.pop_back();
}

void EeqProcessor::redo()
{
    if (redoStack.empty()) return;
    undoStack.push_back(captureState());
    applyState(redoStack.back());
    redoStack.pop_back();
}

juce::AudioProcessorEditor* EeqProcessor::createEditor()
{
    return new EeqEditor(*this);
}

void EeqProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("gainScale", gainScale, nullptr);
    state.setProperty("phaseInverted", phaseInverted, nullptr);
    state.setProperty("autoGain", autoGainEnabled, nullptr);
    state.setProperty("outputPan", outputPan, nullptr);
    state.setProperty("procMode", (int)currentMode, nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void EeqProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
    {
        auto state = juce::ValueTree::fromXml(*xml);
        apvts.replaceState(state);
        gainScale = state.getProperty("gainScale", 1.0f);
        phaseInverted = state.getProperty("phaseInverted", false);
        autoGainEnabled = state.getProperty("autoGain", true);
        outputPan = state.getProperty("outputPan", 0.0f);
        currentMode = (ProcessingMode)(int)state.getProperty("procMode", 0);
    }
}

void EeqProcessor::applyEQMatch()
{
    auto& analyzer = getSpectrumAnalyzer();
    const auto& capture = analyzer.getCaptureSpectrum();
    int captureCount = analyzer.getCaptureCount();
    if (captureCount < 10) return;

    float sr = analyzer.getSampleRate();
    int numBins = analyzer.getNumBins();
    float nyquist = sr * 0.5f;

    pushUndoState();

    // Sample the captured spectrum at more points and find peaks/valleys
    struct SpectralPoint { float freq; float magDB; };
    std::vector<SpectralPoint> points;

    int numSamplePoints = 128;
    for (int i = 0; i < numSamplePoints; ++i)
    {
        float normFreq = (float)(i + 1) / (float)(numSamplePoints + 1);
        float freq = 20.0f * std::pow(nyquist / 20.0f, normFreq); // Log-spaced
        int bin = (int)(freq / nyquist * (float)numBins);
        bin = juce::jlimit(1, numBins - 1, bin);
        float magVal = capture[bin];
        float magDB = (magVal - 0.5f) * 60.0f;
        points.push_back({ freq, magDB });
    }

    // Find local peaks and valleys (significant deviation from neighbors)
    struct Peak { float freq; float gainDB; float q; };
    std::vector<Peak> peaks;

    for (int i = 2; i < (int)points.size() - 2; ++i)
    {
        float avg = (points[i - 2].magDB + points[i - 1].magDB + points[i + 1].magDB + points[i + 2].magDB) / 4.0f;
        float deviation = points[i].magDB - avg;

        if (std::abs(deviation) >= 1.0f)
        {
            // Determine Q based on how narrow the peak is
            float q = 1.0f;
            if (i >= 3 && i < (int)points.size() - 3)
            {
                float avgWide = (points[i - 3].magDB + points[i + 3].magDB) / 2.0f;
                float narrowness = std::abs(deviation) / (std::abs(deviation - (points[i].magDB - avgWide)) + 0.1f);
                q = juce::jlimit(0.3f, 5.0f, narrowness * 1.5f);
            }

            peaks.push_back({ points[i].freq, -deviation, q });
        }
    }

    // Sort by magnitude of deviation (strongest first)
    std::sort(peaks.begin(), peaks.end(), [](const Peak& a, const Peak& b)
    {
        return std::abs(a.gainDB) > std::abs(b.gainDB);
    });

    // Apply to available bands
    int bandIdx = 0;
    for (const auto& peak : peaks)
    {
        if (bandIdx >= MAX_BANDS) break;
        if (std::abs(peak.gainDB) < 0.5f) continue;

        auto id = juce::String(bandIdx + 1);
        float normQ = juce::jlimit(0.1f, 10.0f, peak.q);
        float normQ01 = (normQ - 0.1f) / 9.9f;

        apvts.getParameter("b" + id + "_freq")->setValueNotifyingHost(
            apvts.getParameter("b" + id + "_freq")->convertTo0to1(peak.freq));
        apvts.getParameter("b" + id + "_gain")->setValueNotifyingHost(
            apvts.getParameter("b" + id + "_gain")->convertTo0to1(peak.gainDB));
        apvts.getParameter("b" + id + "_q")->setValueNotifyingHost(normQ01);
        apvts.getParameter("b" + id + "_type")->setValueNotifyingHost(
            apvts.getParameter("b" + id + "_type")->convertTo0to1(0)); // Bell
        apvts.getParameter("b" + id + "_active")->setValueNotifyingHost(1.0f);

        bandIdx++;
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EeqProcessor();
}

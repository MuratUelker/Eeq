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
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"b" + id + "_dynAutoAtk", 1}, "Band " + id + " Auto Attack", true));
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"b" + id + "_dynAutoRel", 1}, "Band " + id + " Auto Release", true));
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"b" + id + "_slope", 1}, "Band " + id + " Slope",
            juce::StringArray{"6 dB", "12 dB", "18 dB", "24 dB", "30 dB", "36 dB", "42 dB", "48 dB", "96 dB", "Brickwall"}, 3));
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"b" + id + "_sc", 1}, "Band " + id + " SC Trigger", false));
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"b" + id + "_phase", 1}, "Band " + id + " Phase Invert", false));
    }

    for (int i = 0; i < MAX_BANDS; ++i)
    {
        auto id = juce::String(i + 1);
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"b" + id + "_solo", 1}, "Band " + id + " Solo", false));
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"b" + id + "_bypass", 1}, "Band " + id + " Bypass", false));
    }

    // Global parameters
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"procMode", 1}, "Processing Mode",
        juce::StringArray{"Zero Latency", "Natural Phase", "Linear Phase"}, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"lpResolution", 1}, "Linear Phase Resolution",
        juce::StringArray{"Low (1024)", "Medium (2048)", "High (4096)", "Very High (8192)"}, 2));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"npResolution", 1}, "Natural Phase Resolution",
        juce::StringArray{"Low (1024)", "Medium (2048)", "High (4096)", "Very High (8192)"}, 2));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"displayRange", 1}, "Display Range",
        juce::StringArray{"3 dB", "6 dB", "12 dB", "30 dB"}, 3));

    return layout;
}

EeqProcessor::EeqProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)
          .withInput("Sidechain", juce::AudioChannelSet::stereo(), true)
          .withInput("Input_51", juce::AudioChannelSet::create5point1(), true)
          .withOutput("Output_51", juce::AudioChannelSet::create5point1(), true)
          .withInput("Input_71", juce::AudioChannelSet::create7point1(), true)
          .withOutput("Output_71", juce::AudioChannelSet::create7point1(), true)),
      apvts(*this, nullptr, juce::Identifier("EeqState"), createLayout())
{
    loadStateFromFile();
}

EeqProcessor::~EeqProcessor()
{
    saveStateToFile();
}

void EeqProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    equalizer.setProcessingMode(currentMode);
    equalizer.setLinearPhaseResolution(lpResolution);
    equalizer.setNaturalPhaseResolution(npResolution);
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
        return static_cast<int>(lpResolution) / 2;
    if (currentMode == ProcessingMode::NaturalPhase)
        return static_cast<int>(npResolution) / 2;
    return 0;
}

void EeqProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    // Handle MIDI Learn
    if (midiLearnActive)
    {
        for (const auto metadata : midiMessages)
        {
            const juce::MidiMessage& msg = metadata.getMessage();
            if (msg.isController())
            {
                int cc = msg.getControllerNumber();
                int channel = msg.getChannel();
                if (selectedBandForMidiLearn >= 0 && selectedParamForMidiLearn.isNotEmpty())
                {
                    addMidiMapping(selectedBandForMidiLearn, selectedParamForMidiLearn, cc, channel);
                    midiLearnActive = false;
                    // Notify editor to update UI
                }
            }
        }
    }

    // Handle existing MIDI mappings
    for (const auto& mapping : midiMappings)
    {
        for (const auto metadata : midiMessages)
        {
            const juce::MidiMessage& msg = metadata.getMessage();
            if (msg.isController() && msg.getControllerNumber() == mapping.cc && msg.getChannel() == mapping.channel)
            {
                float value = msg.getControllerValue() / 127.0f;
                applyMidiMapping(mapping, value);
            }
        }
    }

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
        state.dynamic.autoAttack = apvts.getRawParameterValue("b" + id + "_dynAutoAtk")->load() > 0.5f;
        state.dynamic.autoRelease = apvts.getRawParameterValue("b" + id + "_dynAutoRel")->load() > 0.5f;
        state.scTrigger = apvts.getRawParameterValue("b" + id + "_sc")->load() > 0.5f;
        state.phaseInverted = apvts.getRawParameterValue("b" + id + "_phase")->load() > 0.5f;
        int slopeIdx = (int)apvts.getRawParameterValue("b" + id + "_slope")->load();
        state.slope = (FilterSlope)juce::jlimit(0, 7, slopeIdx);

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

    // Check for processing mode change
    ProcessingMode newMode = (ProcessingMode)(int)apvts.getRawParameterValue("procMode")->load();
    if (newMode != currentMode)
    {
        currentMode = newMode;
        equalizer.setProcessingMode(currentMode);
    }

    // Check for linear phase resolution change
    LinearPhaseResolution newResolution = (LinearPhaseResolution)(int)apvts.getRawParameterValue("lpResolution")->load();
    if (newResolution != lpResolution)
    {
        lpResolution = newResolution;
        equalizer.setLinearPhaseResolution(lpResolution);
    }

    // Check for natural phase resolution change
    NaturalPhaseResolution newNPResolution = (NaturalPhaseResolution)(int)apvts.getRawParameterValue("npResolution")->load();
    if (newNPResolution != npResolution)
    {
        npResolution = newNPResolution;
        equalizer.setNaturalPhaseResolution(npResolution);
    }

    // Check for display range change
    float newDisplayRange = apvts.getRawParameterValue("displayRange")->load();
    if (std::abs(newDisplayRange - displayRange) > 0.01f)
    {
        displayRange = newDisplayRange;
    }

    equalizer.setProcessingMode(currentMode);

    if (numChannels >= 2)
    {
        // For surround, use multi-channel processing
        if (numChannels > 2)
        {
            std::vector<float*> channels(numChannels);
            for (int ch = 0; ch < numChannels; ++ch)
                channels[ch] = buffer.getWritePointer(ch);

            if (currentMode == ProcessingMode::LinearPhase)
            {
                // Linear phase - process first two channels as stereo, others as mono
                auto* left = buffer.getWritePointer(0);
                auto* right = buffer.getWritePointer(1);
                equalizer.processLinearPhase(left, right, numSamples);
                // Process remaining channels
                for (int ch = 2; ch < numChannels; ++ch)
                {
                    auto* chData = buffer.getWritePointer(ch);
                    // Apply same processing to other channels
                    float* monoChannels[1] = { chData };
                    equalizer.processMultiChannel(monoChannels, 1, numSamples);
                }
            }
            else if (currentMode == ProcessingMode::NaturalPhase)
            {
                auto* left = buffer.getWritePointer(0);
                auto* right = buffer.getWritePointer(1);
                equalizer.processNaturalPhase(left, right, numSamples);
                for (int ch = 2; ch < numChannels; ++ch)
                {
                    auto* chData = buffer.getWritePointer(ch);
                    float* monoChannels[1] = { chData };
                    equalizer.processMultiChannel(monoChannels, 1, numSamples);
                }
            }
            else
            {
                equalizer.processMultiChannel(channels.data(), numChannels, numSamples);
            }

            // Auto Gain compensation
            if (autoGainEnabled)
            {
                float eqGain = 0.0f;
                for (int i = 0; i < MAX_BANDS; ++i)
                    if (equalizer.getBand(i).active)
                        eqGain += std::abs(equalizer.getBand(i).gain);
                float compensation = -eqGain / (float)MAX_BANDS * 0.3f;
                float gain = std::pow(10.0f, compensation / 20.0f);
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    auto* chData = buffer.getWritePointer(ch);
                    for (int s = 0; s < numSamples; ++s)
                        chData[s] *= gain;
                }
            }

            // Phase invert
            if (phaseInverted)
            {
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    auto* chData = buffer.getWritePointer(ch);
                    for (int s = 0; s < numSamples; ++s)
                        chData[s] = -chData[s];
                }
            }

            // Output pan (stereo only)
            if (numChannels >= 2 && std::abs(outputPan) > 0.01f)
            {
                auto* left = buffer.getWritePointer(0);
                auto* right = buffer.getWritePointer(1);
                float leftGain = std::min(1.0f, 1.0f - outputPan);
                float rightGain = std::min(1.0f, 1.0f + outputPan);
                for (int s = 0; s < numSamples; ++s)
                {
                    left[s] *= leftGain;
                    right[s] *= rightGain;
                }
            }

            spectrum.pushSamples(buffer.getWritePointer(0), numSamples);
        }

        // Output meter levels (RMS) for multi-channel
        float sumL = 0.0f, sumR = 0.0f;
        auto* left = buffer.getWritePointer(0);
        auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : left;
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
        auto* scRight = buffer.getNumChannels() > 3 ? buffer.getWritePointer(3) : scLeft;
        sidechainSpectrum.pushSamples(scLeft, numSamples);

        float scSumL = 0.0f, scSumR = 0.0f;
        for (int s = 0; s < numSamples; ++s)
        {
            scSumL += scLeft[s] * scLeft[s];
            scSumR += scRight[s] * scRight[s];
        }
float scRmsL = std::sqrt(scSumL / (float)numSamples);
        float scRmsR = std::sqrt(scSumR / (float)numSamples);
        equalizer.setSidechainLevels(scRmsL, scRmsR);
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
        s.scTriggers[i] = apvts.getRawParameterValue("b" + id + "_sc")->load() > 0.5f;
        s.slopes[i] = (int)apvts.getRawParameterValue("b" + id + "_slope")->load();
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
        apvts.getParameter("b" + id + "_sc")->setValueNotifyingHost(s.scTriggers[i] ? 1.0f : 0.0f);
        apvts.getParameter("b" + id + "_slope")->setValueNotifyingHost(
            apvts.getParameter("b" + id + "_slope")->convertTo0to1(s.slopes[i]));
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
    state.setProperty("lpResolution", (int)lpResolution, nullptr);
    state.setProperty("npResolution", (int)npResolution, nullptr);
    state.setProperty("displayRange", displayRange, nullptr);
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
        lpResolution = (LinearPhaseResolution)(int)state.getProperty("lpResolution", (int)LinearPhaseResolution::High);
        npResolution = (NaturalPhaseResolution)(int)state.getProperty("npResolution", (int)NaturalPhaseResolution::High);
        equalizer.setLinearPhaseResolution(lpResolution);
        equalizer.setNaturalPhaseResolution(npResolution);
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

void EeqProcessor::applyEQMatchExternal()
{
    auto& analyzer = getSpectrumAnalyzer();
    if (!analyzer.hasExternalSpectrum()) return;

    const auto& external = analyzer.getExternalSpectrum();
    int numBins = analyzer.getNumBins();
    float sr = analyzer.getSampleRate();
    float nyquist = sr * 0.5f;

    pushUndoState();

    // Sample the external spectrum at log-spaced points and find peaks/valleys
    struct SpectralPoint { float freq; float magDB; };
    std::vector<SpectralPoint> points;

    int numSamplePoints = 128;
    for (int i = 0; i < numSamplePoints; ++i)
    {
        float normFreq = (float)(i + 1) / (float)(numSamplePoints + 1);
        float freq = 20.0f * std::pow(nyquist / 20.0f, normFreq); // Log-spaced
        int bin = (int)(freq / nyquist * (float)numBins);
        bin = juce::jlimit(1, numBins - 1, bin);
        float magVal = external[bin];
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

static juce::File getStateFile()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Eeq").getChildFile("EeqState.xml");
}

void EeqProcessor::saveStateToFile()
{
    auto state = apvts.copyState();
    state.setProperty("gainScale", gainScale, nullptr);
    state.setProperty("phaseInverted", phaseInverted, nullptr);
    state.setProperty("autoGain", autoGainEnabled, nullptr);
    state.setProperty("outputPan", outputPan, nullptr);
    state.setProperty("procMode", (int)currentMode, nullptr);
    state.setProperty("displayRange", displayRange, nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    auto file = getStateFile();
    file.createDirectory();
    xml->writeTo(file);
}

void EeqProcessor::loadStateFromFile()
{
    auto file = getStateFile();
    if (!file.existsAsFile()) return;

    std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(file));
    if (xml && xml->hasTagName(apvts.state.getType()))
    {
        auto state = juce::ValueTree::fromXml(*xml);
        apvts.replaceState(state);
        gainScale = state.getProperty("gainScale", 1.0f);
        phaseInverted = state.getProperty("phaseInverted", false);
        autoGainEnabled = state.getProperty("autoGain", true);
        outputPan = state.getProperty("outputPan", 0.0f);
        currentMode = (ProcessingMode)(int)state.getProperty("procMode", 0);
        lpResolution = (LinearPhaseResolution)(int)state.getProperty("lpResolution", (int)LinearPhaseResolution::High);
        displayRange = state.getProperty("displayRange", 30.0f);
        equalizer.setLinearPhaseResolution(lpResolution);
    }
}

juce::File EeqProcessor::getUserPresetFolder() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Eeq").getChildFile("Presets");
}

void EeqProcessor::saveUserPreset(const juce::String& name)
{
    auto folder = getUserPresetFolder();
    folder.createDirectory();

    auto state = apvts.copyState();
    state.setProperty("gainScale", gainScale, nullptr);
    state.setProperty("phaseInverted", phaseInverted, nullptr);
    state.setProperty("autoGain", autoGainEnabled, nullptr);
    state.setProperty("outputPan", outputPan, nullptr);
    state.setProperty("procMode", (int)currentMode, nullptr);
    state.setProperty("lpResolution", (int)lpResolution, nullptr);
    state.setProperty("npResolution", (int)npResolution, nullptr);
    state.setProperty("displayRange", displayRange, nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    auto file = folder.getChildFile(name + ".xml");
    xml->writeTo(file);
}

void EeqProcessor::deleteUserPreset(const juce::String& name)
{
    auto folder = getUserPresetFolder();
    auto file = folder.getChildFile(name + ".xml");
    if (file.existsAsFile())
        file.deleteFile();
}

juce::StringArray EeqProcessor::getUserPresetNames() const
{
    juce::StringArray names;
    auto folder = getUserPresetFolder();
    if (folder.isDirectory())
    {
        juce::Array<juce::File> files;
        folder.findChildFiles(files, juce::File::findFiles, true, "*.xml");
        for (auto& f : files)
            names.add(f.getFileNameWithoutExtension());
    }
    return names;
}

void EeqProcessor::loadUserPreset(const juce::String& name)
{
    auto folder = getUserPresetFolder();
    auto file = folder.getChildFile(name + ".xml");
    if (!file.existsAsFile()) return;

    std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(file));
    if (xml && xml->hasTagName(apvts.state.getType()))
    {
        auto state = juce::ValueTree::fromXml(*xml);
        apvts.replaceState(state);
        gainScale = state.getProperty("gainScale", 1.0f);
        phaseInverted = state.getProperty("phaseInverted", false);
        autoGainEnabled = state.getProperty("autoGain", true);
        outputPan = state.getProperty("outputPan", 0.0f);
        currentMode = (ProcessingMode)(int)state.getProperty("procMode", 0);
        lpResolution = (LinearPhaseResolution)(int)state.getProperty("lpResolution", (int)LinearPhaseResolution::High);
        npResolution = (NaturalPhaseResolution)(int)state.getProperty("npResolution", (int)NaturalPhaseResolution::High);
        displayRange = state.getProperty("displayRange", 30.0f);
        equalizer.setProcessingMode(currentMode);
        equalizer.setLinearPhaseResolution(lpResolution);
        equalizer.setNaturalPhaseResolution(npResolution);
    }
}

void EeqProcessor::setMidiLearnActive(bool active)
{
    midiLearnActive = active;
    if (!active)
    {
        // Could add logic here to save mappings
    }
}

void EeqProcessor::addMidiMapping(int band, const juce::String& param, int cc, int channel)
{
    MidiMapping mapping;
    mapping.band = band;
    mapping.param = param;
    mapping.cc = cc;
    mapping.channel = channel;
    midiMappings.push_back(mapping);
}

void EeqProcessor::clearMidiMappings()
{
    midiMappings.clear();
}

void EeqProcessor::setMidiLearnTarget(int band, const juce::String& param)
{
    selectedBandForMidiLearn = band;
    selectedParamForMidiLearn = param;
}

void EeqProcessor::applyMidiMapping(const MidiMapping& mapping, float value)
{
    if (mapping.band < 0 || mapping.band >= MAX_BANDS) return;
    
    auto id = juce::String(mapping.band + 1);
    auto* param = apvts.getParameter("b" + id + "_" + mapping.param);
    if (param != nullptr)
    {
        param->setValueNotifyingHost(param->convertTo0to1(value));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EeqProcessor();
}

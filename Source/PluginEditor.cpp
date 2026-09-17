#include "PluginEditor.h"
#include <cmath>

static const juce::StringArray filterTypeNames = {
    "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut", "Notch", "Band Pass", "Flat Tilt", "Tilt Shelf"
};

static const juce::StringArray channelModeNames = {"Stereo", "Left", "Right", "Mid", "Side"};
static const juce::StringArray procModeNames = {"Zero Latency", "Natural Phase", "Linear Phase"};
static const juce::StringArray analyzerNames = {"Pre", "Post", "Off"};

static const juce::StringArray factoryPresetNames = {
    "Init", "Vocal Presence", "De-Esser", "Guitar Bright", "Bass Tight",
    "Drum Smash", "Master Bright", "Master Warm", "Low Pass 8k", "High Pass 80"
};

static const std::vector<std::vector<float>> factoryPresets = {
    {1000, 0, 0.707f, 0, false},
    {3000, 4, 1.5f, 0, true},
    {6000, -6, 3.0f, 0, true},
    {2000, 3, 1.0f, 1, true},
    {100, 3, 0.8f, 1, true},
    {4000, 3, 0.5f, 0, true},
    {10000, 2, 0.7f, 1, true},
    {200, 2, 0.8f, 1, true},
    {8000, -80, 0.7f, 4, true},
    {80, -80, 0.7f, 3, true},
};

static juce::Font makeFont(float size)
{
    return juce::Font(juce::FontOptions(size));
}

static juce::Font makeBoldFont(float size)
{
    return juce::Font(juce::FontOptions(size).withStyle("Bold"));
}

EeqEditor::EeqEditor(EeqProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(1100, 700);
    setResizable(true, true);
    setResizeLimits(800, 550, 2400, 1400);
    setWantsKeyboardFocus(true);

    // === Top bar ===
    for (const auto& name : factoryPresetNames)
        presetSelector.addItem(name, presetSelector.getNumItems() + 1);
    presetSelector.setSelectedId(1);
    presetSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF16213e));
    presetSelector.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    addAndMakeVisible(presetSelector);
    presetSelector.onChange = [this] { loadPreset(presetSelector.getSelectedItemIndex()); };

    savePresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF16213e));
    savePresetBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFe94560));
    addAndMakeVisible(savePresetBtn);

    for (const auto& name : procModeNames)
        procModeBox.addItem(name, procModeBox.getNumItems() + 1);
    procModeBox.setSelectedId(1);
    procModeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF16213e));
    procModeBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    addAndMakeVisible(procModeBox);
    procModeBox.addListener(this);

    for (const auto& name : analyzerNames)
        analyzerMode.addItem(name, analyzerMode.getNumItems() + 1);
    analyzerMode.setSelectedId(1);
    analyzerMode.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF16213e));
    analyzerMode.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    addAndMakeVisible(analyzerMode);

    freezeBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    freezeBtn.setClickingTogglesState(true);
    addAndMakeVisible(freezeBtn);
    freezeBtn.addListener(this);

    abBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFe94560));
    abBtn.setClickingTogglesState(true);
    addAndMakeVisible(abBtn);
    abBtn.addListener(this);

    undoBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFa0a0c0));
    addAndMakeVisible(undoBtn);
    undoBtn.addListener(this);

    redoBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFa0a0c0));
    addAndMakeVisible(redoBtn);
    redoBtn.addListener(this);

    fullScreenBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    addAndMakeVisible(fullScreenBtn);
    fullScreenBtn.addListener(this);

    // === Band controls ===
    freqSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    freqSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    freqSlider.setRange(20.0, 22000.0, 0.1);
    freqSlider.setTextValueSuffix(" Hz");
    freqSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFFe94560));
    freqSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFFe94560));
    freqSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFe0e0ff));
    freqSlider.addListener(this);
    addAndMakeVisible(freqSlider);

    gainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    gainSlider.setRange(-30.0, 30.0, 0.01);
    gainSlider.setTextValueSuffix(" dB");
    gainSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF00b4d8));
    gainSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFF00b4d8));
    gainSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFe0e0ff));
    gainSlider.addListener(this);
    addAndMakeVisible(gainSlider);

    qSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    qSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    qSlider.setRange(0.1, 10.0, 0.01);
    qSlider.setSkewFactor(0.4);
    qSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF533483));
    qSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFF533483));
    qSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFe0e0ff));
    qSlider.addListener(this);
    addAndMakeVisible(qSlider);

    for (auto* label : {&freqLabel, &gainLabel, &qLabel, &typeLabel, &chLabel})
    {
        label->setJustificationType(juce::Justification::centred);
        label->setFont(makeFont(10.0f));
        label->setColour(juce::Label::textColourId, juce::Colour(0xFFa0a0c0));
        addAndMakeVisible(*label);
    }

    for (const auto& name : filterTypeNames)
        typeBox.addItem(name, typeBox.getNumItems() + 1);
    typeBox.setSelectedId(1);
    typeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF1a1a2e));
    typeBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    addAndMakeVisible(typeBox);
    typeBox.addListener(this);

    for (const auto& name : channelModeNames)
        channelModeBox.addItem(name, channelModeBox.getNumItems() + 1);
    channelModeBox.setSelectedId(1);
    channelModeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF1a1a2e));
    channelModeBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    addAndMakeVisible(channelModeBox);
    channelModeBox.addListener(this);

    soloBtn.setButtonText("S");
    soloBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFe9c46a));
    soloBtn.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xFFe9c46a));
    soloBtn.setClickingTogglesState(true);
    addAndMakeVisible(soloBtn);
    soloBtn.addListener(this);

    bandBypassBtn.setButtonText("B");
    bandBypassBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    bandBypassBtn.setClickingTogglesState(true);
    addAndMakeVisible(bandBypassBtn);
    bandBypassBtn.addListener(this);

    dynBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFF2a9d8f));
    dynBtn.setClickingTogglesState(true);
    addAndMakeVisible(dynBtn);
    dynBtn.addListener(this);

    dynRangeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    dynRangeSlider.setRange(-30.0, 30.0, 0.1);
    dynRangeSlider.setValue(0.0, juce::dontSendNotification);
    dynRangeSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFF2a9d8f));
    dynRangeSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xFF1a3a2e));
    dynRangeSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFe0e0ff));
    addAndMakeVisible(dynRangeSlider);
    dynRangeSlider.setVisible(false);

    dynThreshSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    dynThreshSlider.setRange(-60.0, 0.0, 0.1);
    dynThreshSlider.setValue(-20.0, juce::dontSendNotification);
    dynThreshSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFF2a9d8f));
    dynThreshSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xFF1a3a2e));
    dynThreshSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFe0e0ff));
    addAndMakeVisible(dynThreshSlider);
    dynThreshSlider.setVisible(false);

    dynAutoBtn.setButtonText("Auto");
    dynAutoBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFF2a9d8f));
    dynAutoBtn.setClickingTogglesState(true);
    dynAutoBtn.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(dynAutoBtn);
    dynAutoBtn.setVisible(false);
    dynAutoBtn.addListener(this);

    dynRangeLabel.setJustificationType(juce::Justification::centred);
    dynRangeLabel.setFont(makeFont(9.0f));
    dynRangeLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF2a9d8f));
    addAndMakeVisible(dynRangeLabel);
    dynRangeLabel.setVisible(false);

    dynThreshLabel.setJustificationType(juce::Justification::centred);
    dynThreshLabel.setFont(makeFont(9.0f));
    dynThreshLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF2a9d8f));
    addAndMakeVisible(dynThreshLabel);
    dynThreshLabel.setVisible(false);

    // === Bottom bar ===
    phaseBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFe94560));
    phaseBtn.setClickingTogglesState(true);
    addAndMakeVisible(phaseBtn);
    phaseBtn.addListener(this);

    autoGainBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    autoGainBtn.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(autoGainBtn);
    autoGainBtn.addListener(this);

    outputPanSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    outputPanSlider.setRange(-1.0, 1.0, 0.01);
    outputPanSlider.setValue(0.0, juce::dontSendNotification);
    outputPanSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFFe0e0ff));
    outputPanSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xFF2a2a4a));
    outputPanSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFe0e0ff));
    addAndMakeVisible(outputPanSlider);

    panLabel.setJustificationType(juce::Justification::centred);
    panLabel.setFont(makeFont(9.0f));
    panLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFa0a0c0));
    addAndMakeVisible(panLabel);

    gainScaleSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    gainScaleSlider.setRange(0.0, 2.0, 0.01);
    gainScaleSlider.setValue(1.0, juce::dontSendNotification);
    gainScaleSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFFe94560));
    gainScaleSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xFF2a2a4a));
    gainScaleSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFe0e0ff));
    addAndMakeVisible(gainScaleSlider);

    gainScaleLabel.setJustificationType(juce::Justification::centred);
    gainScaleLabel.setFont(makeFont(9.0f));
    gainScaleLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFa0a0c0));
    addAndMakeVisible(gainScaleLabel);

    // EQ Match
    eqMatchBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    eqMatchBtn.setClickingTogglesState(true);
    addAndMakeVisible(eqMatchBtn);
    eqMatchBtn.setVisible(false);
    eqMatchBtn.addListener(this);

    eqMatchCaptureBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF16213e));
    eqMatchCaptureBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFe94560));
    addAndMakeVisible(eqMatchCaptureBtn);
    eqMatchCaptureBtn.setVisible(false);
    eqMatchCaptureBtn.addListener(this);

    eqMatchApplyBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF16213e));
    eqMatchApplyBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF00b4d8));
    addAndMakeVisible(eqMatchApplyBtn);
    eqMatchApplyBtn.setVisible(false);
    eqMatchApplyBtn.addListener(this);

    startTimerHz(30);
}

EeqEditor::~EeqEditor()
{
    stopTimer();
    freqSlider.removeListener(this);
    gainSlider.removeListener(this);
    qSlider.removeListener(this);
    typeBox.removeListener(this);
    channelModeBox.removeListener(this);
    soloBtn.removeListener(this);
    bandBypassBtn.removeListener(this);
    dynBtn.removeListener(this);
    dynAutoBtn.removeListener(this);
    procModeBox.removeListener(this);
    freezeBtn.removeListener(this);
    abBtn.removeListener(this);
    undoBtn.removeListener(this);
    redoBtn.removeListener(this);
    fullScreenBtn.removeListener(this);
    phaseBtn.removeListener(this);
    autoGainBtn.removeListener(this);
    eqMatchBtn.removeListener(this);
    eqMatchCaptureBtn.removeListener(this);
    eqMatchApplyBtn.removeListener(this);
}

void EeqEditor::timerCallback() { repaint(); }

// ===================== Layout =====================

juce::Rectangle<float> EeqEditor::getTopBarBounds() const
{
    return getLocalBounds().toFloat().removeFromTop(36);
}

juce::Rectangle<float> EeqEditor::getPianoBounds() const
{
    return getLocalBounds().toFloat().removeFromBottom(24);
}

juce::Rectangle<float> EeqEditor::getBottomBarBounds() const
{
    return getLocalBounds().toFloat().removeFromBottom(120);
}

juce::Rectangle<float> EeqEditor::getDisplayBounds() const
{
    auto b = getLocalBounds().toFloat();
    b = b.reduced(36, 0);
    b.removeFromTop(36);
    b.removeFromBottom(24);
    b.removeFromBottom(84);
    return b;
}

juce::Rectangle<float> EeqEditor::getMeterBounds() const
{
    auto b = getLocalBounds().toFloat();
    return b.removeFromRight(16).reduced(0, 36);
}

// ===================== Coordinate mapping =====================

float EeqEditor::freqToX(float freq, juce::Rectangle<float> d) const
{
    float logMin = std::log10(MIN_FREQ);
    float logMax = std::log10(MAX_FREQ);
    float normalized = (std::log10(std::max(freq, MIN_FREQ)) - logMin) / (logMax - logMin);
    normalized = (normalized - hScroll) / hZoom;
    return d.getX() + normalized * d.getWidth();
}

float EeqEditor::xToFreq(float x, juce::Rectangle<float> d) const
{
    float logMin = std::log10(MIN_FREQ);
    float logMax = std::log10(MAX_FREQ);
    float normalized = (x - d.getX()) / d.getWidth();
    normalized = normalized * hZoom + hScroll;
    normalized = juce::jlimit(0.0f, 1.0f, normalized);
    return std::pow(10.0f, logMin + normalized * (logMax - logMin));
}

float EeqEditor::gainToY(float gain, juce::Rectangle<float> d) const
{
    float range = processor.getDisplayRange();
    return (d.getHeight() * 0.5f) - (gain / range) * (d.getHeight() * 0.5f);
}

float EeqEditor::yToGain(float y, juce::Rectangle<float> d) const
{
    float range = processor.getDisplayRange();
    return ((d.getHeight() * 0.5f) - y) / (d.getHeight() * 0.5f) * range;
}

float EeqEditor::qToRadius(float q) const
{
    return juce::jmap(q, 0.1f, 10.0f, 12.0f, 4.0f);
}

// ===================== Band interaction =====================

int EeqEditor::findBandAt(float mx, float my) const
{
    for (int i = NUM_BANDS - 1; i >= 0; --i)
    {
        if (!bandVisuals[i].active) continue;
        float dx = mx - bandVisuals[i].x;
        float dy = my - bandVisuals[i].y;
        if (dx * dx + dy * dy < 225.0f)
            return i;
    }
    return -1;
}

void EeqEditor::selectBand(int idx)
{
    selectedBand = idx;
    for (auto& bv : bandVisuals) { bv.selected = false; bv.soloed = false; bv.bypassed = false; }
    if (idx >= 0 && idx < NUM_BANDS)
    {
        bandVisuals[idx].selected = true;
        updateControlsFromBand(idx);
    }
}

void EeqEditor::updateControlsFromBand(int idx)
{
    if (idx < 0 || idx >= NUM_BANDS) return;
    auto id = juce::String(idx + 1);
    auto& apvts = processor.getAPVTS();

    float freqNorm = apvts.getRawParameterValue("b" + id + "_freq")->load();
    float gainNorm = apvts.getRawParameterValue("b" + id + "_gain")->load();
    float qNorm = apvts.getRawParameterValue("b" + id + "_q")->load();
    int typeIdx = (int)apvts.getRawParameterValue("b" + id + "_type")->load();
    int chIdx = (int)apvts.getRawParameterValue("b" + id + "_ch")->load();
    bool active = apvts.getRawParameterValue("b" + id + "_active")->load() > 0.5f;
    bool solo = apvts.getRawParameterValue("b" + id + "_solo")->load() > 0.5f;
    bool bypass = apvts.getRawParameterValue("b" + id + "_bypass")->load() > 0.5f;
    bool dyn = apvts.getRawParameterValue("b" + id + "_dyn")->load() > 0.5f;
    float dynR = apvts.getRawParameterValue("b" + id + "_dynRange")->load();
    float dynT = apvts.getRawParameterValue("b" + id + "_dynThresh")->load();
    bool dynA = apvts.getRawParameterValue("b" + id + "_dynAuto")->load() > 0.5f;

    freqSlider.setValue(freqNorm, juce::dontSendNotification);
    gainSlider.setValue(gainNorm, juce::dontSendNotification);
    qSlider.setValue(qNorm, juce::dontSendNotification);
    typeBox.setSelectedId(typeIdx + 1, juce::dontSendNotification);
    channelModeBox.setSelectedId(chIdx + 1, juce::dontSendNotification);
    dynBtn.setToggleState(dyn, juce::dontSendNotification);
    soloBtn.setToggleState(solo, juce::dontSendNotification);
    bandBypassBtn.setToggleState(bypass, juce::dontSendNotification);
    dynRangeSlider.setValue(dynR, juce::dontSendNotification);
    dynThreshSlider.setValue(dynT, juce::dontSendNotification);
    dynAutoBtn.setToggleState(dynA, juce::dontSendNotification);

    bool dynVisible = dyn;
    dynRangeSlider.setVisible(dynVisible);
    dynThreshSlider.setVisible(dynVisible);
    dynAutoBtn.setVisible(dynVisible);
    dynRangeLabel.setVisible(dynVisible);
    dynThreshLabel.setVisible(dynVisible);
}

void EeqEditor::updateBandFromControls(int idx)
{
    if (idx < 0 || idx >= NUM_BANDS) return;
    auto id = juce::String(idx + 1);
    auto& apvts = processor.getAPVTS();

    float freq = (float)freqSlider.getValue();
    float gain = (float)gainSlider.getValue();
    float q = (float)qSlider.getValue();
    int type = typeBox.getSelectedId() - 1;
    int ch = channelModeBox.getSelectedId() - 1;

    apvts.getParameter("b" + id + "_freq")->setValueNotifyingHost(
        apvts.getParameter("b" + id + "_freq")->convertTo0to1(freq));
    apvts.getParameter("b" + id + "_gain")->setValueNotifyingHost(
        apvts.getParameter("b" + id + "_gain")->convertTo0to1(gain));
    apvts.getParameter("b" + id + "_q")->setValueNotifyingHost(
        apvts.getParameter("b" + id + "_q")->convertTo0to1(q));
    apvts.getParameter("b" + id + "_type")->setValueNotifyingHost(
        apvts.getParameter("b" + id + "_type")->convertTo0to1(type));
    apvts.getParameter("b" + id + "_ch")->setValueNotifyingHost(
        apvts.getParameter("b" + id + "_ch")->convertTo0to1(ch));

    if (dynBtn.getToggleState())
    {
        apvts.getParameter("b" + id + "_dynRange")->setValueNotifyingHost(
            apvts.getParameter("b" + id + "_dynRange")->convertTo0to1((float)dynRangeSlider.getValue()));
        apvts.getParameter("b" + id + "_dynThresh")->setValueNotifyingHost(
            apvts.getParameter("b" + id + "_dynThresh")->convertTo0to1((float)dynThreshSlider.getValue()));
    }
}

void EeqEditor::updateBandFromMouse(int band, float mx, float my)
{
    auto display = getDisplayBounds();
    float freq = xToFreq(mx, display);
    float gain = yToGain(my, display);
    freq = juce::jlimit(MIN_FREQ, MAX_FREQ, freq);
    gain = juce::jlimit(MIN_DB, MAX_DB, gain);

    auto id = juce::String(band + 1);
    processor.getAPVTS().getParameter("b" + id + "_freq")->setValueNotifyingHost(
        processor.getAPVTS().getParameter("b" + id + "_freq")->convertTo0to1(freq));
    processor.getAPVTS().getParameter("b" + id + "_gain")->setValueNotifyingHost(
        processor.getAPVTS().getParameter("b" + id + "_gain")->convertTo0to1(gain));

    if (band == selectedBand)
        updateControlsFromBand(band);
}

void EeqEditor::addBandAt(float freq, float gain)
{
    for (int i = 0; i < NUM_BANDS; ++i)
    {
        if (!bandVisuals[i].active)
        {
            processor.pushUndoState();
            auto id = juce::String(i + 1);
            processor.getAPVTS().getParameter("b" + id + "_freq")->setValueNotifyingHost(
                processor.getAPVTS().getParameter("b" + id + "_freq")->convertTo0to1(freq));
            processor.getAPVTS().getParameter("b" + id + "_gain")->setValueNotifyingHost(
                processor.getAPVTS().getParameter("b" + id + "_gain")->convertTo0to1(gain));
            processor.getAPVTS().getParameter("b" + id + "_q")->setValueNotifyingHost(
                processor.getAPVTS().getParameter("b" + id + "_q")->convertTo0to1(0.707f));
            processor.getAPVTS().getParameter("b" + id + "_type")->setValueNotifyingHost(
                processor.getAPVTS().getParameter("b" + id + "_type")->convertTo0to1(0));
            processor.getAPVTS().getParameter("b" + id + "_active")->setValueNotifyingHost(1.0f);
            selectBand(i);
            break;
        }
    }
}

// ===================== MIDI helpers =====================

int EeqEditor::freqToMidiKey(float freq) const
{
    return (int)std::round(69.0f + 12.0f * std::log2(freq / 440.0f));
}

float EeqEditor::midiKeyToFreq(int key) const
{
    return 440.0f * std::pow(2.0f, (key - 69.0f) / 12.0f);
}

// ===================== Mouse =====================

void EeqEditor::mouseDown(const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    auto display = getDisplayBounds();
    float mx = e.position.x;
    float my = e.position.y;

    if (!display.contains(mx, my)) return;

    int hit = findBandAt(mx, my);

    if (e.mods.isRightButtonDown())
    {
        juce::PopupMenu menu;
        if (hit >= 0)
        {
            auto id = juce::String(hit + 1);
            menu.addSectionHeader("Band " + id);
            menu.addSeparator();
            for (int t = 0; t < filterTypeNames.size(); ++t)
                menu.addItem(t + 1, filterTypeNames[t]);
            menu.addSeparator();
            menu.addSectionHeader("Channel Mode");
            for (int c = 0; c < channelModeNames.size(); ++c)
                menu.addItem(50 + c, channelModeNames[c]);
            menu.addSeparator();
            menu.addItem(100, "Solo");
            menu.addItem(101, "Bypass Band");
            menu.addItem(102, "Dynamic EQ");
            menu.addSeparator();
            menu.addItem(200, "Delete Band");

            menu.showMenuAsync(juce::PopupMenu::Options(), [this, hit](int result)
            {
                if (result == 0) return;
                auto id = juce::String(hit + 1);
                auto& apvts = processor.getAPVTS();

                if (result >= 1 && result <= filterTypeNames.size())
                {
                    apvts.getParameter("b" + id + "_type")->setValueNotifyingHost(
                        apvts.getParameter("b" + id + "_type")->convertTo0to1(result - 1));
                    updateControlsFromBand(hit);
                }
                else if (result >= 50 && result < 50 + channelModeNames.size())
                {
                    apvts.getParameter("b" + id + "_ch")->setValueNotifyingHost(
                        apvts.getParameter("b" + id + "_ch")->convertTo0to1(result - 50));
                    updateControlsFromBand(hit);
                }
                else if (result == 100)
                {
                    bool s = soloBtn.getToggleState();
                    soloBtn.setToggleState(!s, juce::sendNotification);
                }
                else if (result == 101)
                {
                    bool b = bandBypassBtn.getToggleState();
                    bandBypassBtn.setToggleState(!b, juce::sendNotification);
                }
                else if (result == 102)
                {
                    bool d = dynBtn.getToggleState();
                    dynBtn.setToggleState(!d, juce::sendNotification);
                }
                else if (result == 200)
                {
                    processor.pushUndoState();
                    apvts.getParameter("b" + id + "_active")->setValueNotifyingHost(0.0f);
                    bandVisuals[hit].active = false;
                    if (selectedBand == hit) selectBand(-1);
                }
            });
        }
        else
        {
            float freq = xToFreq(mx, display);
            float gain = yToGain(my, display);
            menu.addItem(1, "Add Bell Band");
            menu.addItem(2, "Add Low Cut");
            menu.addItem(3, "Add High Cut");
            menu.addItem(4, "Add Low Shelf");
            menu.addItem(5, "Add High Shelf");
            menu.showMenuAsync(juce::PopupMenu::Options(), [this, freq, gain](int result)
            {
                if (result == 0) return;
                for (int i = 0; i < NUM_BANDS; ++i)
                {
                    if (!bandVisuals[i].active)
                    {
                        processor.pushUndoState();
                        auto id = juce::String(i + 1);
                        int type = result == 2 ? 3 : result == 3 ? 4 : result == 4 ? 1 : result == 5 ? 2 : 0;
                        processor.getAPVTS().getParameter("b" + id + "_freq")->setValueNotifyingHost(
                            processor.getAPVTS().getParameter("b" + id + "_freq")->convertTo0to1(freq));
                        processor.getAPVTS().getParameter("b" + id + "_gain")->setValueNotifyingHost(
                            processor.getAPVTS().getParameter("b" + id + "_gain")->convertTo0to1(gain));
                        processor.getAPVTS().getParameter("b" + id + "_type")->setValueNotifyingHost(
                            processor.getAPVTS().getParameter("b" + id + "_type")->convertTo0to1(type));
                        processor.getAPVTS().getParameter("b" + id + "_active")->setValueNotifyingHost(1.0f);
                        selectBand(i);
                        break;
                    }
                }
            });
        }
        return;
    }

    if (hit >= 0)
    {
        selectBand(hit);
        dragging = true;
        if (e.mods.isAltDown())
        {
            auto id = juce::String(hit + 1);
            bool bypassed = processor.getAPVTS().getRawParameterValue("b" + id + "_active")->load() > 0.5f;
            processor.pushUndoState();
            processor.getAPVTS().getParameter("b" + id + "_active")->setValueNotifyingHost(bypassed ? 0.0f : 1.0f);
        }
        if (e.mods.isCommandDown())
        {
            multiSelectedBands.push_back(hit);
            bandVisuals[hit].selected = true;
        }
    }
    else if (e.getNumberOfClicks() >= 2)
    {
        float freq = xToFreq(mx, display);
        float gain = yToGain(my, display);
        addBandAt(freq, gain);
    }
    else
    {
        float freq = xToFreq(mx, display);
        float gain = yToGain(my, display);

        // Spectrum Grab: check if clicking near the EQ curve
        float curveMagDB = 20.0f * std::log10(std::max(processor.getEqualizer().getMagnitudeAtFreq(freq), 1e-10f));
        float distToCurve = std::abs(gain - curveMagDB);

        if (distToCurve < 4.0f)
        {
            // Near the EQ curve — start spectrum grab
            spectrumGrabbing = true;
            spectrumGrabFreq = freq;
            spectrumGrabGain = curveMagDB;
            addBandAt(freq, curveMagDB);
        }
        else
        {
            // Far from curve — just add a band at click position
            addBandAt(freq, gain);
        }
    }
}

void EeqEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (spectrumGrabbing && selectedBand >= 0)
    {
        // Spectrum grab: update the band to follow the spectrum curve
        updateBandFromMouse(selectedBand, e.position.x, e.position.y);
        return;
    }

    if (dragging && selectedBand >= 0)
    {
        updateBandFromMouse(selectedBand, e.position.x, e.position.y);

        // Multi-band drag
        if (e.mods.isCommandDown() && multiSelectedBands.size() > 1)
        {
            for (int b : multiSelectedBands)
            {
                if (b != selectedBand && bandVisuals[b].active)
                    updateBandFromMouse(b, e.position.x, e.position.y);
            }
        }
    }
}

void EeqEditor::mouseUp(const juce::MouseEvent&)
{
    dragging = false;
    spectrumGrabbing = false;
}

void EeqEditor::mouseDoubleClick(const juce::MouseEvent& e)
{
    auto display = getDisplayBounds();
    if (!display.contains(e.position.x, e.position.y)) return;
    float freq = xToFreq(e.position.x, display);
    float gain = yToGain(e.position.y, display);
    addBandAt(freq, gain);
}

void EeqEditor::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (e.mods.isCommandDown())
    {
        hZoom = juce::jlimit(0.5f, 8.0f, hZoom * (1.0f + wheel.deltaY * 0.2f));
        return;
    }

    if (e.mods.isShiftDown() && selectedBand >= 0)
    {
        hScroll = juce::jlimit(-0.5f, 0.5f, hScroll - wheel.deltaY * 0.02f);
        return;
    }

    if (selectedBand < 0)
    {
        // Change display range
        float range = processor.getDisplayRange();
        range = juce::jlimit(6.0f, 60.0f, range - wheel.deltaY * 3.0f);
        processor.setDisplayRange(range);
        return;
    }
    auto id = juce::String(selectedBand + 1);

    if (e.mods.isAltDown())
    {
        float newFreq = (float)freqSlider.getValue() * (1.0f + wheel.deltaY * 0.05f);
        newFreq = juce::jlimit(MIN_FREQ, MAX_FREQ, newFreq);
        freqSlider.setValue(newFreq, juce::sendNotification);
    }
    else if (e.mods.isShiftDown())
    {
        float newGain = (float)gainSlider.getValue() + wheel.deltaY * 0.5f;
        newGain = juce::jlimit((float)MIN_DB, (float)MAX_DB, newGain);
        gainSlider.setValue(newGain, juce::sendNotification);
    }
    else
    {
        float newQ = (float)qSlider.getValue() * (1.0f + wheel.deltaY * 0.1f);
        newQ = juce::jlimit(0.1f, 10.0f, newQ);
        qSlider.setValue(newQ, juce::sendNotification);
    }
}

void EeqEditor::mouseMove(const juce::MouseEvent& e)
{
    int newHovered = findBandAt(e.position.x, e.position.y);
    if (newHovered != hoveredBand)
    {
        if (hoveredBand >= 0 && hoveredBand < NUM_BANDS)
            bandVisuals[hoveredBand].hovered = false;
        hoveredBand = newHovered;
        if (hoveredBand >= 0 && hoveredBand < NUM_BANDS)
            bandVisuals[hoveredBand].hovered = true;
        setMouseCursor(hoveredBand >= 0 ? juce::MouseCursor::PointingHandCursor
                                        : juce::MouseCursor::NormalCursor);
    }
}

bool EeqEditor::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        if (selectedBand >= 0)
        {
            processor.pushUndoState();
            auto id = juce::String(selectedBand + 1);
            processor.getAPVTS().getParameter("b" + id + "_active")->setValueNotifyingHost(0.0f);
            bandVisuals[selectedBand].active = false;
            selectBand(-1);
        }
        return true;
    }
    if (key == juce::KeyPress('z', true, false))
    {
        processor.undo();
        return true;
    }
    if (key == juce::KeyPress('z', true, true))
    {
        processor.redo();
        return true;
    }
    return false;
}

// ===================== Listeners =====================

void EeqEditor::sliderValueChanged(juce::Slider*)
{
    if (selectedBand >= 0)
    {
        processor.pushUndoState();
        updateBandFromControls(selectedBand);
    }
}

void EeqEditor::comboBoxChanged(juce::ComboBox* box)
{
    if (box == &procModeBox)
    {
        processor.setProcessingMode((ProcessingMode)(procModeBox.getSelectedId() - 1));
    }
    else if (box == &typeBox || box == &channelModeBox)
    {
        if (selectedBand >= 0)
        {
            processor.pushUndoState();
            updateBandFromControls(selectedBand);
        }
    }
}

void EeqEditor::buttonClicked(juce::Button* btn)
{
    if (btn == &freezeBtn)
        processor.getSpectrumAnalyzer().setFreeze(freezeBtn.getToggleState());

    else if (btn == &abBtn)
    {
        if (abBtn.getToggleState())
            processor.switchToB();
        else
            processor.switchToA();
    }
    else if (btn == &undoBtn)
        processor.undo();
    else if (btn == &redoBtn)
        processor.redo();

    else if (btn == &fullScreenBtn)
    {
        fullScreen = !fullScreen;
        if (fullScreen)
        {
            auto screenBounds = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->totalArea;
            setBounds(0, 0, screenBounds.getWidth(), screenBounds.getHeight());
        }
        else
            setSize(1100, 700);
    }
    else if (btn == &phaseBtn)
        processor.setPhaseInverted(phaseBtn.getToggleState());
    else if (btn == &autoGainBtn)
        processor.setAutoGainEnabled(autoGainBtn.getToggleState());

    else if (btn == &soloBtn && selectedBand >= 0)
    {
        auto id = juce::String(selectedBand + 1);
        processor.getAPVTS().getParameter("b" + id + "_solo")->setValueNotifyingHost(
            soloBtn.getToggleState() ? 1.0f : 0.0f);
        bandVisuals[selectedBand].soloed = soloBtn.getToggleState();
    }
    else if (btn == &bandBypassBtn && selectedBand >= 0)
    {
        auto id = juce::String(selectedBand + 1);
        processor.getAPVTS().getParameter("b" + id + "_bypass")->setValueNotifyingHost(
            bandBypassBtn.getToggleState() ? 1.0f : 0.0f);
        bandVisuals[selectedBand].bypassed = bandBypassBtn.getToggleState();
    }
    else if (btn == &dynBtn && selectedBand >= 0)
    {
        auto id = juce::String(selectedBand + 1);
        processor.pushUndoState();
        processor.getAPVTS().getParameter("b" + id + "_dyn")->setValueNotifyingHost(
            dynBtn.getToggleState() ? 1.0f : 0.0f);
        bool vis = dynBtn.getToggleState();
        dynRangeSlider.setVisible(vis);
        dynThreshSlider.setVisible(vis);
        dynAutoBtn.setVisible(vis);
        dynRangeLabel.setVisible(vis);
        dynThreshLabel.setVisible(vis);
    }
    else if (btn == &dynAutoBtn && selectedBand >= 0)
    {
        auto id = juce::String(selectedBand + 1);
        processor.getAPVTS().getParameter("b" + id + "_dynAuto")->setValueNotifyingHost(
            dynAutoBtn.getToggleState() ? 1.0f : 0.0f);
    }
    else if (btn == &eqMatchBtn)
    {
        eqMatchCaptureBtn.setVisible(eqMatchBtn.getToggleState());
        eqMatchApplyBtn.setVisible(eqMatchBtn.getToggleState());
    }
    else if (btn == &eqMatchCaptureBtn)
    {
        eqMatchCapturing = !eqMatchCapturing;
        processor.getSpectrumAnalyzer().setFreeze(eqMatchCapturing);
        eqMatchCaptureBtn.setButtonText(eqMatchCapturing ? "Stop" : "Capture");
        if (!eqMatchCapturing)
            processor.getSpectrumAnalyzer().startCapture();
    }
    else if (btn == &eqMatchApplyBtn)
    {
        processor.applyEQMatch();
        eqMatchCapturing = false;
        processor.getSpectrumAnalyzer().setFreeze(false);
        eqMatchCaptureBtn.setButtonText("Capture");
    }
}

// ===================== Painting =====================

void EeqEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF0a0a1a));

    auto topBar = getTopBarBounds();
    g.setColour(juce::Colour(0xFF0f0f2a));
    g.fillRect(topBar);

    g.setColour(juce::Colour(0xFFe94560));
    g.setFont(makeBoldFont(16.0f));
    g.drawText("EEQ", topBar.reduced(8, 0).removeFromLeft(40), juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xFF6a6a8e));
    g.setFont(makeFont(9.0f));
    g.drawText("PRESET", topBar.getX() + 58, topBar.getY() + 2, 40, 14, juce::Justification::centredRight);
    g.drawText("MODE", topBar.getX() + 285, topBar.getY() + 2, 32, 14, juce::Justification::centredRight);
    g.drawText("ANALYZER", topBar.getX() + 408, topBar.getY() + 2, 52, 14, juce::Justification::centredRight);

    auto display = getDisplayBounds();
    drawGrid(g, display);
    if (analyzerMode.getSelectedId() != 3)
        drawSpectrum(g, display);
    drawEQCurve(g, display);
    drawBandNodes(g, display);
    drawBandInfo(g, display);
    drawPianoRoll(g, getPianoBounds());
    drawOutputMeter(g, getMeterBounds());
}

void EeqEditor::drawGrid(juce::Graphics& g, juce::Rectangle<float> d)
{
    g.setColour(juce::Colour(0xFF15152a));
    float freqs[] = {20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};
    for (float f : freqs)
    {
        float x = freqToX(f, d);
        if (x >= d.getX() && x <= d.getRight())
            g.drawLine(x, d.getY(), x, d.getBottom(), 1.0f);
    }

    float range = processor.getDisplayRange();
    for (float dB = -range; dB <= range; dB += 6.0f)
    {
        float y = d.getY() + gainToY(dB, d);
        g.drawLine(d.getX(), y, d.getRight(), y, 1.0f);
    }

    g.setColour(juce::Colour(0xFF2a2a4a));
    float zeroY = d.getY() + gainToY(0.0f, d);
    g.drawLine(d.getX(), zeroY, d.getRight(), zeroY, 1.5f);

    g.setColour(juce::Colour(0xFF4a4a6e));
    g.setFont(makeFont(9.0f));
    const char* labels[] = {"20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k"};
    for (int i = 0; i < 10; ++i)
    {
        float x = freqToX(freqs[i], d);
        if (x >= d.getX() && x <= d.getRight())
            g.drawText(labels[i], x - 12, d.getBottom() - 14, 24, 12, juce::Justification::centred);
    }

    for (float dB = -range; dB <= range; dB += 10.0f)
    {
        float y = d.getY() + gainToY(dB, d);
        if (y >= d.getY() && y <= d.getBottom())
        {
            juce::String txt = (dB >= 0 ? "+" : "") + juce::String((int)dB);
            g.drawText(txt, d.getX() + 2, y - 7, 28, 14, juce::Justification::centredLeft);
        }
    }
}

void EeqEditor::drawSpectrum(juce::Graphics& g, juce::Rectangle<float> d)
{
    const auto& data = processor.getSpectrumAnalyzer().getSpectrumData();
    int numBins = processor.getSpectrumAnalyzer().getNumBins();
    float sr = processor.getSpectrumAnalyzer().getSampleRate();
    if (numBins <= 0 || sr <= 0) return;
    float nyquist = sr * 0.5f;

    juce::Path path;
    bool started = false;

    for (int px = 0; px < (int)d.getWidth(); ++px)
    {
        float freq = xToFreq((float)px, d);
        if (freq < MIN_FREQ || freq > MAX_FREQ) continue;
        int bin = (int)(freq / nyquist * (float)numBins);
        bin = juce::jlimit(0, numBins - 1, bin);
        float mag = data[bin];

        float x = d.getX() + (float)px;
        float y = d.getY() + d.getHeight() * (1.0f - mag);

        if (!started) { path.startNewSubPath(x, y); started = true; }
        else path.lineTo(x, y);
    }

    juce::Path filledPath(path);
    filledPath.lineTo(d.getRight(), d.getBottom());
    filledPath.lineTo(d.getX(), d.getBottom());
    filledPath.closeSubPath();

    g.setColour(juce::Colour(0xFF00ff88).withAlpha(0.08f));
    g.fillPath(filledPath);
    g.setColour(juce::Colour(0xFF00ff88).withAlpha(0.5f));
    g.strokePath(path, juce::PathStrokeType(1.5f));

    // Peak hold
    const auto& peaks = processor.getSpectrumAnalyzer().peakHold;
    for (int px = 0; px < (int)d.getWidth(); px += 2)
    {
        float freq = xToFreq((float)px, d);
        if (freq < MIN_FREQ || freq > MAX_FREQ) continue;
        int bin = (int)(freq / nyquist * (float)numBins);
        bin = juce::jlimit(0, numBins - 1, bin);
        float peak = peaks[bin];
        float y = d.getY() + d.getHeight() * (1.0f - peak);
        g.setColour(juce::Colour(0xFF00ff88).withAlpha(0.3f));
        g.drawLine(d.getX() + (float)px, y, d.getX() + (float)px + 1.0f, y, 1.0f);
    }

    // EQ Match capture spectrum
    if (eqMatchCapturing && processor.getSpectrumAnalyzer().isCapturing())
    {
        const auto& cap = processor.getSpectrumAnalyzer().getCaptureSpectrum();
        juce::Path capPath;
        bool capStarted = false;
        for (int px = 0; px < (int)d.getWidth(); ++px)
        {
            float freq = xToFreq((float)px, d);
            if (freq < MIN_FREQ || freq > MAX_FREQ) continue;
            int bin = (int)(freq / nyquist * (float)numBins);
            bin = juce::jlimit(0, numBins - 1, bin);
            float mag = cap[bin];
            float x = d.getX() + (float)px;
            float y = d.getY() + d.getHeight() * (1.0f - mag);
            if (!capStarted) { capPath.startNewSubPath(x, y); capStarted = true; }
            else capPath.lineTo(x, y);
        }
        g.setColour(juce::Colour(0xFFe94560).withAlpha(0.6f));
        g.strokePath(capPath, juce::PathStrokeType(1.0f));
    }
}

void EeqEditor::drawEQCurve(juce::Graphics& g, juce::Rectangle<float> d)
{
    juce::Path path;
    bool started = false;

    for (int px = 0; px < (int)d.getWidth(); ++px)
    {
        float freq = xToFreq((float)px, d);
        if (freq < MIN_FREQ || freq > MAX_FREQ) continue;
        float magDB = 20.0f * std::log10(std::max(processor.getEqualizer().getMagnitudeAtFreq(freq), 1e-10f));
        float range = processor.getDisplayRange();
        magDB = juce::jlimit(-range, range, magDB);

        float x = d.getX() + (float)px;
        float y = d.getY() + gainToY(magDB, d);

        if (!started) { path.startNewSubPath(x, y); started = true; }
        else path.lineTo(x, y);
    }

    g.setColour(juce::Colour(0xFFe94560).withAlpha(0.15f));
    g.strokePath(path, juce::PathStrokeType(5.0f));
    g.setColour(juce::Colour(0xFFe94560));
    g.strokePath(path, juce::PathStrokeType(2.0f));
}

void EeqEditor::drawBandNodes(juce::Graphics& g, juce::Rectangle<float> d)
{
    auto& apvts = processor.getAPVTS();

    for (int i = 0; i < NUM_BANDS; ++i)
    {
        auto id = juce::String(i + 1);
        float freq = apvts.getRawParameterValue("b" + id + "_freq")->load();
        float gain = apvts.getRawParameterValue("b" + id + "_gain")->load();
        float q = apvts.getRawParameterValue("b" + id + "_q")->load();
        bool active = apvts.getRawParameterValue("b" + id + "_active")->load() > 0.5f;

        bandVisuals[i].active = active;
        bandVisuals[i].soloed = apvts.getRawParameterValue("b" + id + "_solo")->load() > 0.5f;
        bandVisuals[i].bypassed = apvts.getRawParameterValue("b" + id + "_bypass")->load() > 0.5f;
        if (!active) continue;

        float x = freqToX(freq, d);
        float y = d.getY() + gainToY(gain, d);

        if (x < d.getX() - 20 || x > d.getRight() + 20) continue;

        bandVisuals[i].x = x;
        bandVisuals[i].y = y;

        float radius = qToRadius(q);
        auto col = bandColours[i % 24];

        if (bandVisuals[i].selected)
        {
            g.setColour(col.withAlpha(0.2f));
            g.fillEllipse(x - radius - 6, y - radius - 6, (radius + 6) * 2, (radius + 6) * 2);

            int typeIdx = (int)apvts.getRawParameterValue("b" + id + "_type")->load();
            int chIdx = (int)apvts.getRawParameterValue("b" + id + "_ch")->load();
            juce::String info = filterTypeNames[juce::jmin(typeIdx, 8)] + "\n"
                + channelModeNames[juce::jmin(chIdx, 4)] + "\n"
                + juce::String(freq, 0) + " Hz\n"
                + juce::String(gain, 1) + " dB\nQ: " + juce::String(q, 2);
            g.setFont(makeFont(9.0f));
            g.setColour(col.withAlpha(0.7f));
            g.drawText(info, x - 45, y - radius - 58, 90, 54, juce::Justification::centred);
        }

        if (bandVisuals[i].hovered && !bandVisuals[i].selected)
        {
            g.setColour(col.withAlpha(0.15f));
            g.fillEllipse(x - radius - 3, y - radius - 3, (radius + 3) * 2, (radius + 3) * 2);
        }

        g.setColour(col.withAlpha(0.6f));
        g.fillEllipse(x - radius, y - radius, radius * 2, radius * 2);
        g.setColour(col);
        g.fillEllipse(x - radius + 2, y - radius + 2, (radius - 2) * 2, (radius - 2) * 2);

        if (bandVisuals[i].selected)
        {
            g.setColour(juce::Colours::white);
            g.drawEllipse(x - radius - 1, y - radius - 1, (radius + 1) * 2, (radius + 1) * 2, 1.5f);
        }

        // Bypass indicator
        if (bandVisuals[i].bypassed)
        {
            g.setColour(juce::Colour(0xFFa0a0c0).withAlpha(0.5f));
            g.drawEllipse(x - 3, y - 3, 6, 6, 2.0f);
        }

        // Solo indicator
        if (bandVisuals[i].soloed)
        {
            g.setColour(juce::Colour(0xFFe9c46a));
            g.setFont(makeBoldFont(8.0f));
            g.drawText("S", x - 4, y - radius - 10, 8, 10, juce::Justification::centred);
        }

        // Dynamic indicator
        bool dyn = apvts.getRawParameterValue("b" + id + "_dyn")->load() > 0.5f;
        if (dyn)
        {
            g.setColour(juce::Colour(0xFF2a9d8f));
            g.setFont(makeFont(7.0f));
            g.drawText("D", x + radius + 2, y - 4, 8, 8, juce::Justification::centred);
        }
    }
}

void EeqEditor::drawBandInfo(juce::Graphics& g, juce::Rectangle<float> d)
{
    g.setColour(juce::Colour(0xFF4a4a6e));
    g.setFont(makeFont(10.0f));
    int activeCount = 0;
    for (int i = 0; i < NUM_BANDS; ++i)
        if (bandVisuals[i].active) activeCount++;
    g.drawText(juce::String(activeCount) + " / " + juce::String(NUM_BANDS) + " bands",
               d.getX() + 4, d.getY() + 2, 80, 14, juce::Justification::centredLeft);

    float range = processor.getDisplayRange();
    g.drawText(juce::String((int)range) + " dB range",
               d.getRight() - 70, d.getY() + 2, 66, 14, juce::Justification::centredRight);

    // Zoom indicator
    if (hZoom > 1.05f)
    {
        g.setColour(juce::Colour(0xFFe94560).withAlpha(0.6f));
        g.setFont(makeFont(8.0f));
        g.drawText("x" + juce::String(hZoom, 1), d.getX() + 80, d.getY() + 2, 40, 14, juce::Justification::centredLeft);
    }
}

void EeqEditor::drawPianoRoll(juce::Graphics& g, juce::Rectangle<float> d)
{
    if (d.isEmpty()) return;

    int lowKey = freqToMidiKey(MIN_FREQ);
    int highKey = freqToMidiKey(MAX_FREQ);
    lowKey = juce::jmax(0, lowKey);
    highKey = juce::jmin(NUM_PIANO_KEYS - 1, highKey);

    for (int key = lowKey; key <= highKey; ++key)
    {
        float freq1 = midiKeyToFreq(key);
        float freq2 = midiKeyToFreq(key + 1);
        float x1 = freqToX(freq1, d);
        float x2 = freqToX(freq2, d);
        if (x2 < d.getX() || x1 > d.getRight()) continue;

        x1 = juce::jmax(x1, d.getX());
        x2 = juce::jmin(x2, d.getRight());

        int noteInOctave = key % 12;
        bool isBlack = (noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 ||
                         noteInOctave == 8 || noteInOctave == 10);

        if (isBlack)
            g.setColour(juce::Colour(0xFF2a2a3e));
        else
            g.setColour(juce::Colour(0xFF4a4a5e));

        g.fillRect(x1, d.getY(), x2 - x1, d.getHeight());

        g.setColour(juce::Colour(0xFF1a1a2e));
        g.drawRect(x1, d.getY(), x2 - x1, d.getHeight(), 0.5f);

        // Draw key name at C notes
        if (noteInOctave == 0 && (x2 - x1) > 14)
        {
            int octave = (key / 12) - 1;
            g.setColour(juce::Colour(0xFF8a8aae));
            g.setFont(makeFont(7.0f));
            g.drawText("C" + juce::String(octave), x1 + 1, d.getY() + 1, 20, 10, juce::Justification::centredLeft);
        }
    }
}

void EeqEditor::drawOutputMeter(juce::Graphics& g, juce::Rectangle<float> d)
{
    if (d.isEmpty()) return;
    float meterW = 8.0f;
    float meterH = d.getHeight() - 4.0f;

    g.setColour(juce::Colour(0xFF1a1a2e));
    g.fillRect(d.getX(), d.getY() + 2, meterW, meterH);
    g.fillRect(d.getX() + meterW + 2, d.getY() + 2, meterW, meterH);

    float levelL = juce::jlimit(0.0f, 1.0f, (processor.getOutputLevelL() + 60.0f) / 60.0f);
    float levelR = juce::jlimit(0.0f, 1.0f, (processor.getOutputLevelR() + 60.0f) / 60.0f);

    auto drawMeterBar = [&](float x, float level)
    {
        float h = level * meterH;
        float y = d.getY() + 2 + meterH - h;

        for (int i = 0; i < (int)h; i += 2)
        {
            float ratio = (float)i / meterH;
            juce::Colour c;
            if (ratio > 0.85f) c = juce::Colour(0xFFe94560);
            else if (ratio > 0.65f) c = juce::Colour(0xFFe9c46a);
            else c = juce::Colour(0xFF2a9d8f);
            g.setColour(c.withAlpha(0.8f));
            g.fillRect(x, y + (float)i, meterW, 1.0f);
        }
    };

    drawMeterBar(d.getX(), levelL);
    drawMeterBar(d.getX() + meterW + 2, levelR);
}

// ===================== Resized =====================

void EeqEditor::resized()
{
    auto bounds = getLocalBounds();
    auto topBar = bounds.removeFromTop(36);
    auto pianoArea = bounds.removeFromBottom(24);
    auto bottomBar = bounds.removeFromBottom(84);
    auto meterArea = bounds.removeFromRight(16);
    (void)meterArea;

    // Top bar
    int x = topBar.getX() + 48;
    presetSelector.setBounds(x, topBar.getY() + 6, 130, 24);
    savePresetBtn.setBounds(x + 135, topBar.getY() + 6, 32, 24);
    x += 180;
    procModeBox.setBounds(x, topBar.getY() + 6, 100, 24);
    x += 108;
    analyzerMode.setBounds(x, topBar.getY() + 6, 72, 24);
    x += 80;
    freezeBtn.setBounds(x, topBar.getY() + 6, 24, 24);
    x += 32;
    eqMatchBtn.setBounds(x, topBar.getY() + 6, 48, 24);
    x += 56;
    eqMatchCaptureBtn.setBounds(x, topBar.getY() + 6, 58, 24);
    x += 64;
    eqMatchApplyBtn.setBounds(x, topBar.getY() + 6, 42, 24);
    x += 50;

    auto rightSide = topBar.removeFromRight(0);
    undoBtn.setBounds(topBar.getRight() - 120, topBar.getY() + 6, 36, 24);
    redoBtn.setBounds(topBar.getRight() - 80, topBar.getY() + 6, 36, 24);
    abBtn.setBounds(topBar.getRight() - 40, topBar.getY() + 6, 24, 24);
    fullScreenBtn.setBounds(topBar.getRight() - 12, topBar.getY() + 6, 0, 0);

    // Band controls
    int cx = bottomBar.getX() + 8;
    int cy = bottomBar.getY() + 4;
    freqSlider.setBounds(cx, cy, 60, 50);
    freqLabel.setBounds(cx, cy, 60, 10);
    cx += 66;
    gainSlider.setBounds(cx, cy, 60, 50);
    gainLabel.setBounds(cx, cy, 60, 10);
    cx += 66;
    qSlider.setBounds(cx, cy, 60, 50);
    qLabel.setBounds(cx, cy, 60, 10);
    cx += 68;
    typeBox.setBounds(cx, cy + 4, 72, 20);
    typeLabel.setBounds(cx, cy, 72, 10);
    cx += 80;
    channelModeBox.setBounds(cx, cy + 4, 62, 20);
    chLabel.setBounds(cx, cy, 62, 10);
    cx += 68;
    soloBtn.setBounds(cx, cy + 4, 24, 20);
    bandBypassBtn.setBounds(cx + 28, cy + 4, 24, 20);
    dynBtn.setBounds(cx + 56, cy + 4, 34, 20);

    cx += 100;
    dynRangeLabel.setBounds(cx, cy, 36, 10);
    dynRangeSlider.setBounds(cx, cy + 10, 80, 16);
    dynThreshLabel.setBounds(cx + 88, cy, 38, 10);
    dynThreshSlider.setBounds(cx + 88, cy + 10, 80, 16);
    dynAutoBtn.setBounds(cx + 176, cy + 8, 36, 18);

    // Bottom bar
    int bx = bottomBar.getX() + 8;
    int by = bottomBar.getY() + 48;
    phaseBtn.setBounds(bx, by, 50, 22);
    bx += 58;
    autoGainBtn.setBounds(bx, by, 36, 22);
    bx += 44;
    panLabel.setBounds(bx, by, 28, 10);
    outputPanSlider.setBounds(bx, by + 12, 100, 14);
    bx += 110;
    gainScaleLabel.setBounds(bx, by, 34, 10);
    gainScaleSlider.setBounds(bx, by + 12, 100, 14);

    // Bring interactive elements to front
    presetSelector.toFront(true);
    savePresetBtn.toFront(true);
    procModeBox.toFront(true);
    analyzerMode.toFront(true);
    freqSlider.toFront(true);
    gainSlider.toFront(true);
    qSlider.toFront(true);
    typeBox.toFront(true);
    channelModeBox.toFront(true);
}

// ===================== Presets =====================

void EeqEditor::loadPreset(int index)
{
    if (index < 0 || index >= (int)factoryPresets.size()) return;
    const auto& preset = factoryPresets[index];
    auto& apvts = processor.getAPVTS();

    processor.pushUndoState();

    for (int i = 0; i < NUM_BANDS; ++i)
    {
        auto id = juce::String(i + 1);
        if (index == 0)
        {
            apvts.getParameter("b" + id + "_active")->setValueNotifyingHost(0.0f);
        }
        else if (i < (int)preset.size() / 5)
        {
            int base = i * 5;
            apvts.getParameter("b" + id + "_freq")->setValueNotifyingHost(
                apvts.getParameter("b" + id + "_freq")->convertTo0to1(preset[base]));
            apvts.getParameter("b" + id + "_gain")->setValueNotifyingHost(
                apvts.getParameter("b" + id + "_gain")->convertTo0to1(preset[base + 1]));
            apvts.getParameter("b" + id + "_q")->setValueNotifyingHost(
                apvts.getParameter("b" + id + "_q")->convertTo0to1(preset[base + 2]));
            apvts.getParameter("b" + id + "_type")->setValueNotifyingHost(
                apvts.getParameter("b" + id + "_type")->convertTo0to1((int)preset[base + 3]));
            apvts.getParameter("b" + id + "_active")->setValueNotifyingHost(
                preset[base + 4] > 0.5f ? 1.0f : 0.0f);
        }
        else
        {
            apvts.getParameter("b" + id + "_active")->setValueNotifyingHost(0.0f);
        }
    }

    selectBand(-1);
}

#include "PluginEditor.h"
#include <cmath>

static const juce::StringArray filterTypeNames = {
    "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut", "Notch", "Band Pass", "Flat Tilt", "Tilt Shelf"
};

static const juce::StringArray channelModeNames = {"Stereo", "Left", "Right", "Mid", "Side"};
static const juce::StringArray procModeNames = {"Zero Latency", "Natural Phase", "Linear Phase"};
static const juce::StringArray lpResolutionNames = {"Low (1024)", "Medium (2048)", "High (4096)", "Very High (8192)"};
static const juce::StringArray npResolutionNames = {"Low (1024)", "Medium (2048)", "High (4096)", "Very High (8192)"};
static const juce::StringArray displayRangeNames = {"3 dB", "6 dB", "12 dB", "30 dB"};
static const juce::StringArray analyzerNames = {"Pre", "Post", "Off"};

const juce::StringArray EeqEditor::slopeNames = {"6 dB", "12 dB", "18 dB", "24 dB", "30 dB", "36 dB", "42 dB", "48 dB", "96 dB", "Brickwall"};

static const juce::StringArray factoryPresetNames = {
    "Init", "Vocal Presence", "De-Esser", "Guitar Bright", "Bass Tight",
    "Drum Smash", "Master Bright", "Master Warm", "Low Pass 8k", "High Pass 80",
    "Vocal Air", "Vocal Body", "Vocal Clarity", "Male Vocal", "Female Vocal",
    "Acoustic Guitar", "Electric Guitar Rhythm", "Electric Guitar Lead", "Bass Guitar", "Kick Drum",
    "Snare Drum", "Overheads", "Room Mic", "Piano Bright", "Piano Warm",
    "Synth Pad", "Synth Lead", "Synth Bass", "Strings", "Brass",
    "Low End Cleanup", "Mid Scoop", "Presence Boost", "Air Band", "Tilt EQ",
    "Telephone Effect", "Radio Voice", "Lo-Fi", "Vintage Warmth", "Modern Polish",
    "Mastering Gentle", "Mastering Punch", "Mastering Air", "Corrective Cut", "Problem Frequency",
    "Side Chain Duck", "Parallel EQ", "M/S Width", "M/S Focus", "Linear Phase Master"
};

static const std::vector<std::vector<float>> factoryPresets = {
    {1000, 0, 0.707f, 0, false},           // Init
    {3000, 4, 1.5f, 0, true},              // Vocal Presence
    {6000, -6, 3.0f, 0, true},             // De-Esser
    {2000, 3, 1.0f, 1, true},              // Guitar Bright
    {100, 3, 0.8f, 1, true},               // Bass Tight
    {4000, 3, 0.5f, 0, true},              // Drum Smash
    {10000, 2, 0.7f, 1, true},             // Master Bright
    {200, 2, 0.8f, 1, true},               // Master Warm
    {8000, -80, 0.7f, 4, true},            // Low Pass 8k
    {80, -80, 0.7f, 3, true},              // High Pass 80
    {12000, 3, 1.0f, 1, true},             // Vocal Air
    {200, 2, 1.2f, 1, true},               // Vocal Body
    {4000, 4, 2.0f, 0, true},              // Vocal Clarity
    {150, 3, 1.0f, 1, true},               // Male Vocal
    {300, 4, 1.5f, 1, true},               // Female Vocal
    {80, -3, 0.7f, 3, true},               // Acoustic Guitar
    {3000, 2, 1.5f, 0, true},              // Electric Guitar Rhythm
    {2500, 6, 2.5f, 0, true},              // Electric Guitar Lead
    {60, 4, 1.2f, 1, true},                // Bass Guitar
    {60, 6, 2.0f, 0, true},                // Kick Drum
    {200, 3, 1.5f, 1, true},               // Snare Drum
    {10000, 2, 1.0f, 1, true},             // Overheads
    {5000, 2, 1.0f, 1, true},              // Room Mic
    {5000, 3, 1.5f, 1, true},              // Piano Bright
    {3000, 2, 1.2f, 1, true},              // Piano Warm
    {10000, 2, 0.8f, 1, true},             // Synth Pad
    {3000, 4, 2.0f, 0, true},              // Synth Lead
    {80, 4, 1.5f, 1, true},                // Synth Bass
    {12000, 3, 1.2f, 1, true},             // Strings
    {5000, 2, 1.0f, 1, true},              // Brass
    {40, -6, 0.7f, 3, true},               // Low End Cleanup
    {500, -4, 1.5f, 1, true},              // Mid Scoop
    {8000, 3, 1.5f, 1, true},              // Presence Boost
    {16000, 2, 0.7f, 1, true},             // Air Band
    {1000, -2, 1.0f, 5, true},             // Tilt EQ
    {800, -80, 0.7f, 3, true},             // Telephone Effect
    {1000, 2, 2.0f, 0, true},              // Radio Voice
    {2000, -6, 3.0f, 0, true},             // Lo-Fi
    {200, 2, 0.8f, 1, true},               // Vintage Warmth
    {10000, 1, 0.7f, 1, true},             // Modern Polish
    {10000, 1, 1.0f, 1, true},             // Mastering Gentle
    {60, 2, 1.0f, 1, true},                // Mastering Punch
    {15000, 2, 0.7f, 1, true},             // Mastering Air
    {2500, -4, 4.0f, 0, true},             // Corrective Cut
    {1500, -8, 5.0f, 0, true},             // Problem Frequency
    {80, -6, 2.0f, 0, true},               // Side Chain Duck
    {500, 3, 1.0f, 2, true},               // Parallel EQ
    {8000, 2, 1.5f, 2, true},              // M/S Width
    {3000, 3, 2.0f, 4, true},              // M/S Focus
    {10000, 1, 1.0f, 1, true},             // Linear Phase Master
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
    
    // High DPI / Retina support
    setResizeLimits(800, 550, 2400, 1400);
    setRepaintsOnMouseActivity(true);

    refreshPresetList();
    presetSelector.setSelectedId(1);
    presetSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF16213e));
    presetSelector.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    addAndMakeVisible(presetSelector);
    presetSelector.onChange = [this] { loadPreset(presetSelector.getSelectedItemIndex()); };

    savePresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF16213e));
    savePresetBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFe94560));
    addAndMakeVisible(savePresetBtn);
    savePresetBtn.onClick = [this] { showSavePresetDialog(); };

    for (const auto& name : procModeNames)
        procModeBox.addItem(name, procModeBox.getNumItems() + 1);
    procModeBox.setSelectedId(1);
    procModeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF16213e));
    procModeBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    addAndMakeVisible(procModeBox);
    procModeBox.addListener(this);

    for (const auto& name : lpResolutionNames)
        lpResolutionBox.addItem(name, lpResolutionBox.getNumItems() + 1);
    lpResolutionBox.setSelectedId(3); // High (4096) default
    lpResolutionBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF16213e));
    lpResolutionBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    addAndMakeVisible(lpResolutionBox);
    lpResolutionBox.addListener(this);

    for (const auto& name : npResolutionNames)
        npResolutionBox.addItem(name, npResolutionBox.getNumItems() + 1);
    npResolutionBox.setSelectedId(3); // High (4096) default
    npResolutionBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF16213e));
    npResolutionBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    addAndMakeVisible(npResolutionBox);
    npResolutionBox.addListener(this);

    for (const auto& name : displayRangeNames)
        displayRangeBox.addItem(name, displayRangeBox.getNumItems() + 1);
    displayRangeBox.setSelectedId(4); // 30 dB default
    displayRangeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF16213e));
    displayRangeBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    addAndMakeVisible(displayRangeBox);
    displayRangeBox.addListener(this);

    for (const auto& name : analyzerNames)
        analyzerMode.addItem(name, analyzerMode.getNumItems() + 1);
    analyzerMode.setSelectedId(1);
    analyzerMode.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF16213e));
    analyzerMode.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    addAndMakeVisible(analyzerMode);
    analyzerMode.addListener(this);

    instanceSelector.addItem("Self", 1);
    instanceSelector.setSelectedId(1);
    instanceSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF16213e));
    instanceSelector.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    addAndMakeVisible(instanceSelector);
    instanceSelector.addListener(this);

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

    pianoScaleBtn.setButtonText("Piano");
    pianoScaleBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    addAndMakeVisible(pianoScaleBtn);
    pianoScaleBtn.addListener(this);

    // === Floating band controls ===

    // Bypass
    bandBypassBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    bandBypassBtn.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xFFe94560));
    bandBypassBtn.setClickingTogglesState(true);
    addAndMakeVisible(bandBypassBtn);
    bandBypassBtn.setVisible(false);
    bandBypassBtn.addListener(this);

    // Type
    for (const auto& name : filterTypeNames)
        typeBox.addItem(name, typeBox.getNumItems() + 1);
    typeBox.setSelectedId(1);
    typeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF1a1a2e));
    typeBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    addAndMakeVisible(typeBox);
    typeBox.setVisible(false);
    typeBox.addListener(this);

    // Slope
    for (const auto& name : slopeNames)
        slopeBox.addItem(name, slopeBox.getNumItems() + 1);
    slopeBox.setSelectedId(4);
    slopeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF1a1a2e));
    slopeBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    addAndMakeVisible(slopeBox);
    slopeBox.setVisible(false);
    slopeBox.addListener(this);

    // Freq knob
    freqSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    freqSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 52, 14);
    freqSlider.setRange(20.0, 22000.0, 0.1);
    freqSlider.setTextValueSuffix(" Hz");
    freqSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFFe94560));
    freqSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFFe94560));
    freqSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFe0e0ff));
    freqSlider.setDoubleClickReturnValue(true, 1000.0);
    addAndMakeVisible(freqSlider);
    freqSlider.setVisible(false);
    freqSlider.addListener(this);

    // Gain knob
    gainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 52, 14);
    gainSlider.setRange(-30.0, 30.0, 0.01);
    gainSlider.setTextValueSuffix(" dB");
    gainSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF00b4d8));
    gainSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFF00b4d8));
    gainSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFe0e0ff));
    gainSlider.setDoubleClickReturnValue(true, 0.0);
    addAndMakeVisible(gainSlider);
    gainSlider.setVisible(false);
    gainSlider.addListener(this);

    // Q knob
    qSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    qSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 52, 14);
    qSlider.setRange(0.1, 10.0, 0.01);
    qSlider.setSkewFactor(0.4);
    qSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF533483));
    qSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFF533483));
    qSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFe0e0ff));
    qSlider.setDoubleClickReturnValue(true, 0.707);
    addAndMakeVisible(qSlider);
    qSlider.setVisible(false);
    qSlider.addListener(this);

    // Channel mode
    for (const auto& name : channelModeNames)
        channelModeBox.addItem(name, channelModeBox.getNumItems() + 1);
    channelModeBox.setSelectedId(1);
    channelModeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF1a1a2e));
    channelModeBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    addAndMakeVisible(channelModeBox);
    channelModeBox.setVisible(false);
    channelModeBox.addListener(this);

    // Gain-Q interaction
    gainQBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    gainQBtn.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xFFe9c46a));
    gainQBtn.setClickingTogglesState(true);
    addAndMakeVisible(gainQBtn);
    gainQBtn.setVisible(false);
    gainQBtn.addListener(this);

    // Prev / Next band
    prevBandBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    prevBandBtn.setClickingTogglesState(false);
    addAndMakeVisible(prevBandBtn);
    prevBandBtn.setVisible(false);
    prevBandBtn.addListener(this);

    nextBandBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    nextBandBtn.setClickingTogglesState(false);
    addAndMakeVisible(nextBandBtn);
    nextBandBtn.setVisible(false);
    nextBandBtn.addListener(this);

    // Band number
    bandNumberLabel.setJustificationType(juce::Justification::centred);
    bandNumberLabel.setFont(makeBoldFont(11.0f));
    bandNumberLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFe0e0ff));
    addAndMakeVisible(bandNumberLabel);
    bandNumberLabel.setVisible(false);

    // Delete
    deleteBandBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFe94560));
    addAndMakeVisible(deleteBandBtn);
    deleteBandBtn.setVisible(false);
    deleteBandBtn.addListener(this);

    // Invert Gain
    invertGainBtn.setButtonText("Inv");
    invertGainBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF16213e));
    invertGainBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFe94560));
    addAndMakeVisible(invertGainBtn);
    invertGainBtn.setVisible(false);
    invertGainBtn.addListener(this);

    // Dynamic EQ row
    dynRangeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dynRangeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 52, 14);
    dynRangeSlider.setRange(-30.0, 30.0, 0.1);
    dynRangeSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF2a9d8f));
    dynRangeSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFF2a9d8f));
    dynRangeSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFe0e0ff));
    dynRangeSlider.setDoubleClickReturnValue(true, 0.0);
    addAndMakeVisible(dynRangeSlider);
    dynRangeSlider.setVisible(false);
    dynRangeSlider.addListener(this);

    dynThreshSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dynThreshSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 52, 14);
    dynThreshSlider.setRange(-60.0, 0.0, 0.1);
    dynThreshSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF2a9d8f));
    dynThreshSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFF2a9d8f));
    dynThreshSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFe0e0ff));
    dynThreshSlider.setDoubleClickReturnValue(true, -20.0);
    addAndMakeVisible(dynThreshSlider);
    dynThreshSlider.setVisible(false);
    dynThreshSlider.addListener(this);

    dynAutoBtn.setButtonText("Auto");
    dynAutoBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFF2a9d8f));
    dynAutoBtn.setClickingTogglesState(true);
    dynAutoBtn.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(dynAutoBtn);
    dynAutoBtn.setVisible(false);
    dynAutoBtn.addListener(this);

    scTriggerBtn.setButtonText("SC");
    scTriggerBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFf72585));
    scTriggerBtn.setClickingTogglesState(true);
    addAndMakeVisible(scTriggerBtn);
    scTriggerBtn.setVisible(false);
    scTriggerBtn.addListener(this);

    phaseInvertBtn.setButtonText("Ø");
    phaseInvertBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFe94560));
    phaseInvertBtn.setClickingTogglesState(true);
    addAndMakeVisible(phaseInvertBtn);
    phaseInvertBtn.setVisible(false);
    phaseInvertBtn.addListener(this);

    // === Bottom bar (global) ===
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
    outputPanSlider.setDoubleClickReturnValue(true, 0.0);
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
    gainScaleSlider.setDoubleClickReturnValue(true, 1.0);
    addAndMakeVisible(gainScaleSlider);

    gainScaleLabel.setJustificationType(juce::Justification::centred);
    gainScaleLabel.setFont(makeFont(9.0f));
    gainScaleLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFa0a0c0));
    addAndMakeVisible(gainScaleLabel);

    spectrumGrabLabel.setJustificationType(juce::Justification::centred);
    spectrumGrabLabel.setFont(makeFont(9.0f));
    spectrumGrabLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFe94560));
    addAndMakeVisible(spectrumGrabLabel);
    spectrumGrabLabel.setVisible(false);

    // Advanced Auto Gain
    autoGainAdvBtn.setButtonText("Adv");
    autoGainAdvBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    addAndMakeVisible(autoGainAdvBtn);
    autoGainAdvBtn.addListener(this);

    autoGainWeightSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    autoGainWeightSlider.setRange(0.0, 1.0, 0.01);
    autoGainWeightSlider.setValue(0.5, juce::dontSendNotification);
    autoGainWeightSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFFe94560));
    autoGainWeightSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xFF2a2a4a));
    autoGainWeightSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFe0e0ff));
    autoGainWeightSlider.setDoubleClickReturnValue(true, 0.5);
    addAndMakeVisible(autoGainWeightSlider);

    autoGainWeightLabel.setJustificationType(juce::Justification::centred);
    autoGainWeightLabel.setFont(makeFont(9.0f));
    autoGainWeightLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFa0a0c0));
    addAndMakeVisible(autoGainWeightLabel);

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

    // MIDI Learn
    midiLearnBtn.setButtonText("MIDI");
    midiLearnBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF16213e));
    midiLearnBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFe94560));
    addAndMakeVisible(midiLearnBtn);
    midiLearnBtn.setVisible(false);
    midiLearnBtn.addListener(this);

    startTimerHz(30);

    if (useOpenGL)
        initializeOpenGL();
}

EeqEditor::~EeqEditor()
{
    stopTimer();
    shutdownOpenGL();
    freqSlider.removeListener(this);
    gainSlider.removeListener(this);
    qSlider.removeListener(this);
    typeBox.removeListener(this);
    channelModeBox.removeListener(this);
    slopeBox.removeListener(this);
    bandBypassBtn.removeListener(this);
    gainQBtn.removeListener(this);
    prevBandBtn.removeListener(this);
    nextBandBtn.removeListener(this);
    deleteBandBtn.removeListener(this);
    dynRangeSlider.removeListener(this);
    dynThreshSlider.removeListener(this);
    dynAutoBtn.removeListener(this);
    scTriggerBtn.removeListener(this);
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
    return getLocalBounds().toFloat().removeFromBottom(40);
}

juce::Rectangle<float> EeqEditor::getDisplayBounds() const
{
    auto b = getLocalBounds().toFloat();
    b = b.reduced(36, 0);
    b.removeFromTop(36);
    b.removeFromBottom(24);
    b.removeFromBottom(40);
    return b;
}

juce::Rectangle<float> EeqEditor::getMeterBounds() const
{
    auto b = getLocalBounds().toFloat();
    return b.removeFromRight(16).reduced(0, 36);
}

juce::Rectangle<float> EeqEditor::getBandControlsBounds() const
{
    if (selectedBand < 0) return {};

    auto display = getDisplayBounds();
    float panelW = 540.0f;
    float panelH = 56.0f;
    if (selectedBand >= 0)
    {
        auto& apvts = processor.getAPVTS();
        auto id = juce::String(selectedBand + 1);
        bool dyn = apvts.getRawParameterValue("b" + id + "_dyn")->load() > 0.5f;
        if (dyn) panelH = 86.0f;
    }

    float bandX = bandVisuals[selectedBand].x;
    float px = juce::jmax(display.getX() + 4.0f,
                          juce::jmin(bandX - panelW * 0.5f, display.getRight() - panelW - 4.0f));
    float py = display.getBottom() - panelH - 4.0f;

    return { px, py, panelW, panelH };
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

    bool showPanel = (idx >= 0 && idx < NUM_BANDS);

    freqSlider.setVisible(showPanel);
    gainSlider.setVisible(showPanel);
    qSlider.setVisible(showPanel);
    typeBox.setVisible(showPanel);
    channelModeBox.setVisible(showPanel);
    bandBypassBtn.setVisible(showPanel);
    prevBandBtn.setVisible(showPanel);
    nextBandBtn.setVisible(showPanel);
    bandNumberLabel.setVisible(showPanel);
    deleteBandBtn.setVisible(showPanel);
    invertGainBtn.setVisible(showPanel);
    gainQBtn.setVisible(showPanel);
    slopeBox.setVisible(false);
    dynRangeSlider.setVisible(false);
    dynThreshSlider.setVisible(false);
    dynAutoBtn.setVisible(false);
    scTriggerBtn.setVisible(false);

    if (showPanel)
    {
        bandVisuals[idx].selected = true;
        updateControlsFromBand(idx);
        resized();
    }
}

void EeqEditor::navigateBand(int direction)
{
    int start = selectedBand;
    for (int i = 0; i < NUM_BANDS; ++i)
    {
        int check = (start + direction + i * direction + NUM_BANDS) % NUM_BANDS;
        if (check != start && bandVisuals[check].active)
        {
            selectBand(check);
            return;
        }
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
    bool bypass = apvts.getRawParameterValue("b" + id + "_bypass")->load() > 0.5f;
    bool dyn = apvts.getRawParameterValue("b" + id + "_dyn")->load() > 0.5f;
    float dynR = apvts.getRawParameterValue("b" + id + "_dynRange")->load();
    float dynT = apvts.getRawParameterValue("b" + id + "_dynThresh")->load();
    bool dynA = apvts.getRawParameterValue("b" + id + "_dynAuto")->load() > 0.5f;
    int slopeIdx = (int)apvts.getRawParameterValue("b" + id + "_slope")->load();
    bool sc = apvts.getRawParameterValue("b" + id + "_sc")->load() > 0.5f;

    freqSlider.setValue(freqNorm, juce::dontSendNotification);
    gainSlider.setValue(gainNorm, juce::dontSendNotification);
    qSlider.setValue(qNorm, juce::dontSendNotification);
    typeBox.setSelectedId(typeIdx + 1, juce::dontSendNotification);
    channelModeBox.setSelectedId(chIdx + 1, juce::dontSendNotification);
    bandBypassBtn.setToggleState(bypass, juce::dontSendNotification);
    slopeBox.setSelectedId(slopeIdx + 1, juce::dontSendNotification);
    dynRangeSlider.setValue(dynR, juce::dontSendNotification);
    dynThreshSlider.setValue(dynT, juce::dontSendNotification);
    dynAutoBtn.setToggleState(dynA, juce::dontSendNotification);
    scTriggerBtn.setToggleState(sc, juce::dontSendNotification);
    phaseInvertBtn.setToggleState(apvts.getRawParameterValue("b" + id + "_phase")->load() > 0.5f, juce::dontSendNotification);
    bandNumberLabel.setText(juce::String(idx + 1), juce::dontSendNotification);

    bool isCutType = (typeIdx == 3 || typeIdx == 4);
    slopeBox.setVisible(isCutType);

    dynRangeSlider.setVisible(dyn);
    dynThreshSlider.setVisible(dyn);
    dynAutoBtn.setVisible(dyn);
    scTriggerBtn.setVisible(dyn);
    phaseInvertBtn.setVisible(true);
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
    apvts.getParameter("b" + id + "_slope")->setValueNotifyingHost(
        apvts.getParameter("b" + id + "_slope")->convertTo0to1(slopeBox.getSelectedId() - 1));
    apvts.getParameter("b" + id + "_sc")->setValueNotifyingHost(
        scTriggerBtn.getToggleState() ? 1.0f : 0.0f);

    apvts.getParameter("b" + id + "_dynRange")->setValueNotifyingHost(
        apvts.getParameter("b" + id + "_dynRange")->convertTo0to1((float)dynRangeSlider.getValue()));
    apvts.getParameter("b" + id + "_dynThresh")->setValueNotifyingHost(
        apvts.getParameter("b" + id + "_dynThresh")->convertTo0to1((float)dynThreshSlider.getValue()));
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

    // Click outside display
    if (!display.contains(mx, my))
    {
        auto piano = getPianoBounds();
        if (piano.contains(mx, my))
        {
            float freq = xToFreq(mx, piano);
            if (e.mods.isShiftDown())
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
                            processor.getAPVTS().getParameter("b" + id + "_gain")->convertTo0to1(0.0f));
                        processor.getAPVTS().getParameter("b" + id + "_type")->setValueNotifyingHost(
                            processor.getAPVTS().getParameter("b" + id + "_type")->convertTo0to1(0));
                        processor.getAPVTS().getParameter("b" + id + "_active")->setValueNotifyingHost(1.0f);
                        selectBand(i);
                        break;
                    }
                }
            }
            else
            {
                addBandAt(freq, 0.0f);
            }
        }
        return;
    }

    // Click on floating panel — don't deselect
    auto panelBounds = getBandControlsBounds();
    if (panelBounds.toFloat().contains(mx, my))
        return;

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
            menu.addSeparator();
            menu.addItem(201, "Make Dynamic");
            menu.addItem(202, "Invert Gain");
            menu.addSeparator();
            menu.addItem(203, "Copy Band");
            menu.addItem(204, "Paste Band");

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
                    bool s = apvts.getRawParameterValue("b" + id + "_solo")->load() > 0.5f;
                    apvts.getParameter("b" + id + "_solo")->setValueNotifyingHost(s ? 0.0f : 1.0f);
                }
                else if (result == 101)
                {
                    bool b = apvts.getRawParameterValue("b" + id + "_bypass")->load() > 0.5f;
                    apvts.getParameter("b" + id + "_bypass")->setValueNotifyingHost(b ? 0.0f : 1.0f);
                }
                else if (result == 102)
                {
                    bool d = apvts.getRawParameterValue("b" + id + "_dyn")->load() > 0.5f;
                    apvts.getParameter("b" + id + "_dyn")->setValueNotifyingHost(d ? 0.0f : 1.0f);
                }
                else if (result == 200)
                {
                    processor.pushUndoState();
                    apvts.getParameter("b" + id + "_active")->setValueNotifyingHost(0.0f);
                    bandVisuals[hit].active = false;
                    if (selectedBand == hit) selectBand(-1);
                }
                else if (result == 201)
                {
                    processor.pushUndoState();
                    bool d = apvts.getRawParameterValue("b" + id + "_dyn")->load() > 0.5f;
                    apvts.getParameter("b" + id + "_dyn")->setValueNotifyingHost(d ? 0.0f : 1.0f);
                }
                else if (result == 202)
                {
                    processor.pushUndoState();
                    auto* gainParam = apvts.getParameter("b" + id + "_gain");
                    float currentGain = gainParam->getValue();
                    gainParam->setValueNotifyingHost(gainParam->convertTo0to1(-currentGain));
                }
                else if (result == 203)
                {
                    // Copy band
                    processor.copyBandToClipboard(hit);
                }
                else if (result == 204)
                {
                    processor.pasteBandFromClipboard(hit);
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
        // Deselect all
        selectBand(-1);

        float freq = xToFreq(mx, display);
        float gain = yToGain(my, display);

        float curveMagDB = 20.0f * std::log10(std::max(processor.getEqualizer().getMagnitudeAtFreq(freq), 1e-10f));
        float distToCurve = std::abs(gain - curveMagDB);

        if (distToCurve < 4.0f)
        {
            spectrumGrabbing = true;
            spectrumGrabFreq = freq;
            spectrumGrabGain = curveMagDB;
            addBandAt(freq, curveMagDB);
            
            // Show frequency/note label
            int note = freqToMidiKey(freq);
            float noteFreq = midiKeyToFreq(note);
            int cents = (int)std::round(1200.0f * std::log2(freq / noteFreq));
            juce::String noteNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
            juce::String noteName = noteNames[note % 12] + juce::String(note / 12 - 1);
            juce::String centsStr = (cents >= 0 ? "+" : "") + juce::String(cents);
            spectrumGrabLabel.setText("Grab: " + juce::String(freq, 1) + " Hz (" + noteName + " " + centsStr + "¢)", juce::dontSendNotification);
            spectrumGrabLabel.setVisible(true);
        }
        else
        {
            addBandAt(freq, gain);
        }
    }
}

void EeqEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (spectrumGrabbing && selectedBand >= 0)
    {
        auto display = getDisplayBounds();
        float freq = xToFreq(e.position.x, display);
        
        // Update frequency/note label during drag
        int note = freqToMidiKey(freq);
        float noteFreq = midiKeyToFreq(note);
        int cents = (int)std::round(1200.0f * std::log2(freq / noteFreq));
        juce::String noteNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        juce::String noteName = noteNames[note % 12] + juce::String(note / 12 - 1);
        int centsInt = cents;
        juce::String centsStr = (centsInt >= 0 ? "+" : "") + juce::String(centsInt);
        spectrumGrabLabel.setText("Grab: " + juce::String(freq, 1) + " Hz (" + noteName + " " + centsStr + "¢)", juce::dontSendNotification);
        
        updateBandFromMouse(selectedBand, e.position.x, e.position.y);
        resized();
        return;
    }

    if (dragging && selectedBand >= 0)
    {
        updateBandFromMouse(selectedBand, e.position.x, e.position.y);

        if (e.mods.isCommandDown() && multiSelectedBands.size() > 1)
        {
            for (int b : multiSelectedBands)
            {
                if (b != selectedBand && bandVisuals[b].active)
                    updateBandFromMouse(b, e.position.x, e.position.y);
            }
        }

        resized();
    }
}

void EeqEditor::mouseUp(const juce::MouseEvent&)
{
    dragging = false;
    spectrumGrabbing = false;
    spectrumGrabLabel.setVisible(false);
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
        float range = processor.getDisplayRange();
        range = juce::jlimit(6.0f, 60.0f, range - wheel.deltaY * 3.0f);
        processor.setDisplayRange(range);
        return;
    }

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
    if (key == juce::KeyPress('b', false, false))
    {
        if (selectedBand >= 0)
        {
            auto id = juce::String(selectedBand + 1);
            bool b = processor.getAPVTS().getRawParameterValue("b" + id + "_bypass")->load() > 0.5f;
            processor.getAPVTS().getParameter("b" + id + "_bypass")->setValueNotifyingHost(b ? 0.0f : 1.0f);
        }
        return true;
    }
    if (key == juce::KeyPress('s', false, false))
    {
        if (selectedBand >= 0)
        {
            auto id = juce::String(selectedBand + 1);
            bool s = processor.getAPVTS().getRawParameterValue("b" + id + "_solo")->load() > 0.5f;
            processor.getAPVTS().getParameter("b" + id + "_solo")->setValueNotifyingHost(s ? 0.0f : 1.0f);
        }
        return true;
    }
    if (key == juce::KeyPress('d', false, false))
    {
        if (selectedBand >= 0)
        {
            processor.pushUndoState();
            auto id = juce::String(selectedBand + 1);
            bool d = processor.getAPVTS().getRawParameterValue("b" + id + "_dyn")->load() > 0.5f;
            processor.getAPVTS().getParameter("b" + id + "_dyn")->setValueNotifyingHost(d ? 0.0f : 1.0f);
        }
        return true;
    }
    if (key == juce::KeyPress::leftKey)
    {
        navigateBand(-1);
        return true;
    }
    if (key == juce::KeyPress::rightKey)
    {
        navigateBand(1);
        return true;
    }
    if (key == juce::KeyPress::upKey)
    {
        if (selectedBand >= 0)
        {
            auto id = juce::String(selectedBand + 1);
            float gain = processor.getAPVTS().getRawParameterValue("b" + id + "_gain")->load();
            processor.getAPVTS().getParameter("b" + id + "_gain")->setValueNotifyingHost(
                processor.getAPVTS().getParameter("b" + id + "_gain")->convertTo0to1(gain + 0.5f));
        }
        return true;
    }
    if (key == juce::KeyPress::downKey)
    {
        if (selectedBand >= 0)
        {
            auto id = juce::String(selectedBand + 1);
            float gain = processor.getAPVTS().getRawParameterValue("b" + id + "_gain")->load();
            processor.getAPVTS().getParameter("b" + id + "_gain")->setValueNotifyingHost(
                processor.getAPVTS().getParameter("b" + id + "_gain")->convertTo0to1(gain - 0.5f));
        }
        return true;
    }
    return false;
}

// ===================== Listeners =====================

void EeqEditor::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &autoGainWeightSlider)
    {
        processor.setAutoGainChannelWeight((float)autoGainWeightSlider.getValue());
    }
    else if (selectedBand >= 0)
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
    else if (box == &lpResolutionBox)
    {
        processor.setLinearPhaseResolution((LinearPhaseResolution)(lpResolutionBox.getSelectedId() - 1));
    }
    else if (box == &npResolutionBox)
    {
        processor.setNaturalPhaseResolution((NaturalPhaseResolution)(npResolutionBox.getSelectedId() - 1));
    }
    else if (box == &displayRangeBox)
    {
        float ranges[] = {3.0f, 6.0f, 12.0f, 30.0f};
        processor.setDisplayRange(ranges[displayRangeBox.getSelectedId() - 1]);
    }
    else if (box == &instanceSelector)
    {
        // Instance selector changed - update spectrum analyzer source
        int selectedId = instanceSelector.getSelectedId();
        if (selectedId == 1)
        {
            // Self - use own spectrum
            processor.getSpectrumAnalyzer().setExternalSpectrum({}, 0);
        }
        else
        {
            // External instance
            auto& instances = processor.getVisibleInstances();
            int index = selectedId - 2;
            if (index >= 0 && index < (int)instances.size())
            {
                auto* info = instances[index];
                if (info->hasSpectrum)
                {
                    processor.getSpectrumAnalyzer().setExternalSpectrum(info->spectrum, info->spectrum.size());
                }
            }
        }
    }
    else if (box == &typeBox || box == &channelModeBox || box == &slopeBox)
    {
        if (selectedBand >= 0)
        {
            processor.pushUndoState();
            updateBandFromControls(selectedBand);
            if (box == &typeBox)
            {
                int typeIdx = typeBox.getSelectedId() - 1;
                bool isCutType = (typeIdx == 3 || typeIdx == 4);
                slopeBox.setVisible(isCutType);
            }
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
        fullScreenBtn.setToggleState(fullScreen, juce::dontSendNotification);
        if (fullScreen)
        {
            previousBounds = getBounds();
        }
    }
    else if (btn == &pianoScaleBtn)
    {
        pianoScale = !pianoScale;
        pianoScaleBtn.setToggleState(pianoScale, juce::dontSendNotification);
        repaint();
    }
    else if (btn == &fullScreenBtn)
    {
        fullScreen = !fullScreen;
        fullScreenBtn.setToggleState(fullScreen, juce::dontSendNotification);
        if (fullScreen)
        {
            previousBounds = getBounds();
            auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
            if (display != nullptr)
                setBounds(display->totalArea);
        }
        else
        {
            setBounds(previousBounds);
        }
    }
    else if (btn == &phaseBtn)
        processor.setPhaseInverted(phaseBtn.getToggleState());
    else if (btn == &autoGainBtn)
        processor.setAutoGainEnabled(autoGainBtn.getToggleState());
    else if (btn == &autoGainAdvBtn)
        processor.setAutoGainAdvanced(autoGainAdvBtn.getToggleState());

    else if (btn == &bandBypassBtn && selectedBand >= 0)
    {
        auto id = juce::String(selectedBand + 1);
        processor.getAPVTS().getParameter("b" + id + "_bypass")->setValueNotifyingHost(
            bandBypassBtn.getToggleState() ? 1.0f : 0.0f);
        bandVisuals[selectedBand].bypassed = bandBypassBtn.getToggleState();
    }
    else if (btn == &prevBandBtn)
    {
        navigateBand(-1);
    }
    else if (btn == &nextBandBtn)
    {
        navigateBand(1);
    }
    else if (btn == &deleteBandBtn && selectedBand >= 0)
    {
        processor.pushUndoState();
        auto id = juce::String(selectedBand + 1);
        processor.getAPVTS().getParameter("b" + id + "_active")->setValueNotifyingHost(0.0f);
        bandVisuals[selectedBand].active = false;
        selectBand(-1);
    }
    else if (btn == &invertGainBtn && selectedBand >= 0)
    {
        processor.pushUndoState();
        auto id = juce::String(selectedBand + 1);
        auto* gainParam = processor.getAPVTS().getParameter("b" + id + "_gain");
        float currentGain = gainParam->getValue();
        gainParam->setValueNotifyingHost(gainParam->convertTo0to1(-currentGain));
    }
    else if (btn == &dynAutoBtn && selectedBand >= 0)
    {
        auto id = juce::String(selectedBand + 1);
        processor.getAPVTS().getParameter("b" + id + "_dynAuto")->setValueNotifyingHost(
            dynAutoBtn.getToggleState() ? 1.0f : 0.0f);
    }
    else if (btn == &scTriggerBtn && selectedBand >= 0)
    {
        auto id = juce::String(selectedBand + 1);
        processor.getAPVTS().getParameter("b" + id + "_sc")->setValueNotifyingHost(
            scTriggerBtn.getToggleState() ? 1.0f : 0.0f);
    }
    else if (btn == &phaseInvertBtn && selectedBand >= 0)
    {
        auto id = juce::String(selectedBand + 1);
        processor.getAPVTS().getParameter("b" + id + "_phase")->setValueNotifyingHost(
            phaseInvertBtn.getToggleState() ? 1.0f : 0.0f);
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
    else if (btn == &midiLearnBtn)
    {
        midiLearnActive = !midiLearnActive;
        midiLearnBtn.setToggleState(midiLearnActive, juce::dontSendNotification);
        midiLearnBtn.setButtonText(midiLearnActive ? "LEARN" : "MIDI");
        midiLearnBtn.setColour(juce::TextButton::buttonColourId, midiLearnActive ? juce::Colour(0xFFe94560) : juce::Colour(0xFF16213e));
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
    drawBandControls(g, display);
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

        // Clamp very small values to zero to prevent low-end flicker
        if (mag < 0.005f) mag = 0.0f;

        float x = d.getX() + (float)px;
        float y = d.getY() + d.getHeight() * (1.0f - mag);

        if (!started) { path.startNewSubPath(x, y); started = true; }
        else path.lineTo(x, y);
    }

    juce::Path filledPath(path);
    filledPath.lineTo(d.getRight(), d.getBottom());
    filledPath.lineTo(d.getX(), d.getBottom());
    filledPath.closeSubPath();

    // Spectrum gradient coloring by frequency
    for (int px = 0; px < (int)d.getWidth(); px += 3)
    {
        float freq = xToFreq((float)px, d);
        float norm = (std::log10(std::max(freq, 20.0f)) - std::log10(20.0f))
                   / (std::log10(22000.0f) - std::log10(20.0f));
        norm = juce::jlimit(0.0f, 1.0f, norm);

        float r, gr, b;
        if (norm < 0.33f) { r = norm * 3.0f; gr = 0.6f + norm; b = 0.2f; }
        else if (norm < 0.66f) { r = 1.0f; gr = 1.0f - (norm - 0.33f) * 1.5f; b = 0.2f; }
        else { r = 1.0f - (norm - 0.66f); gr = 0.2f; b = 0.3f + norm; }

        g.setColour(juce::Colour::fromFloatRGBA(r, gr, b, 0.06f));
        g.fillRect(d.getX() + (float)px, d.getY(), 3.0f, d.getHeight());
    }

    g.setColour(juce::Colour(0xFF00ff88).withAlpha(0.08f));
    g.fillPath(filledPath);
    g.setColour(juce::Colour(0xFF00ff88).withAlpha(0.5f));
    g.strokePath(path, juce::PathStrokeType(1.5f));

    const auto& peaks = processor.getSpectrumAnalyzer().peakHold;
    for (int px = 0; px < (int)d.getWidth(); px += 2)
    {
        float freq = xToFreq((float)px, d);
        if (freq < MIN_FREQ || freq > MAX_FREQ) continue;
        int bin = (int)(freq / nyquist * (float)numBins);
        bin = juce::jlimit(0, numBins - 1, bin);
        float peak = peaks[bin];
        if (peak < 0.005f) continue;
        float y = d.getY() + d.getHeight() * (1.0f - peak);
        g.setColour(juce::Colour(0xFF00ff88).withAlpha(0.3f));
        g.drawLine(d.getX() + (float)px, y, d.getX() + (float)px + 1.0f, y, 1.0f);
    }

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

        if (bandVisuals[i].bypassed)
        {
            g.setColour(juce::Colour(0xFFa0a0c0).withAlpha(0.5f));
            g.drawEllipse(x - 3, y - 3, 6, 6, 2.0f);
        }

        if (bandVisuals[i].soloed)
        {
            g.setColour(juce::Colour(0xFFe9c46a));
            g.setFont(makeBoldFont(8.0f));
            g.drawText("S", x - 4, y - radius - 10, 8, 10, juce::Justification::centred);
        }

        bool dyn = apvts.getRawParameterValue("b" + id + "_dyn")->load() > 0.5f;
        if (dyn)
        {
            g.setColour(juce::Colour(0xFF2a9d8f));
            g.setFont(makeFont(7.0f));
            g.drawText("D", x + radius + 2, y - 4, 8, 8, juce::Justification::centred);
        }

        bool sc = apvts.getRawParameterValue("b" + id + "_sc")->load() > 0.5f;
        if (sc)
        {
            g.setColour(juce::Colour(0xFFf72585));
            g.setFont(makeFont(7.0f));
            g.drawText("SC", x + radius + 2, y + 4, 12, 8, juce::Justification::centred);
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

    if (hZoom > 1.05f)
    {
        g.setColour(juce::Colour(0xFFe94560).withAlpha(0.6f));
        g.setFont(makeFont(8.0f));
        g.drawText("x" + juce::String(hZoom, 1), d.getX() + 80, d.getY() + 2, 40, 14, juce::Justification::centredLeft);
    }
}

// ===================== Floating Band Controls Panel =====================

void EeqEditor::drawBandControls(juce::Graphics& g, juce::Rectangle<float> display)
{
    if (selectedBand < 0) return;
    auto panel = getBandControlsBounds();
    if (panel.isEmpty()) return;

    // Draw semi-transparent background
    g.setColour(juce::Colour(0xDD12121e));
    g.fillRoundedRectangle(panel, 6.0f);
    g.setColour(juce::Colour(0xFFe94560).withAlpha(0.4f));
    g.drawRoundedRectangle(panel, 6.0f, 1.0f);

    // Draw vertical connector line from band node to panel
    if (selectedBand >= 0 && selectedBand < NUM_BANDS && bandVisuals[selectedBand].active)
    {
        float bandX = bandVisuals[selectedBand].x;
        float bandY = bandVisuals[selectedBand].y;
        g.setColour(juce::Colour(0xFFe94560).withAlpha(0.3f));
        g.drawLine(bandX, bandY + qToRadius(0.707f), bandX, panel.getY(), 1.0f);
    }

    auto col = bandColours[selectedBand % 24];

    // Small colour indicator
    g.setColour(col);
    g.fillRoundedRectangle(panel.getX() + 8, panel.getY() + 6, 4, panel.getHeight() - 12, 2.0f);

    // Labels above knobs (row 1)
    float ly = panel.getY() + 4;
    g.setColour(juce::Colour(0xFF8a8aae));
    g.setFont(makeFont(8.0f));

    // We don't draw text labels for knobs — the knobs have TextBoxBelow
    // Just draw small labels for the combo boxes
    auto typeArea = panel.reduced(0).removeFromTop(panel.getHeight());
    (void)typeArea;
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

        if (noteInOctave == 0 && (x2 - x1) > 14)
        {
            int octave = (key / 12) - 1;
            g.setColour(juce::Colour(0xFF8a8aae));
            g.setFont(makeFont(7.0f));
            g.drawText("C" + juce::String(octave), x1 + 1, d.getY() + 1, 20, 10, juce::Justification::centredLeft);
        }

        // Show frequency labels when piano scale is enabled
        if (pianoScale && (x2 - x1) > 20)
        {
            float freq = midiKeyToFreq(key);
            juce::String freqText;
            if (freq >= 1000.0f)
                freqText = juce::String(freq / 1000.0f, 1) + "k";
            else
                freqText = juce::String((int)freq);

            g.setColour(juce::Colour(0xFF8a8aae).withAlpha(0.7f));
            g.setFont(makeFont(6.0f));
            g.drawText(freqText, x1 + 1, d.getY() + d.getHeight() - 10, (int)(x2 - x1) - 2, 10, juce::Justification::centred);
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
    auto bottomBar = bounds.removeFromBottom(40);
    auto meterArea = bounds.removeFromRight(16);
    (void)meterArea;

    // Top bar
    int x = topBar.getX() + 48;
    presetSelector.setBounds(x, topBar.getY() + 6, 130, 24);
    savePresetBtn.setBounds(x + 135, topBar.getY() + 6, 32, 24);
    x += 180;
    procModeBox.setBounds(x, topBar.getY() + 6, 100, 24);
    x += 108;
    lpResolutionBox.setBounds(x, topBar.getY() + 6, 110, 24);
    x += 118;
    npResolutionBox.setBounds(x, topBar.getY() + 6, 110, 24);
    x += 118;
    displayRangeBox.setBounds(x, topBar.getY() + 6, 60, 24);
    x += 68;
    analyzerMode.setBounds(x, topBar.getY() + 6, 72, 24);
    x += 80;
    instanceSelector.setBounds(x, topBar.getY() + 6, 90, 24);
    x += 98;
    freezeBtn.setBounds(x, topBar.getY() + 6, 24, 24);
    x += 32;
    eqMatchBtn.setBounds(x, topBar.getY() + 6, 48, 24);
    x += 56;
    eqMatchCaptureBtn.setBounds(x, topBar.getY() + 6, 58, 24);
    x += 64;
    eqMatchApplyBtn.setBounds(x, topBar.getY() + 6, 42, 24);
    x += 48;
    midiLearnBtn.setBounds(x, topBar.getY() + 6, 36, 24);
    x += 40;
    pianoScaleBtn.setBounds(x, topBar.getY() + 6, 42, 24);
    x += 44;
    spectrumGrabLabel.setBounds(x, topBar.getY() + 6, 200, 24);

    undoBtn.setBounds(topBar.getRight() - 120, topBar.getY() + 6, 36, 24);
    redoBtn.setBounds(topBar.getRight() - 80, topBar.getY() + 6, 36, 24);
    abBtn.setBounds(topBar.getRight() - 40, topBar.getY() + 6, 24, 24);

    // Bottom bar (global only)
    int bx = bottomBar.getX() + 8;
    int by = bottomBar.getY() + 8;
    phaseBtn.setBounds(bx, by, 50, 22);
    bx += 58;
    autoGainBtn.setBounds(bx, by, 36, 22);
    bx += 44;
    panLabel.setBounds(bx, by - 2, 28, 10);
    outputPanSlider.setBounds(bx, by + 10, 100, 14);
    bx += 110;
    gainScaleLabel.setBounds(bx, by - 2, 34, 10);
    gainScaleSlider.setBounds(bx, by + 10, 100, 14);
    bx += 110;
    autoGainWeightLabel.setBounds(bx, by - 2, 34, 10);
    autoGainWeightSlider.setBounds(bx, by + 10, 100, 14);
    bx += 110;
    autoGainAdvBtn.setBounds(bx, by, 26, 22);
    bx += 30;
    phaseBtn.setBounds(bx, by, 50, 22);

    // === Floating band controls layout ===
    auto panel = getBandControlsBounds();
    if (!panel.isEmpty() && selectedBand >= 0)
    {
        float px = panel.getX() + 18.0f;
        float py = panel.getY() + 4.0f;
        float knobW = 52.0f;
        float knobH = 48.0f;
        float smallBtn = 20.0f;

        // Row 1: Bypass | Type | Slope | Freq | Gain | Q | Ch | GQ | Prev | # | Next | Del
        bandBypassBtn.setBounds(px, py, smallBtn, smallBtn);
        px += 24;
        typeBox.setBounds(px, py, 68, smallBtn);
        px += 72;
        slopeBox.setBounds(px, py, 48, smallBtn);
        px += 52;
        freqSlider.setBounds(px, py - 2, knobW, knobH);
        px += knobW + 4;
        gainSlider.setBounds(px, py - 2, knobW, knobH);
        px += knobW + 4;
        qSlider.setBounds(px, py - 2, knobW, knobH);
        px += knobW + 4;
        channelModeBox.setBounds(px, py, 56, smallBtn);
        px += 60;
        gainQBtn.setBounds(px, py, smallBtn, smallBtn);
        px += 24;
        prevBandBtn.setBounds(px, py + 2, 18, 16);
        bandNumberLabel.setBounds(px + 20, py, 16, smallBtn);
        nextBandBtn.setBounds(px + 38, py + 2, 18, 16);
        px += 62;
        deleteBandBtn.setBounds(px, py, smallBtn, smallBtn);
        px += 26;
        invertGainBtn.setBounds(px, py, smallBtn, smallBtn);

        // Row 2 (dynamic EQ): Range | Thresh | Auto | SC | Phase
        if (dynRangeSlider.isVisible())
        {
            float dy = py + knobH - 2;
            dynRangeSlider.setBounds(panel.getX() + 18, dy, knobW, knobH - 8);
            dynThreshSlider.setBounds(panel.getX() + 18 + knobW + 4, dy, knobW, knobH - 8);
            dynAutoBtn.setBounds(panel.getX() + 18 + (knobW + 4) * 2, dy + 8, 36, 18);
            scTriggerBtn.setBounds(panel.getX() + 18 + (knobW + 4) * 2 + 40, dy + 8, 28, 18);
            phaseInvertBtn.setBounds(panel.getX() + 18 + (knobW + 4) * 2 + 72, dy + 8, 24, 18);
        }
    }

    // Bring interactive elements to front
    presetSelector.toFront(true);
    savePresetBtn.toFront(true);
    procModeBox.toFront(true);
    lpResolutionBox.toFront(true);
    npResolutionBox.toFront(true);
    displayRangeBox.toFront(true);
    analyzerMode.toFront(true);
    instanceSelector.toFront(true);
    freqSlider.toFront(true);
    gainSlider.toFront(true);
    qSlider.toFront(true);
    typeBox.toFront(true);
    channelModeBox.toFront(true);
    slopeBox.toFront(true);
    phaseInvertBtn.toFront(true);
    midiLearnBtn.toFront(true);
    pianoScaleBtn.toFront(true);
    invertGainBtn.toFront(true);
    spectrumGrabLabel.toFront(true);
    autoGainAdvBtn.toFront(true);
    autoGainWeightSlider.toFront(true);
    autoGainWeightLabel.toFront(true);
}

// ===================== Presets =====================

void EeqEditor::refreshPresetList()
{
    presetSelector.clear();
    int id = 1;
    for (const auto& name : factoryPresetNames)
        presetSelector.addItem("Factory: " + name, id++);

    auto userPresets = processor.getUserPresetNames();
    if (!userPresets.isEmpty())
    {
        presetSelector.addItem("--- User Presets ---", id++);
        for (const auto& name : userPresets)
            presetSelector.addItem(name, id++);
    }
}

void EeqEditor::showSavePresetDialog()
{
    class PresetNameDialog : public juce::AlertWindow
    {
    public:
        PresetNameDialog() : AlertWindow("Save Preset", "Enter preset name:", AlertWindow::NoIcon)
        {
            addTextEditor("name", "", "Preset name:");
            addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
            addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        }
        
        juce::String getName() const { return getTextEditorContents("name"); }
    };

    std::unique_ptr<PresetNameDialog> dialog(new PresetNameDialog());
    dialog->enterModalState(true, juce::ModalCallbackFunction::create([this, dialog = dialog.get()](int result)
    {
        if (result == 1 && dialog)
        {
            juce::String name = dialog->getName().trim();
            if (name.isNotEmpty())
            {
                processor.saveUserPreset(name);
                refreshPresetList();
                // Select the newly saved preset
                for (int i = 1; i <= presetSelector.getNumItems(); ++i)
                {
                    if (presetSelector.getItemText(i) == name)
                    {
                        presetSelector.setSelectedId(i);
                        break;
                    }
                }
            }
        }
    }));
}

void EeqEditor::loadPreset(int index)
{
    if (index < 0) return;

    // Check if it's a separator
    auto text = presetSelector.getItemText(index + 1);
    if (text.startsWith("---") || text.startsWith("Factory:"))
    {
        // Handle factory presets
        int factoryIndex = 0;
        for (const auto& name : factoryPresetNames)
        {
            if (text == "Factory: " + name)
            {
                loadFactoryPreset(factoryIndex);
                return;
            }
            factoryIndex++;
        }
        return;
    }

    // User preset
    processor.loadUserPreset(text);
    updateAllControlsFromProcessor();
}

void EeqEditor::loadFactoryPreset(int index)
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

void EeqEditor::updateAllControlsFromProcessor()
{
    for (int i = 0; i < NUM_BANDS; ++i)
    {
        if (selectedBand == i || multiSelectedBands.empty() || std::find(multiSelectedBands.begin(), multiSelectedBands.end(), i) != multiSelectedBands.end())
        {
            updateBandFromControls(i);
        }
    }
    selectBand(-1);
}

void EeqEditor::initializeOpenGL()
{
    openGLContext.setRenderer(this);
    openGLContext.attachTo(*this);
    openGLContext.setContinuousRepainting(true);
}

void EeqEditor::shutdownOpenGL()
{
    openGLContext.detach();
}

void EeqEditor::newOpenGLContextCreated()
{
    // Initialize OpenGL resources here (shaders, textures, etc.)
}

void EeqEditor::renderOpenGL()
{
    // OpenGL rendering - the paint() method will be called with OpenGL graphics context
    // when using OpenGLContext with continuous repainting
}

void EeqEditor::openGLContextClosing()
{
    // Clean up OpenGL resources here
}

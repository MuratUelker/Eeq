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
    {4500, 2.5f, 0.9f, 0, true},           // Vocal Presence (bell)
    {6000, -4, 1.2f, 0, true},             // De-Esser (bell)
    {4000, 2.5f, 0.8f, 2, true},           // Guitar Bright (high shelf)
    {110, 2, 0.7f, 1, true},               // Bass Tight (low shelf)
    {4000, 3, 1.0f, 0, true},              // Drum Smash (bell)
    {12000, 2, 0.7f, 2, true},             // Master Bright (high shelf)
    {150, 2, 0.7f, 1, true},               // Master Warm (low shelf)
    {8000, 0, 0.7f, 4, true},              // Low Pass 8k (high cut)
    {80, 0, 0.7f, 3, true},                // High Pass 80 (low cut)
    {12000, 2.5f, 0.7f, 2, true},          // Vocal Air (high shelf)
    {250, 2, 0.8f, 1, true},               // Vocal Body (low shelf)
    {3000, 3, 1.0f, 0, true},              // Vocal Clarity (bell)
    {150, 2, 0.8f, 1, true},               // Male Vocal (low shelf)
    {350, 2.5f, 0.8f, 1, true},            // Female Vocal (low shelf)
    {90, 0, 0.7f, 3, true},                // Acoustic Guitar (low cut)
    {2500, 2.5f, 1.0f, 0, true},           // Electric Guitar Rhythm (bell)
    {3000, 4, 1.2f, 0, true},              // Electric Guitar Lead (bell)
    {80, 3, 0.8f, 1, true},                // Bass Guitar (low shelf)
    {65, 3, 1.0f, 0, true},                // Kick Drum (bell)
    {200, 2.5f, 0.9f, 0, true},            // Snare Drum (bell)
    {10000, 2, 0.8f, 2, true},             // Overheads (high shelf)
    {9000, 1.5f, 0.8f, 2, true},           // Room Mic (high shelf)
    {8000, 2.5f, 0.8f, 2, true},           // Piano Bright (high shelf)
    {300, 1.5f, 0.8f, 1, true},            // Piano Warm (low shelf)
    {15000, 2, 0.7f, 2, true},             // Synth Pad (high shelf)
    {2000, 3, 1.5f, 0, true},              // Synth Lead (bell)
    {90, 3, 0.8f, 1, true},                // Synth Bass (low shelf)
    {6000, 2, 0.8f, 2, true},              // Strings (high shelf)
    {4000, 2.5f, 0.8f, 2, true},           // Brass (high shelf)
    {35, 0, 0.7f, 3, true},                // Low End Cleanup (low cut)
    {500, -3, 1.2f, 0, true},              // Mid Scoop (bell)
    {8000, 2.5f, 0.8f, 2, true},           // Presence Boost (high shelf)
    {16000, 2, 0.7f, 2, true},             // Air Band (high shelf)
    {1000, 4, 1.0f, 7, true},              // Tilt EQ (flat tilt)
    {300, 0, 0.707f, 3, true},             // Telephone Effect (low cut @300)
    {1000, 3, 1.2f, 0, true},              // Radio Voice (bell)
    {2000, -3, 0.8f, 0, true},             // Lo-Fi (bell)
    {150, 2, 0.8f, 1, true},               // Vintage Warmth (low shelf)
    {12000, 1.5f, 0.8f, 2, true},          // Modern Polish (high shelf)
    {12000, 1, 0.7f, 2, true},             // Mastering Gentle (high shelf)
    {60, 2, 0.7f, 1, true},                // Mastering Punch (low shelf)
    {16000, 2, 0.7f, 2, true},             // Mastering Air (high shelf)
    {2500, -3, 1.0f, 0, true},             // Corrective Cut (bell)
    {3000, -4, 2.0f, 0, true},             // Problem Frequency (bell)
    {200, -4, 0.8f, 0, true},              // Side Chain Duck (bell)
    {5000, 2, 0.8f, 2, true},              // Parallel EQ (high shelf)
    {8000, 1.5f, 0.8f, 2, true},           // M/S Width (high shelf)
    {3000, 3, 1.0f, 0, true},              // M/S Focus (bell)
    {11000, 1, 0.7f, 2, true},             // Linear Phase Master (high shelf)
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
    
    // Tooltips for all controls
    tooltipWindow.setMillisecondsBeforeTipAppears(400);
    
    // High DPI / Retina support
    setResizeLimits(800, 550, 2400, 1400);
    setRepaintsOnMouseActivity(true);

    refreshPresetList();
    presetSelector.setSelectedId(1, juce::dontSendNotification);
    presetSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF16213e));
    presetSelector.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    presetSelector.setTooltip("Select a preset");
    addAndMakeVisible(presetSelector);
    presetSelector.onChange = [this] { loadPreset(presetSelector.getSelectedItemIndex()); };
    presetSelector.onPopupSelection = [this](int itemId) { loadPresetById(itemId); };

    savePresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF16213e));
    savePresetBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFe94560));
    savePresetBtn.setTooltip("Save the current settings as a new preset");
    addAndMakeVisible(savePresetBtn);
    savePresetBtn.onClick = [this] { showPresetMenu(); };

    for (const auto& name : procModeNames)
        procModeBox.addItem(name, procModeBox.getNumItems() + 1);
    procModeBox.setSelectedId(1);
    procModeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF16213e));
    procModeBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    procModeBox.setTooltip("Processing mode: Zero Latency / Natural Phase / Linear Phase");
    addAndMakeVisible(procModeBox);
    procModeBox.addListener(this);

    for (const auto& name : lpResolutionNames)
        lpResolutionBox.addItem(name, lpResolutionBox.getNumItems() + 1);
    lpResolutionBox.setSelectedId(3); // High (4096) default
    lpResolutionBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF16213e));
    lpResolutionBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    lpResolutionBox.setTooltip("Linear phase FFT resolution");
    addAndMakeVisible(lpResolutionBox);
    lpResolutionBox.addListener(this);

    for (const auto& name : npResolutionNames)
        npResolutionBox.addItem(name, npResolutionBox.getNumItems() + 1);
    npResolutionBox.setSelectedId(3); // High (4096) default
    npResolutionBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF16213e));
    npResolutionBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    npResolutionBox.setTooltip("Natural phase FFT resolution");
    addAndMakeVisible(npResolutionBox);
    npResolutionBox.addListener(this);

    for (const auto& name : displayRangeNames)
        displayRangeBox.addItem(name, displayRangeBox.getNumItems() + 1);
    displayRangeBox.setSelectedId(1); // 3 dB default (kullanıcı ile uyumlu)
    displayRangeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF16213e));
    displayRangeBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    displayRangeBox.setTooltip("Vertical zoom of the EQ display");
    addAndMakeVisible(displayRangeBox);
    displayRangeBox.addListener(this);

    for (const auto& name : analyzerNames)
        analyzerMode.addItem(name, analyzerMode.getNumItems() + 1);
    analyzerMode.setSelectedId(1);
    analyzerMode.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF16213e));
    analyzerMode.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    analyzerMode.setTooltip("Analyzer source: pre-EQ / post-EQ / off");
    addAndMakeVisible(analyzerMode);
    analyzerMode.addListener(this);

    instanceSelector.addItem("Self", 1);
    instanceSelector.setSelectedId(1);
    instanceSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF16213e));
    instanceSelector.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    instanceSelector.setTooltip("Select which instance this panel controls");
    addAndMakeVisible(instanceSelector);
    instanceSelector.addListener(this);

    freezeBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    freezeBtn.setClickingTogglesState(true);
    freezeBtn.setTooltip("Freeze the spectrum analyzer display");
    addAndMakeVisible(freezeBtn);
    freezeBtn.addListener(this);

    abBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFe94560));
    abBtn.setClickingTogglesState(true);
    abBtn.setTooltip("A/B compare two EQ settings");
    addAndMakeVisible(abBtn);
    abBtn.addListener(this);

    undoBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFa0a0c0));
    undoBtn.setTooltip("Undo the last change (Cmd+Z)");
    addAndMakeVisible(undoBtn);
    undoBtn.addListener(this);

    redoBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFa0a0c0));
    redoBtn.setTooltip("Redo the last undone change (Cmd+Shift+Z)");
    addAndMakeVisible(redoBtn);
    redoBtn.addListener(this);

    fullScreenBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    fullScreenBtn.setTooltip("Toggle full screen");
    addAndMakeVisible(fullScreenBtn);
    fullScreenBtn.addListener(this);

    pianoScaleBtn.setButtonText("Piano");
    pianoScaleBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    pianoScaleBtn.setTooltip("Show a piano keyboard scale on the frequency axis");
    addAndMakeVisible(pianoScaleBtn);
    pianoScaleBtn.addListener(this);

    // === Floating band controls ===

    // Bypass
    bandBypassBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    bandBypassBtn.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xFFe94560));
    bandBypassBtn.setClickingTogglesState(true);
    bandBypassBtn.setTooltip("Bypass / enable this EQ band");
    addAndMakeVisible(bandBypassBtn);
    bandBypassBtn.setVisible(false);
    bandBypassBtn.addListener(this);

    // Type
    for (const auto& name : filterTypeNames)
        typeBox.addItem(name, typeBox.getNumItems() + 1);
    typeBox.setSelectedId(1);
    typeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF1a1a2e));
    typeBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    typeBox.setTooltip("Filter type for this EQ band");
    addAndMakeVisible(typeBox);
    typeBox.setVisible(false);
    typeBox.addListener(this);

    // Slope
    for (const auto& name : slopeNames)
        slopeBox.addItem(name, slopeBox.getNumItems() + 1);
    slopeBox.setSelectedId(4);
    slopeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF1a1a2e));
    slopeBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    slopeBox.setTooltip("Filter slope (for Low Cut / High Cut types)");
    addAndMakeVisible(slopeBox);
    slopeBox.setVisible(false);
    slopeBox.addListener(this);

    // Freq knob - modern UI: larger text box (80x16), visible Hz value
    freqSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    freqSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 104, 22);
    freqSlider.setRange(20.0, 22000.0, 0.1);
    freqSlider.setTextValueSuffix(" Hz");
    freqSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFFe94560));
    freqSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFFe94560));
    freqSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFffffff));
    freqSlider.setNumDecimalPlacesToDisplay(0);

    qSlider.setVelocityBasedMode(true);
    qSlider.setDoubleClickReturnValue(true, 0.707);
    qSlider.setVelocityModeParameters(0.35, 2, 0.0);
    freqSlider.setVelocityBasedMode(true);
    freqSlider.setVelocityModeParameters(0.35, 2, 0.0);
    freqSlider.setDoubleClickReturnValue(true, 1000.0);
    freqSlider.setTooltip("Frequency of this EQ band (double-click to reset to 1 kHz)");
    addAndMakeVisible(freqSlider);
    freqSlider.setVisible(false);
    freqSlider.addListener(this);

    // Gain knob - modern UI: larger text box (80x16), visible dB value
    gainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 104, 22);
    gainSlider.setRange(-30.0, 30.0, 0.01);
    gainSlider.setTextValueSuffix(" dB");
    gainSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF00b4d8));
    gainSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFF00b4d8));
    gainSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFffffff));
    gainSlider.setNumDecimalPlacesToDisplay(2);

    gainSlider.setVelocityBasedMode(true);
    gainSlider.setVelocityModeParameters(0.35, 2, 0.0);
    gainSlider.setDoubleClickReturnValue(true, 0.0);
    gainSlider.setTooltip("Gain of this EQ band in dB (double-click to reset to 0 dB)");
    addAndMakeVisible(gainSlider);
    gainSlider.setVisible(false);
    gainSlider.addListener(this);

    // Q knob - modern UI: larger text box (80x16), visible Q value
    qSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    qSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 104, 22);
    qSlider.setRange(0.1, 10.0, 0.01);
    qSlider.setSkewFactor(0.4);
    qSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF533483));
    qSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFF533483));
    qSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFffffff));
    qSlider.setNumDecimalPlacesToDisplay(3);
    qSlider.setDoubleClickReturnValue(true, 0.707);
    qSlider.setTooltip("Q (bandwidth) of this EQ band (double-click to reset to 0.707)");
    addAndMakeVisible(qSlider);
    qSlider.setVisible(false);
    qSlider.addListener(this);

    // Channel mode - modern UI: larger font (11pt), hover highlight on selected item
    for (const auto& name : channelModeNames)
        channelModeBox.addItem(name, channelModeBox.getNumItems() + 1);
    channelModeBox.setSelectedId(1);
    channelModeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF16213e));
    channelModeBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFffffff));
    channelModeBox.setTooltip("Which channel this band processes (Stereo / L / R / Mid / Side)");
    addAndMakeVisible(channelModeBox);
    channelModeBox.setVisible(false);
    channelModeBox.addListener(this);

    // Gain-Q interaction - modern UI: larger text (12pt), hover/focus state
    gainQBtn.setButtonText("GQ");
    gainQBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    gainQBtn.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xFFe9c46a));
    gainQBtn.setClickingTogglesState(true);
    gainQBtn.setTooltip("Toggle Gain-Q interaction (FabFilter style link)");
    addAndMakeVisible(gainQBtn);
    gainQBtn.setVisible(false);
    gainQBtn.addListener(this);

    // Prev / Next band - modern UI: larger text (12pt), hover/focus state
    prevBandBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    prevBandBtn.setClickingTogglesState(false);
    prevBandBtn.setTooltip("Select previous band");
    addAndMakeVisible(prevBandBtn);
    prevBandBtn.setVisible(false);
    prevBandBtn.addListener(this);

    nextBandBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    nextBandBtn.setClickingTogglesState(false);
    nextBandBtn.setTooltip("Select next band");
    addAndMakeVisible(nextBandBtn);
    nextBandBtn.setVisible(false);
    nextBandBtn.addListener(this);

    // Band number - modern UI: larger font (13pt), brighter text
    bandNumberLabel.setFont(makeBoldFont(13.0f));
    bandNumberLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFffffff));
    addAndMakeVisible(bandNumberLabel);
    bandNumberLabel.setVisible(false);

    // Delete - modern UI: larger text (12pt), hover/focus state
    deleteBandBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFe94560));

    // Invert Gain - modern UI: larger text (12pt), hover/focus state
    invertGainBtn.setButtonText("Inv");
    invertGainBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF16213e));
    invertGainBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFe94560));

    // Dynamic EQ sliders - modern UI: larger text box (80x16), visible values
    dynRangeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dynRangeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 104, 22);
    dynRangeSlider.setRange(-30.0, 30.0, 0.1);
    dynRangeSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF2a9d8f));
    dynRangeSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFF2a9d8f));
    dynRangeSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFffffff));
    dynRangeSlider.setNumDecimalPlacesToDisplay(1);

    dynRangeSlider.setVelocityBasedMode(true);
    dynRangeSlider.setVelocityModeParameters(0.35, 2, 0.0);
    dynRangeSlider.setDoubleClickReturnValue(true, 0.0);
    addAndMakeVisible(dynRangeSlider);
    dynRangeSlider.setVisible(false);
    dynRangeSlider.addListener(this);

    dynThreshSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dynThreshSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 104, 22);
    dynThreshSlider.setRange(-60.0, 0.0, 0.1);
    dynThreshSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF2a9d8f));
    dynThreshSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFF2a9d8f));
    dynThreshSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFffffff));
    dynThreshSlider.setNumDecimalPlacesToDisplay(1);

    dynThreshSlider.setVelocityBasedMode(true);
    dynThreshSlider.setVelocityModeParameters(0.35, 2, 0.0);
    dynThreshSlider.setDoubleClickReturnValue(true, -20.0);
    dynThreshSlider.setTooltip("Threshold level that triggers the dynamic EQ");
    addAndMakeVisible(dynThreshSlider);
    dynThreshSlider.setVisible(false);
    dynThreshSlider.addListener(this);

    // Dynamic EQ buttons - modern UI: larger text (12pt), hover/focus state
    dynAutoBtn.setButtonText("Auto");
    dynAutoBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFF2a9d8f));
    dynAutoBtn.setClickingTogglesState(true);
    dynAutoBtn.setToggleState(true, juce::dontSendNotification);
    dynAutoBtn.setTooltip("Auto-threshold: disable to set the threshold manually");
    addAndMakeVisible(dynAutoBtn);
    dynAutoBtn.setVisible(false);
    dynAutoBtn.addListener(this);

    scTriggerBtn.setButtonText("SC");
    scTriggerBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFf72585));
    scTriggerBtn.setClickingTogglesState(true);
    scTriggerBtn.setTooltip("Use sidechain input to trigger the dynamic EQ");
    addAndMakeVisible(scTriggerBtn);
    scTriggerBtn.setVisible(false);
    scTriggerBtn.addListener(this);

    phaseInvertBtn.setButtonText("Ø");
    phaseInvertBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFe94560));
    phaseInvertBtn.setClickingTogglesState(true);
    phaseInvertBtn.setTooltip("Invert the phase of this band");
    addAndMakeVisible(phaseInvertBtn);
    phaseInvertBtn.setVisible(false);
    phaseInvertBtn.addListener(this);

    // Bottom bar buttons - modern UI: larger text (12pt), hover/focus state
    phaseBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFe94560));
    phaseBtn.setClickingTogglesState(true);

    autoGainAdvBtn.setButtonText("Adv");
    autoGainAdvBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));

    // EQ Match buttons - modern UI: larger text (12pt), hover/focus state
    eqMatchCaptureBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF16213e));
    eqMatchCaptureBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFe94560));

    eqMatchApplyBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF16213e));
    eqMatchApplyBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF00b4d8));

    // MIDI Learn - modern UI: larger text (12pt), hover/focus state
    midiLearnBtn.setButtonText("MIDI");
    midiLearnBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF16213e));
    midiLearnBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFe94560));

    // Side panel toggles - modern UI: larger text (12pt), hover/focus state
    instPanelBtn.setClickingTogglesState(true);
    instPanelBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));

    undoPanelBtn.setClickingTogglesState(true);
    undoPanelBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));

    // Top bar buttons - modern UI: larger text (12pt), hover/focus state
    savePresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF16213e));
    savePresetBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFe94560));

    undoBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFa0a0c0));

    redoBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFa0a0c0));

    fullScreenBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));

    pianoScaleBtn.setButtonText("Piano");
    pianoScaleBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));

    // Freeze and AB buttons - modern UI: larger text (12pt), hover/focus state
    freezeBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));

    abBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFe94560));

    // Band bypass button - modern UI: larger text (12pt), hover/focus state
    bandBypassBtn.setButtonText("B");
    bandBypassBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));

    // Gain-Q interaction button - modern UI: larger text (12pt), hover/focus state
    gainQBtn.setButtonText("GQ");
    gainQBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));

    // Bypass button - modern UI: larger text (12pt), hover/focus state
    bandBypassBtn.setButtonText("B");
    bandBypassBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));

    // Output pan slider - modern UI: larger text box (80x16), visible value
    outputPanSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    outputPanSlider.setRange(-1.0, 1.0, 0.01);
    outputPanSlider.setValue(0.0, juce::dontSendNotification);
    outputPanSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFFe0e0ff));
    outputPanSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xFF2a2a4a));
    outputPanSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFffffff));
    outputPanSlider.setDoubleClickReturnValue(true, 0.0);

    // Gain scale slider - modern UI: larger text box (80x16), visible value
    gainScaleSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    gainScaleSlider.setRange(0.0, 2.0, 0.01);
    gainScaleSlider.setValue(1.0, juce::dontSendNotification);
    gainScaleSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFFe94560));
    gainScaleSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xFF2a2a4a));
    gainScaleSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFffffff));
    gainScaleSlider.setDoubleClickReturnValue(true, 1.0);

    // Auto gain weight slider - modern UI: larger text box (80x16), visible value
    autoGainWeightSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    autoGainWeightSlider.setRange(0.0, 1.0, 0.01);
    autoGainWeightSlider.setValue(0.5, juce::dontSendNotification);
    autoGainWeightSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFFe94560));
    autoGainWeightSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xFF2a2a4a));
    autoGainWeightSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFffffff));
    autoGainWeightSlider.setDoubleClickReturnValue(true, 0.5);

    // Labels - modern UI: larger font (11pt), brighter text
    panLabel.setFont(makeFont(11.0f));
    panLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFffffff));

    gainScaleLabel.setFont(makeFont(11.0f));
    gainScaleLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFffffff));

    autoGainWeightLabel.setFont(makeFont(11.0f));
    autoGainWeightLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFffffff));

    spectrumGrabLabel.setFont(makeFont(11.0f));
    spectrumGrabLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFe94560));

    // Undo history panel buttons - modern UI: larger text (12pt), hover/focus state
    addAndMakeVisible(deleteBandBtn);
    deleteBandBtn.setVisible(false);
    deleteBandBtn.setTooltip("Delete this band");
    deleteBandBtn.addListener(this);

    // Invert Gain
    invertGainBtn.setButtonText("Inv");
    invertGainBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF16213e));
    invertGainBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFe94560));
    invertGainBtn.setTooltip("Invert the gain of this band (boost becomes cut and vice versa)");
    addAndMakeVisible(invertGainBtn);
    invertGainBtn.setVisible(false);
    invertGainBtn.addListener(this);

    // Dynamic EQ row
    dynRangeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dynRangeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 104, 22);
    dynRangeSlider.setRange(-30.0, 30.0, 0.1);
    dynRangeSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF2a9d8f));
    dynRangeSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFF2a9d8f));
    dynRangeSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFffffff));
    dynRangeSlider.setNumDecimalPlacesToDisplay(1);
    dynRangeSlider.setDoubleClickReturnValue(true, 0.0);
    dynRangeSlider.setTooltip("Dynamic EQ range: how much the gain varies around its static value");
    addAndMakeVisible(dynRangeSlider);
    dynRangeSlider.setVisible(false);
    dynRangeSlider.addListener(this);

    dynThreshSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dynThreshSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 104, 22);
    dynThreshSlider.setRange(-60.0, 0.0, 0.1);
    dynThreshSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF2a9d8f));
    dynThreshSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFF2a9d8f));
    dynThreshSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFffffff));
    dynThreshSlider.setNumDecimalPlacesToDisplay(1);
    dynThreshSlider.setDoubleClickReturnValue(true, -20.0);
    dynThreshSlider.setTooltip("Threshold level that triggers the dynamic EQ");
    addAndMakeVisible(dynThreshSlider);
    dynThreshSlider.setVisible(false);
    dynThreshSlider.addListener(this);

    dynAtkSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dynAtkSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 88, 20);
    dynAtkSlider.setRange(1.0, 500.0, 0.1);
    dynAtkSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFFe76f51));
    dynAtkSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFFe76f51));
    dynAtkSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFffffff));
    dynAtkSlider.setNumDecimalPlacesToDisplay(0);

    dynAtkSlider.setVelocityBasedMode(true);
    dynAtkSlider.setVelocityModeParameters(0.35, 2, 0.0);
    dynAtkSlider.setDoubleClickReturnValue(true, 10.0);
    dynAtkSlider.setTooltip("Manual attack time (ms): used when Auto Attack is off");
    addAndMakeVisible(dynAtkSlider);
    dynAtkSlider.setVisible(false);
    dynAtkSlider.addListener(this);

    dynRelSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dynRelSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 88, 20);
    dynRelSlider.setRange(1.0, 2000.0, 0.1);
    dynRelSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFFe76f51));
    dynRelSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFFe76f51));
    dynRelSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFffffff));
    dynRelSlider.setNumDecimalPlacesToDisplay(0);

    dynRelSlider.setVelocityBasedMode(true);
    dynRelSlider.setVelocityModeParameters(0.35, 2, 0.0);
    dynRelSlider.setDoubleClickReturnValue(true, 100.0);
    dynRelSlider.setTooltip("Manual release time (ms): used when Auto Release is off");
    addAndMakeVisible(dynRelSlider);
    dynRelSlider.setVisible(false);
    dynRelSlider.addListener(this);

    dynAutoBtn.setButtonText("Auto");
    dynAutoBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFF2a9d8f));
    dynAutoBtn.setClickingTogglesState(true);
    dynAutoBtn.setToggleState(true, juce::dontSendNotification);
    dynAutoBtn.setTooltip("Auto-threshold: disable to set the threshold manually");
    addAndMakeVisible(dynAutoBtn);
    dynAutoBtn.setVisible(false);
    dynAutoBtn.addListener(this);

    scTriggerBtn.setButtonText("SC");
    scTriggerBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFf72585));
    scTriggerBtn.setClickingTogglesState(true);
    scTriggerBtn.setTooltip("Use sidechain input to trigger the dynamic EQ");
    addAndMakeVisible(scTriggerBtn);
    scTriggerBtn.setVisible(false);
    scTriggerBtn.addListener(this);

    phaseInvertBtn.setButtonText("Ø");
    phaseInvertBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFe94560));
    phaseInvertBtn.setClickingTogglesState(true);
    phaseInvertBtn.setTooltip("Invert the phase of this band");
    addAndMakeVisible(phaseInvertBtn);
    phaseInvertBtn.setVisible(false);
    phaseInvertBtn.addListener(this);

    // === Bottom bar (global) ===
    phaseBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFe94560));
    phaseBtn.setClickingTogglesState(true);
    phaseBtn.setTooltip("Global phase invert of the output");
    addAndMakeVisible(phaseBtn);
    phaseBtn.addListener(this);

    globalBypassBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFe94560));
    globalBypassBtn.setClickingTogglesState(true);
    globalBypassBtn.setTooltip("Global bypass (B): disables the entire EQ without affecting the signal path");
    addAndMakeVisible(globalBypassBtn);
    globalBypassBtn.addListener(this);

    autoGainBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    autoGainBtn.setToggleState(true, juce::dontSendNotification);
    autoGainBtn.setTooltip("Compensate for gain changes introduced by the EQ curve");
    addAndMakeVisible(autoGainBtn);
    autoGainBtn.addListener(this);

    outputPanSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    outputPanSlider.setRange(-1.0, 1.0, 0.01);
    outputPanSlider.setValue(0.0, juce::dontSendNotification);
    outputPanSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFFe0e0ff));
    outputPanSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xFF2a2a4a));
    outputPanSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFe0e0ff));
    outputPanSlider.setDoubleClickReturnValue(true, 0.0);
    outputPanSlider.setTooltip("Global stereo pan of the output");
    addAndMakeVisible(outputPanSlider);

    panLabel.setJustificationType(juce::Justification::centred);
    panLabel.setFont(makeFont(10.0f));
    panLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFa0a0c0));
    panLabel.setTooltip("Global stereo pan of the output");
    addAndMakeVisible(panLabel);

    gainScaleSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    gainScaleSlider.setRange(0.0, 2.0, 0.01);
    gainScaleSlider.setValue(1.0, juce::dontSendNotification);
    gainScaleSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFFe94560));
    gainScaleSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xFF2a2a4a));
    gainScaleSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFe0e0ff));
    gainScaleSlider.setDoubleClickReturnValue(true, 1.0);
    gainScaleSlider.setTooltip("Global gain scale (multiplier applied to all band gains)");
    addAndMakeVisible(gainScaleSlider);

    gainScaleLabel.setJustificationType(juce::Justification::centred);
    gainScaleLabel.setFont(makeFont(10.0f));
    gainScaleLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFa0a0c0));
    gainScaleLabel.setTooltip("Global gain scale (multiplier applied to all band gains)");
    addAndMakeVisible(gainScaleLabel);

    spectrumGrabLabel.setJustificationType(juce::Justification::centred);
    spectrumGrabLabel.setFont(makeFont(11.0f));
    spectrumGrabLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFe94560));
    addAndMakeVisible(spectrumGrabLabel);
    spectrumGrabLabel.setVisible(false);

    // Advanced Auto Gain
    autoGainAdvBtn.setButtonText("Adv");
    autoGainAdvBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    autoGainAdvBtn.setTooltip("Advanced auto-gain options");
    addAndMakeVisible(autoGainAdvBtn);
    autoGainAdvBtn.addListener(this);

    autoGainWeightSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    autoGainWeightSlider.setRange(0.0, 1.0, 0.01);
    autoGainWeightSlider.setValue(0.5, juce::dontSendNotification);
    autoGainWeightSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFFe94560));
    autoGainWeightSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xFF2a2a4a));
    autoGainWeightSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xFFe0e0ff));
    autoGainWeightSlider.setDoubleClickReturnValue(true, 0.5);
    autoGainWeightSlider.setTooltip("Weight of auto-gain compensation against the static gain");
    addAndMakeVisible(autoGainWeightSlider);

    autoGainWeightLabel.setJustificationType(juce::Justification::centred);
    autoGainWeightLabel.setFont(makeFont(10.0f));
    autoGainWeightLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFa0a0c0));
    autoGainWeightLabel.setTooltip("Weight of auto-gain compensation against the static gain");
    addAndMakeVisible(autoGainWeightLabel);

    // EQ Match
    eqMatchBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    eqMatchBtn.setClickingTogglesState(true);
    addAndMakeVisible(eqMatchBtn);
    eqMatchBtn.setVisible(false);
    eqMatchBtn.addListener(this);

    eqMatchCaptureBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF16213e));
    eqMatchCaptureBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFe94560));
    eqMatchCaptureBtn.setTooltip("Capture the current spectrum as the EQ match target");
    addAndMakeVisible(eqMatchCaptureBtn);
    eqMatchCaptureBtn.setVisible(false);
    eqMatchCaptureBtn.addListener(this);

    eqMatchApplyBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF16213e));
    eqMatchApplyBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF00b4d8));
    eqMatchApplyBtn.setTooltip("Apply the matched EQ curve to the bands");
    addAndMakeVisible(eqMatchApplyBtn);
    eqMatchApplyBtn.setVisible(false);
    eqMatchApplyBtn.addListener(this);

    // MIDI Learn
    midiLearnBtn.setButtonText("MIDI");
    midiLearnBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF16213e));
    midiLearnBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFe94560));
    midiLearnBtn.setTooltip("Learn a MIDI CC to control this instance");
    addAndMakeVisible(midiLearnBtn);
    midiLearnBtn.setVisible(false);
    midiLearnBtn.addListener(this);

    // Side panel toggles
    instPanelBtn.setClickingTogglesState(true);
    instPanelBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    instPanelBtn.setTooltip("Show / hide the instance list panel");
    addAndMakeVisible(instPanelBtn);
    instPanelBtn.addListener(this);

    undoPanelBtn.setClickingTogglesState(true);
    undoPanelBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    undoPanelBtn.setTooltip("Show / hide the undo history panel");
    addAndMakeVisible(undoPanelBtn);
    undoPanelBtn.addListener(this);

    instancePanel.setVisible(false);
    addChildComponent(instancePanel);
    historyPanel.setVisible(false);
    addChildComponent(historyPanel);

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

void EeqEditor::timerCallback()
{
    if (instPanelVisible)
        instancePanel.syncWithProcessor(processor);

    // Periodic spectrum diagnostics (first 30 ticks) to catch the 15 kHz spike source
    if (logTickCount < 30)
    {
        ++logTickCount;
        const auto& sdata = processor.getSpectrumAnalyzer().getSpectrumData();
        int nb = processor.getSpectrumAnalyzer().getNumBins();
        float sr = processor.getSpectrumAnalyzer().getSampleRate();
        if (nb > 0 && sr > 0.0f)
        {
            float nyquist = sr * 0.5f;
            float maxVal = 0.0f, maxFreq = 0.0f;
            for (int b = 0; b < nb; ++b)
            {
                if (sdata[(size_t)b] > maxVal)
                {
                    maxVal = sdata[(size_t)b];
                    maxFreq = (float)b / (float)nb * nyquist;
                }
            }
            int bin15 = (int)(15000.0f / nyquist * (float)nb);
            bin15 = juce::jlimit(0, nb - 1, bin15);
            if (logTickCount == 1 || logTickCount % 5 == 0)
                EeqProcessor::writeLog("tick#" + juce::String(logTickCount)
                    + " spectrumMax=" + juce::String(maxVal, 3)
                    + " @ " + juce::String(maxFreq, 0) + "Hz"
                    + " val15k=" + juce::String(sdata[(size_t)bin15], 3));
        }
    }

    repaint();
}

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
    if (selectedBand >= 0)
        b.removeFromBottom(getBandControlStripHeight());
    return b;
}

juce::Rectangle<float> EeqEditor::getBandControlStripBounds() const
{
    auto b = getLocalBounds().toFloat();
    b = b.reduced(36, 0);
    b.removeFromTop(36);
    b.removeFromBottom(24);
    b.removeFromBottom(40);
    return b.removeFromBottom(getBandControlStripHeight());
}

float EeqEditor::getBandControlStripHeight() const
{
    if (selectedBand >= 0 && selectedBand < NUM_BANDS)
    {
        auto& apvts = processor.getAPVTS();
        auto id = juce::String(selectedBand + 1);
        bool dyn = apvts.getRawParameterValue("b" + id + "_dyn")->load() > 0.5f;
        if (dyn) return 140.0f;
    }
    return 98.0f;
}

juce::Rectangle<float> EeqEditor::getMeterBounds() const
{
    auto b = getLocalBounds().toFloat();
    return b.removeFromRight(16).reduced(0, 36);
}

juce::Rectangle<float> EeqEditor::getBandControlsBounds() const
{
    if (selectedBand < 0) return {};
    return getBandControlStripBounds();
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
    // Use a reference range of 6.0f for consistency with yToGain
    // The displayRange still controls visual display scaling
    constexpr float REFERENCE_RANGE = 6.0f;
    return (d.getHeight() * 0.5f) - (gain / REFERENCE_RANGE) * (d.getHeight() * 0.5f);
}

float EeqEditor::yToGain(float y, juce::Rectangle<float> d) const
{
    float range = processor.getDisplayRange();
    // Use a reference range of 6.0f for mouse sensitivity to ensure consistent
    // behavior regardless of the display range setting. The displayRange variable
    // still controls the visual zoom of the EQ curve.
    constexpr float REFERENCE_RANGE = 6.0f;
    return ((d.getHeight() * 0.5f) - y) / (d.getHeight() * 0.5f) * REFERENCE_RANGE;
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
    dynAtkSlider.setValue((float)apvts.getRawParameterValue("b" + id + "_dynAtkMs")->load(), juce::dontSendNotification);
    dynRelSlider.setValue((float)apvts.getRawParameterValue("b" + id + "_dynRelMs")->load(), juce::dontSendNotification);
    bool autoAtk = apvts.getRawParameterValue("b" + id + "_dynAutoAtk")->load() > 0.5f;
    bool autoRel = apvts.getRawParameterValue("b" + id + "_dynAutoRel")->load() > 0.5f;
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

    bool showAtk = dyn && dynA && !autoAtk;
    dynAtkSlider.setVisible(showAtk);
    bool showRel = dyn && dynA && !autoRel;
    dynRelSlider.setVisible(showRel);
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
    bool isCutType = (type == 3 || type == 4);
    apvts.getParameter("b" + id + "_slope")->setValueNotifyingHost(
        apvts.getParameter("b" + id + "_slope")->convertTo0to1(
            isCutType ? slopeBox.getSelectedId() - 1 : 0));
    apvts.getParameter("b" + id + "_sc")->setValueNotifyingHost(
        scTriggerBtn.getToggleState() ? 1.0f : 0.0f);

    apvts.getParameter("b" + id + "_dynRange")->setValueNotifyingHost(
        apvts.getParameter("b" + id + "_dynRange")->convertTo0to1((float)dynRangeSlider.getValue()));
    apvts.getParameter("b" + id + "_dynThresh")->setValueNotifyingHost(
        apvts.getParameter("b" + id + "_dynThresh")->convertTo0to1((float)dynThreshSlider.getValue()));
    apvts.getParameter("b" + id + "_dynAtkMs")->setValueNotifyingHost(
        apvts.getParameter("b" + id + "_dynAtkMs")->convertTo0to1((float)dynAtkSlider.getValue()));
    apvts.getParameter("b" + id + "_dynRelMs")->setValueNotifyingHost(
        apvts.getParameter("b" + id + "_dynRelMs")->convertTo0to1((float)dynRelSlider.getValue()));
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
    if (key == juce::KeyPress('b', true, false))
    {
        globalBypassBtn.setToggleState(!globalBypassBtn.getToggleState(), juce::sendNotification);
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
        // Write the APVTS parameter using the selected index (canonical way for choice params)
        // This ensures the value sticks across sessions
        auto* param = processor.getAPVTS().getParameter("displayRange");
        if (param != nullptr)
            param->setValueNotifyingHost(displayRangeBox.getSelectedId() - 1); // 0-based index
        
        // Also update the processor member directly for immediate display response
        processor.setDisplayRange((float)displayRangeBox.getSelectedId());
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
        {
            processor.undo();
            historyPanel.addHistoryItem("Undo");
            historyPanel.repaint();
        }
    else if (btn == &redoBtn)
        {
            processor.redo();
            historyPanel.addHistoryItem("Redo");
            historyPanel.repaint();
        }

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
    else if (btn == &globalBypassBtn)
        processor.setGlobalBypassEnabled(globalBypassBtn.getToggleState());
    else if (btn == &autoGainBtn)
        processor.setAutoGainEnabled(autoGainBtn.getToggleState());
    else if (btn == &autoGainAdvBtn)
        processor.setAutoGainAdvanced(autoGainAdvBtn.getToggleState());

    else if (btn == &instPanelBtn)
    {
        instPanelVisible = instPanelBtn.getToggleState();
        instancePanel.setVisible(instPanelVisible);
        resized();
    }
    else if (btn == &undoPanelBtn)
    {
        undoPanelVisible = undoPanelBtn.getToggleState();
        historyPanel.setVisible(undoPanelVisible);
        resized();
    }

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
    juce::ColourGradient bgGrad(juce::Colour(0xFF12121f), 0.0f, 0.0f,
                                juce::Colour(0xFF0a0a1a), 0.0f, (float)getHeight(), false);
    g.setGradientFill(bgGrad);
    g.fillAll();

    auto topBar = getTopBarBounds();
    juce::ColourGradient topGrad(juce::Colour(0xFF1a1a35), 0.0f, 0.0f,
                                 juce::Colour(0xFF0f0f2a), 0.0f, (float)topBar.getHeight(), false);
    g.setGradientFill(topGrad);
    g.fillRect(topBar);
    g.setColour(juce::Colour(0xFF2a2a4a));
    g.drawHorizontalLine(topBar.getBottom() - 1, (float)topBar.getX(), (float)topBar.getRight());

    g.setColour(juce::Colour(0xFFe94560));
    g.setFont(makeBoldFont(20.0f));
    g.drawText("EEQ", topBar.reduced(8, 0).removeFromLeft(48), juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xFF6a6a8e));
    g.setFont(makeFont(9.0f));
    g.drawText("PARAMETRIC EQ", topBar.getX() + 6, topBar.getY() + 20, 50, 12, juce::Justification::centredLeft);

    // Current preset name (Pro-Q3 style, centered in the top bar)
    g.setColour(juce::Colour(0xFFe9e9ef));
    g.setFont(makeBoldFont(14.0f));
    g.drawText(presetSelector.getText(),
               topBar.getX() + 200, topBar.getY() + 2, topBar.getWidth() - 420, topBar.getHeight() - 4,
               juce::Justification::centred);

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
    juce::ColourGradient panelGrad(juce::Colour(0xFF16162a), 0.0f, d.getY(),
                                   juce::Colour(0xFF101020), 0.0f, d.getBottom(), false);
    g.setGradientFill(panelGrad);
    g.fillRoundedRectangle(d, 6.0f);

    g.setColour(juce::Colour(0xFF2a2a4a));
    g.drawRoundedRectangle(d, 6.0f, 1.0f);

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

    // === Peak Labels on Spectrum Grab ===
    if (spectrumGrabbing)
    {
        const auto& data = processor.getSpectrumAnalyzer().getSpectrumData();
        std::vector<std::pair<float, float>> peaks; // freq, mag
        const float grabRange = std::max(2.0f * spectrumGrabFreq, 200.0f);
        const float loFreq = spectrumGrabFreq / grabRange;
        const float hiFreq = spectrumGrabFreq * grabRange;

        // Find local maxima within logarithmic window around the grabbed frequency
        for (int px = 1; px < (int)d.getWidth() - 1; ++px)
        {
            float freq = xToFreq((float)px, d);
            if (freq < loFreq || freq > hiFreq) continue;

            int bin = (int)(freq / nyquist * (float)numBins);
            bin = juce::jlimit(0, numBins - 1, bin);
            int binP = juce::jlimit(0, numBins - 1, bin + 1);
            int binM = juce::jlimit(0, numBins - 1, bin - 1);

            float mag = data[bin];
            if (mag < data[binM] || mag < data[binP]) continue; // not a local max
            if (mag < 0.05f) continue;

            peaks.push_back({freq, mag});
        }

        // Keep strongest peaks only
        std::sort(peaks.begin(), peaks.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        if (peaks.size() > 5) peaks.resize(5);

        for (auto& p : peaks)
        {
            float x = d.getX() + std::round(xToFreq(p.first, d));
            float y = d.getY() + d.getHeight() * (1.0f - p.second);
            float magDB = 20.0f * std::log10(std::max(p.second, 1e-10f));

            int note = freqToMidiKey(p.first);
            float noteFreq = midiKeyToFreq(note);
            int cents = (int)std::round(1200.0f * std::log2(p.first / noteFreq));
            juce::String noteNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
            juce::String noteName = noteNames[note % 12] + juce::String(note / 12 - 1);
            juce::String centsStr = (cents >= 0 ? "+" : "") + juce::String(cents);

            juce::String labelText = juce::String(p.first, 1) + " Hz  " + noteName + " " + centsStr + "¢  " + juce::String(magDB, 1) + " dB";

            juce::Font labelFont = makeFont(11.0f);
            juce::GlyphArrangement glyphs;
            glyphs.addCurtailedLineOfText(labelFont, labelText, 0.0f, 0.0f, 1000.0f, false);
            auto textWidth = glyphs.getBoundingBox(0, glyphs.getNumGlyphs(), true).getWidth() + 8.0f;
            auto textHeight = 15.0f;
            auto labelX = juce::jlimit(d.getX(), d.getRight() - textWidth, x + 8.0f);
            auto labelY = juce::jlimit(d.getY(), d.getBottom() - textHeight, y - textHeight - 4.0f);

            g.setColour(juce::Colour(0xCC11111a));
            g.fillRoundedRectangle(labelX, labelY, textWidth, textHeight, 4.0f);
            g.setColour(juce::Colour(0xFFe94560));
            g.setFont(labelFont);
            g.drawText(labelText, juce::Rectangle<float>(labelX, labelY, textWidth, textHeight),
                       juce::Justification::centred, false);

            // Peak marker line
            g.setColour(juce::Colour(0xFFe94560).withAlpha(0.6f));
            g.drawLine(x, y - 6.0f, x, y + 6.0f, 1.5f);
        }
    }

    // === Collision Visual Overlay (red shading from other instances) ===
    {
        // Pro-Q3 style: only show other instances when THIS instance also has
        // signal energy (a real frequency collision). When self spectrum is
        // silent, keep the display flat.
        const auto& selfData = processor.getSpectrumAnalyzer().getSpectrumData();
        bool selfHasSignal = false;
        for (int b = 0; b < numBins && b < 4096; ++b)
        {
            if (selfData[(size_t)b] > 0.15f)
            {
                selfHasSignal = true;
                break;
            }
        }

        if (!selfHasSignal)
            return;

        int instIndex = 0;
        const auto& instances = EeqProcessor::getInstanceList();
        for (auto* info : instances)
        {
            if (info == nullptr)
                continue;

            // Skip self and hidden instances
            if (info->instanceId == reinterpret_cast<uintptr_t>(&processor) || !info->isVisible)
            {
                ++instIndex;
                continue;
            }

            // Collision overlay can be individually toggled from the Instance List panel
            if (instPanelVisible && instIndex < instancePanel.getInstanceCount()
                && instancePanel.isVisible(instIndex) == false)
            {
                ++instIndex;
                continue;
            }

            if (!info->hasSpectrum)
            {
                ++instIndex;
                continue;
            }

            juce::Path overlayPath;
            bool overlayStarted = false;
            for (int px = 0; px < (int)d.getWidth(); ++px)
            {
                float freq = xToFreq((float)px, d);
                if (freq < MIN_FREQ || freq > MAX_FREQ) continue;
                int bin = (int)(freq / nyquist * (float)numBins);
                bin = juce::jlimit(0, numBins - 1, bin);
                float mag = info->spectrum[(size_t)bin];
                if (mag < 0.005f) mag = 0.0f;

                float x = d.getX() + (float)px;
                float y = d.getY() + d.getHeight() * (1.0f - mag);
                if (!overlayStarted) { overlayPath.startNewSubPath(x, y); overlayStarted = true; }
                else overlayPath.lineTo(x, y);
            }

            juce::Path filledOverlay(overlayPath);
            filledOverlay.lineTo(d.getRight(), d.getBottom());
            filledOverlay.lineTo(d.getX(), d.getBottom());
            filledOverlay.closeSubPath();

            // Red shading - Pro-Q3 external spectrum style
            g.setColour(juce::Colour(0xFFe94560).withAlpha(0.25f));
            g.fillPath(filledOverlay);

            juce::Colour overlayCol = bandColours[0];
            g.setColour(overlayCol.withAlpha(0.4f));
            g.strokePath(overlayPath, juce::PathStrokeType(1.2f));

            ++instIndex;
        }
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

            // Pro-Q3 style big value readout above the selected node
            juce::String freqLabel;
            if (freq >= 1000.0f)
                freqLabel = juce::String(freq / 1000.0f, 1) + "k";
            else
                freqLabel = juce::String((int)freq);
            juce::String gainLabel = (gain >= 0 ? "+" : "") + juce::String(gain, 1) + " dB";
            juce::String qLabel = "Q " + juce::String(q, 2);
            juce::String readout = freqLabel + "  " + gainLabel + "  " + qLabel;

            g.setFont(makeBoldFont(14.0f));
            juce::GlyphArrangement gauge;
            gauge.addCurtailedLineOfText(g.getCurrentFont(), readout, 0.0f, 0.0f, 1.0e9f, false);
            float tw = gauge.getBoundingBox(0, gauge.getNumGlyphs(), true).getWidth();
            float readoutX = x - tw * 0.5f;
            float readoutY = y - radius - 28.0f;

            // Background pill
            g.setColour(juce::Colour(0xDD101020));
            g.fillRoundedRectangle(readoutX - 8, readoutY - 2, tw + 16, 18, 6.0f);
            g.setColour(col.withAlpha(0.5f));
            g.drawRoundedRectangle(juce::Rectangle<float>(readoutX - 8, readoutY - 2, tw + 16, 18), 6.0f, 1.0f);

            g.setColour(juce::Colour(0xFFf0f0ff));
            g.drawText(readout, (int)readoutX, (int)readoutY, (int)tw, 16, juce::Justification::centred);
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
    int activeCount = 0;
    for (int i = 0; i < NUM_BANDS; ++i)
        if (bandVisuals[i].active) activeCount++;
    g.drawText(juce::String(activeCount) + " / " + juce::String(NUM_BANDS) + " bands",
               d.getX() + 4, d.getY() + 2, 80, 14, juce::Justification::centredLeft);

    // Live M/S gain offset chip (Pro-Q parity): a band routed to Mid or Side
    // with a non-zero split-gain offset shows a compact second readout in the
    // strip info row. Paint-only; reads live APVTS, writes nothing.
    if (selectedBand >= 0 && selectedBand < NUM_BANDS)
    {
        auto bandId = juce::String(selectedBand + 1);
        auto chVal = (int)processor.getAPVTS().getRawParameterValue("b" + bandId + "_ch")->load();
        float chipDb = 0.0f;
        juce::String which;
        if (chVal == 3)      { which = "M"; chipDb = processor.getAPVTS().getRawParameterValue("b" + bandId + "_midGain")->load(); }
        else if (chVal == 4) { which = "S"; chipDb = processor.getAPVTS().getRawParameterValue("b" + bandId + "_sideGain")->load(); }
        if (which.isNotEmpty() && std::fabs(chipDb) > 0.05f)
        {
            juce::Colour chipCol = juce::Colour(0xFF00b4d8).withAlpha(0.85f);
            juce::Rectangle<int> chip((int)(d.getX() + 88), (int)(d.getY() + 2), 58, 14);
            g.setColour(chipCol);
            g.drawRoundedRectangle(chip.toFloat(), 4.0f, 1.0f);
            g.setColour(chipCol);
            auto txt = which + " " + (chipDb > 0.0f ? "+" : "-")
                       + juce::String(std::fabs(chipDb), 1) + " dB";
            g.drawText(txt, chip.getX(), chip.getY(), 58, 14, juce::Justification::centred);
        }
    }

    float range = processor.getDisplayRange();
    g.drawText(juce::String((int)range) + " dB range",
               d.getRight() - 70, d.getY() + 2, 66, 14, juce::Justification::centredRight);

    if (hZoom > 1.05f)
    {
    g.setColour(juce::Colour(0xFFe94560).withAlpha(0.6f));
    }
}

// ===================== Floating Band Controls Panel =====================

void EeqEditor::drawBandControls(juce::Graphics& g, juce::Rectangle<float> display)
{
    if (selectedBand < 0) return;
    auto panel = getBandControlsBounds();
    if (panel.isEmpty()) return;

    // Draw fixed strip background (Pro-Q3 style dark panel)
    juce::ColourGradient panelGrad(juce::Colour(0xFF14141f), 0.0f, panel.getY(),
                                   juce::Colour(0xFF0f0f1a), 0.0f, panel.getBottom(), false);
    g.setGradientFill(panelGrad);
    g.fillRoundedRectangle(panel, 8.0f);
    g.setColour(juce::Colour(0xFF2a2a4a).withAlpha(0.8f));
    g.drawRoundedRectangle(panel, 8.0f, 1.0f);

    auto col = bandColours[selectedBand % 24];

    // Colour accent bar at top of strip
    g.setColour(col);
    g.fillRoundedRectangle(panel.getX() + 10, panel.getY(), 64, 3, 2.0f);

    // Caption labels above controls (Pro-Q3 style small caps)
    g.setColour(juce::Colour(0xFF8a8aae));
    juce::Font capFont = makeFont(10.0f);
    g.setFont(capFont);

    float labelTop = panel.getY() + 5.0f;
    auto drawCap = [&](float x, const char* text, float w = 66.0f)
    {
        g.drawText(text, (int)x, (int)labelTop, (int)w, 12, juce::Justification::centred);
    };

    float lx = panel.getX() + 16.0f + 36.0f;
    drawCap(lx, "TYPE", 96.0f);
    lx += 104.0f;
    drawCap(lx, "SLOPE", 56.0f);
    lx += 62.0f;
    drawCap(lx, "FREQUENCY", 66.0f);
    lx += 104.0f;
    drawCap(lx, "GAIN", 66.0f);
    lx += 104.0f;
    drawCap(lx, "Q", 66.0f);

    // Dynamic row captions
    if (dynRangeSlider.isVisible())
    {
        float l2 = panel.getX() + 16.0f;
        drawCap(l2, "DYNAMIC RANGE");
        l2 += 104.0f;
        drawCap(l2, "THRESHOLD");
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

        if (noteInOctave == 0 && (x2 - x1) > 14)
        {
            int octave = (key / 12) - 1;
    g.setColour(juce::Colour(0xFF8a8aae));
    g.setFont(makeFont(7.0f));
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
    presetSelector.setBounds(x, topBar.getY() + 4, 140, 28);
    savePresetBtn.setBounds(x + 145, topBar.getY() + 4, 36, 28);
    x += 192;
    procModeBox.setBounds(x, topBar.getY() + 4, 110, 28);
    x += 120;
    lpResolutionBox.setBounds(x, topBar.getY() + 4, 120, 28);
    x += 130;
    npResolutionBox.setBounds(x, topBar.getY() + 4, 120, 28);
    x += 130;
    displayRangeBox.setBounds(x, topBar.getY() + 4, 64, 28);
    x += 74;
    analyzerMode.setBounds(x, topBar.getY() + 4, 80, 28);
    x += 90;
    instanceSelector.setBounds(x, topBar.getY() + 4, 90, 28);
    x += 100;
    freezeBtn.setBounds(x, topBar.getY() + 4, 26, 28);
    x += 34;
    eqMatchBtn.setBounds(x, topBar.getY() + 4, 52, 28);
    x += 62;
    eqMatchCaptureBtn.setBounds(x, topBar.getY() + 4, 62, 28);
    x += 72;
    eqMatchApplyBtn.setBounds(x, topBar.getY() + 4, 46, 28);
    x += 56;
    midiLearnBtn.setBounds(x, topBar.getY() + 4, 40, 28);
    x += 48;
    pianoScaleBtn.setBounds(x, topBar.getY() + 4, 46, 28);
    x += 54;
    spectrumGrabLabel.setBounds(x, topBar.getY() + 4, 200, 28);

    undoBtn.setBounds(topBar.getRight() - 152, topBar.getY() + 4, 40, 28);
    redoBtn.setBounds(topBar.getRight() - 108, topBar.getY() + 4, 40, 28);
    undoPanelBtn.setBounds(topBar.getRight() - 64, topBar.getY() + 4, 32, 28);
    instPanelBtn.setBounds(topBar.getRight() - 28, topBar.getY() + 4, 32, 28);
    abBtn.setBounds(topBar.getRight() - 2, topBar.getY() + 4, 26, 28);

    // Side panels (Pro-Q3 style)
    const int panelW = 180;
    const int panelH = 220;
    auto panelArea = bounds.removeFromBottom(bottomBar.getHeight());
    if (instPanelVisible)
        instancePanel.setBounds(topBar.getX(), topBar.getBottom() + 8, panelW, panelH);
    if (undoPanelVisible)
        historyPanel.setBounds(topBar.getRight() - panelW, topBar.getBottom() + 8, panelW, panelH);

    // Bottom bar (global only)
    int bx = bottomBar.getX() + 8;
    int by = bottomBar.getY() + 7;
    autoGainBtn.setBounds(bx, by, 40, 26);
    bx += 48;
    panLabel.setBounds(bx, by, 28, 12);
    outputPanSlider.setBounds(bx, by + 12, 100, 14);
    bx += 108;
    gainScaleLabel.setBounds(bx, by, 34, 12);
    gainScaleSlider.setBounds(bx, by + 12, 100, 14);
    bx += 108;
    autoGainWeightLabel.setBounds(bx, by, 34, 12);
    autoGainWeightSlider.setBounds(bx, by + 12, 100, 14);
    bx += 108;
    autoGainAdvBtn.setBounds(bx, by, 30, 26);
    bx += 38;
    phaseBtn.setBounds(bx, by, 56, 26);
    bx += 64;
    globalBypassBtn.setBounds(bx, by, 56, 26);

    // === Fixed band controls strip (Pro-Q3 style) ===
    auto panel = getBandControlsBounds();
    if (!panel.isEmpty() && selectedBand >= 0)
    {
        float px = panel.getX() + 16.0f;
        float py = panel.getY() + 18.0f;
        float knobW = 66.0f;
        float knobH = 52.0f;
        float knobPitch = 104.0f;
        float smallBtn = 26.0f;
        float comboH = 26.0f;

        // Row 1: Bypass | Type | Slope | Freq | Gain | Q | Ch | GQ | Prev | # | Next | Del | Inv
        bandBypassBtn.setBounds(px, py, smallBtn, smallBtn);
        px += smallBtn + 10;
        typeBox.setBounds(px, py, 96, comboH);
        px += 104;
        slopeBox.setBounds(px, py, 56, comboH);
        px += 62;
        freqSlider.setBounds(px, py - 4, knobW, knobH);
        px += knobPitch;
        gainSlider.setBounds(px, py - 4, knobW, knobH);
        px += knobPitch;
        qSlider.setBounds(px, py - 4, knobW, knobH);
        px += knobPitch;
        channelModeBox.setBounds(px, py, 70, comboH);
        px += 78;
        gainQBtn.setBounds(px, py, smallBtn, smallBtn);
        px += smallBtn + 8;
        prevBandBtn.setBounds(px, py + 4, 22, 20);
        bandNumberLabel.setBounds(px + 24, py, 20, comboH);
        nextBandBtn.setBounds(px + 46, py + 4, 22, 20);
        px += 76;
        deleteBandBtn.setBounds(px, py, smallBtn, smallBtn);
        px += smallBtn + 8;
        invertGainBtn.setBounds(px, py, smallBtn, smallBtn);

        // Row 2 (dynamic EQ): Range | Thresh | Auto | SC | Phase
        if (dynRangeSlider.isVisible())
        {
            float dy = panel.getY() + 72.0f;
            float d2 = panel.getX() + 16.0f;
            dynRangeSlider.setBounds(d2, dy - 4, knobW, 46.0f);
            d2 += knobPitch;
            dynThreshSlider.setBounds(d2, dy - 4, knobW, 46.0f);
            d2 += knobPitch;
            dynAutoBtn.setBounds(d2, dy + 6, 46, 24);
            d2 += 54;
            scTriggerBtn.setBounds(d2, dy + 6, 40, 24);
            d2 += 48;
            phaseInvertBtn.setBounds(d2, dy + 6, 36, 24);
            d2 += 36;

            if (dynAtkSlider.isVisible()) {
                dynAtkSlider.setBounds(d2, dy - 4, knobW, 46.0f);
                d2 += knobPitch;
            }
            if (dynRelSlider.isVisible()) {
                dynRelSlider.setBounds(d2, dy - 4, knobW, 46.0f);
                d2 += knobPitch;
            }
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

void EeqEditor::showPresetMenu()
{
    juce::PopupMenu menu;

    menu.addItem(1, "Save As...", true, false);
    menu.addItem(2, "Delete Preset...", true, false);
    menu.addSeparator();

    juce::String currentName;
    int selId = presetSelector.getSelectedId();
    auto selectedText = presetSelector.getText();
    if (selId > 0 && !selectedText.startsWith("Factory:") && !selectedText.startsWith("---"))
        currentName = selectedText;
    menu.addItem(3, "Overwrite \"" + currentName + "\"", currentName.isNotEmpty(), false);

    menu.showMenuAsync(juce::PopupMenu::Options().withParentComponent(this),
        juce::ModalCallbackFunction::create([this, currentName](int result)
        {
            if (result == 1)
                savePresetToName(false, {});
            else if (result == 2)
                showDeletePresetConfirm(currentName);
            else if (result == 3 && currentName.isNotEmpty())
                showOverwriteConfirm(currentName);
        }));
}

void EeqEditor::savePresetToName(bool overwriteExisting, const juce::String& existingName)
{
    class PresetNameDialog : public juce::AlertWindow
    {
    public:
        PresetNameDialog(const juce::String& initial) : AlertWindow("Save Preset", "Enter preset name:", AlertWindow::NoIcon)
        {
            addTextEditor("name", initial, "Preset name:");
            addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
            addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        }

        juce::String getName() const { return getTextEditorContents("name"); }
    };

    auto dialog = std::make_shared<PresetNameDialog>(existingName);
    dialog->enterModalState(true, juce::ModalCallbackFunction::create([this, dialog, overwriteExisting, existingName](int result)
    {
        if (result == 1)
        {
            juce::String name = dialog->getName().trim();
            if (name.isNotEmpty())
            {
                if (!overwriteExisting && name == "Init")
                {
                    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                        "Reserved Name", "\"Init\" is reserved for the factory default preset. Please choose a different name.", "OK", this);
                    savePresetToName(false, name);
                    return;
                }
                processor.saveUserPreset(name);
                if (overwriteExisting && name != existingName)
                    processor.deleteUserPreset(existingName);
                refreshPresetList();
                // Select the newly saved preset
                for (int i = 0; i < presetSelector.getNumItems(); ++i)
                {
                    if (presetSelector.getItemText(i) == name)
                    {
                        presetSelector.setSelectedId(i + 1);
                        break;
                    }
                }
            }
        }
    }));
}

void EeqEditor::showOverwriteConfirm(const juce::String& name)
{
    juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon,
        "Overwrite Preset", "Overwrite preset \"" + name + "\" with the current settings?",
        "Overwrite", "Cancel", this,
        juce::ModalCallbackFunction::create([this, name](int result)
        {
            if (result == 1)
                processor.saveUserPreset(name);
        }));
}

void EeqEditor::showDeletePresetConfirm(const juce::String& name)
{
    if (name.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
            "Delete Preset", "Please select a user preset from the preset selector to delete it.", "OK", this);
        return;
    }
    juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon,
        "Delete Preset", "Delete preset \"" + name + "\"? This cannot be undone.",
        "Delete", "Cancel", this,
        juce::ModalCallbackFunction::create([this, name](int result)
        {
            if (result == 1)
            {
                processor.deleteUserPreset(name);
                refreshPresetList();
            }
        }));
}

void EeqEditor::loadPresetById(int itemId)
{
    if (itemId <= 0) return;
    for (int i = 0; i < presetSelector.getNumItems(); ++i)
    {
        if (presetSelector.getItemId(i) == itemId)
        {
            loadPreset(i);
            return;
        }
    }
}

void EeqEditor::loadPreset(int index)
{
    auto& apvts = processor.getAPVTS();
    if (index < 0 || index >= presetSelector.getNumItems()) return;

    // getItemText counts only item entries (no separators/headers), and the
    // first item is "Factory: Init". Factory presets are loaded by their index
    // in factoryPresetNames because a separator may shift the mapping.
    auto text = presetSelector.getItemText(index);
    if (text.startsWith("---") || text.startsWith("Factory:"))
    {
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

    if (text.isEmpty() || text.startsWith("---"))
        return;

    // User preset
    processor.loadUserPreset(text);
    processor.syncAllBandsToDSP();
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
            // Init: reset every band to its default (flat) values
            apvts.getParameter("b" + id + "_freq")->setValueNotifyingHost(
                apvts.getParameter("b" + id + "_freq")->convertTo0to1(1000.0f));
            apvts.getParameter("b" + id + "_gain")->setValueNotifyingHost(
                apvts.getParameter("b" + id + "_gain")->convertTo0to1(0.0f));
            apvts.getParameter("b" + id + "_q")->setValueNotifyingHost(
                apvts.getParameter("b" + id + "_q")->convertTo0to1(0.707f));
            apvts.getParameter("b" + id + "_type")->setValueNotifyingHost(
                apvts.getParameter("b" + id + "_type")->convertTo0to1(0));
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

    processor.syncAllBandsToDSP();
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

void EeqEditor::CollisionOverlay::update(const std::array<float, 4096>& spectrum)
{
    active = false;
    peakFreq = 0.0f;
    peakGain = -100.0f;

    // Build red shading values in log-frequency domain
    for (size_t i = 0; i < redShading.size(); ++i)
        redShading[i] = 0.0f;

    for (size_t i = 0; i < spectrum.size(); ++i)
    {
        float mag = spectrum[i];
        if (mag > peakGain)
        {
            peakGain = mag;
            peakFreq = (float)i;
        }
        if (mag > -60.0f)
            redShading[i] = mag;
    }

    if (peakGain > -60.0f)
        active = true;
}

// === InstanceListPanel implementation ===
InstanceListPanel::InstanceListPanel()
{
    setSize(180, 240);
    addBtn.setButtonText("+ Add");
    removeBtn.setButtonText("-");
    instanceNameLabel.setText("Instances", juce::dontSendNotification);

    addAndMakeVisible(addBtn);
    addAndMakeVisible(removeBtn);
    addAndMakeVisible(instanceNameLabel);

    addBtn.addListener(this);
    removeBtn.addListener(this);

    addInstance("Eeq");
}

InstanceListPanel::~InstanceListPanel() {}

void InstanceListPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient bgGrad(juce::Colour(0xff26262f), 0.0f, 0.0f,
                                juce::Colour(0xff1a1a22), 0.0f, (float)getHeight(), false);
    g.setGradientFill(bgGrad);
    g.fillRoundedRectangle(bounds, 8.0f);
    g.setColour(juce::Colour(0xff3a3a4a));
    g.drawRoundedRectangle(bounds.reduced(0.5f, 0.5f), 8.0f, 1.0f);

    g.setColour(juce::Colour(0xffe94560));
    g.setFont(makeBoldFont(15));
    g.drawText("Instance List", 6, 6, getWidth() - 12, 22, juce::Justification::left, false);

    const auto rowsStart = 58;
    const auto rowHeight = 24;
    g.setFont(makeFont(13));
    for (size_t i = 0; i < instances.size(); ++i)
    {
        auto y = static_cast<int>(rowsStart + i * rowHeight);
        if (y + rowHeight > getHeight())
            break;

        auto bg = (static_cast<int>(i) == selectedInstance) ? juce::Colour(0x33e94560)
                                                            : juce::Colour(0x33ffffff);
        g.setColour(bg);
        g.fillRect(6, y, getWidth() - 12, rowHeight - 2);

        g.setColour(instances[i].visible ? juce::Colour(0xffe9e9ef) : juce::Colour(0xff8a8a97));
        auto name = instances[i].name;
        if (!instances[i].visible)
            name = "(" + name + ")";
        g.drawText(name, 10, y, getWidth() - 74, rowHeight - 2, juce::Justification::left, false);

        // Collision toggle dot
        g.setColour(instances[i].collisionDetected ? juce::Colour(0xffe94560)
                                                   : juce::Colour(0xff3a3a45));
        g.fillEllipse(getWidth() - 46, y + 7, 10, 10);

        // Eye icon (show/hide)
        auto eyeColour = instances[i].visible ? juce::Colour(0xff00b4d8) : juce::Colour(0xff555566);
        g.setColour(eyeColour);
        g.drawEllipse(getWidth() - 28, y + 5, 14, 14, 1.5f);
        g.drawEllipse(getWidth() - 24, y + 9, 6, 6, 1.5f);
        g.fillRect(getWidth() - 22, y + 11, 2, 2);
    }
}

void InstanceListPanel::resized()
{
    addBtn.setBounds(6, 30, 90, 22);
    removeBtn.setBounds(100, 30, 40, 22);
    instanceNameLabel.setBounds(6, 54, getWidth() - 12, 16);
}

void InstanceListPanel::buttonClicked(juce::Button* btn)
{
    if (btn == &addBtn)
    {
        addInstance("Instance " + juce::String(static_cast<int>(instances.size()) + 1));
    }
    else if (btn == &removeBtn && !instances.empty())
    {
        removeInstance(static_cast<int>(instances.size()) - 1);
        repaint();
    }
}

void InstanceListPanel::mouseDown(const juce::MouseEvent& e)
{
    const int rowStart = 54;
    const int rowHeight = 24;
    const int rowIndex = ((int)e.getPosition().y - rowStart) / rowHeight;

    if (rowIndex >= 0 && rowIndex < static_cast<int>(instances.size()))
    {
        selectedInstance = rowIndex;

        // Right-click toggles collision display for that instance
        if (e.mods.isRightButtonDown())
        {
            toggleCollision(rowIndex);
        }
        else
        {
            // Eye icon area on the right toggles visibility
            juce::Rectangle<int> eyeRect(getWidth() - 34, rowStart + rowIndex * rowHeight, 20, rowHeight - 2);
            if (eyeRect.contains(e.getPosition().toInt()))
                toggleVisibility(rowIndex);
            else
                repaint();
        }
    }
}

void InstanceListPanel::mouseDoubleClick(const juce::MouseEvent& e)
{
    const int rowStart = 54;
    const int rowHeight = 24;
    const int rowIndex = ((int)e.getPosition().y - rowStart) / rowHeight;

    if (rowIndex >= 0 && rowIndex < static_cast<int>(instances.size()))
    {
        selectedInstance = rowIndex;

        // Use a modal AlertWindow for rename (JUCE 8 compatible, heap-managed).
        auto* w = new juce::AlertWindow("Rename Instance", "Enter new name:",
                                        juce::MessageBoxIconType::NoIcon);
        w->addTextEditor("name", instances[static_cast<size_t>(rowIndex)].name);
        w->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
        w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

        w->enterModalState(true,
                           juce::ModalCallbackFunction::create([this, w](int result) {
                               if (result != 0)
                               {
                                   auto newName = w->getTextEditorContents("name");
                                   if (newName.isNotEmpty())
                                       renameInstance(selectedInstance, newName);
                               }
                               delete w;
                           }));
    }
}

void InstanceListPanel::setVisibleFromPanel(int index, bool vis)
{
    if (index >= 0 && index < static_cast<int>(instances.size()))
    {
        instances[static_cast<size_t>(index)].visible = vis;
        repaint();
    }
}

void InstanceListPanel::toggleCollision(int index)
{
    if (index >= 0 && index < static_cast<int>(instances.size()))
    {
        instances[static_cast<size_t>(index)].collisionDetected = !instances[static_cast<size_t>(index)].collisionDetected;
        repaint();
    }
}

void InstanceListPanel::addInstance(const juce::String& name)
{
    InstanceRow row;
    row.id = static_cast<int>(instances.size()) + 1;
    row.name = name;
    row.visible = true;
    instances.push_back(row);
    repaint();
}

void InstanceListPanel::removeInstance(int index)
{
    if (index >= 0 && index < static_cast<int>(instances.size()))
    {
        instances.erase(instances.begin() + index);
        if (selectedInstance >= static_cast<int>(instances.size()))
            selectedInstance = -1;
    }
}

void InstanceListPanel::renameInstance(int index, const juce::String& newName)
{
    if (index >= 0 && index < static_cast<int>(instances.size()))
    {
        instances[static_cast<size_t>(index)].name = newName;
        repaint();
    }
}

void InstanceListPanel::toggleVisibility(int index)
{
    if (index >= 0 && index < static_cast<int>(instances.size()))
    {
        instances[static_cast<size_t>(index)].visible = !instances[static_cast<size_t>(index)].visible;
        repaint();
    }
}

void InstanceListPanel::setInstanceName(int index, const juce::String& name)
{
    renameInstance(index, name);
}

void InstanceListPanel::syncWithProcessor(EeqProcessor& processor)
{
    auto& list = EeqProcessor::getInstanceList();
    int currentCount = static_cast<int>(instances.size());
    int desiredCount = static_cast<int>(list.size());

    while (instances.size() < static_cast<size_t>(desiredCount))
        addInstance("Eeq " + juce::String(static_cast<int>(instances.size()) + 1));

    while (instances.size() > static_cast<size_t>(desiredCount))
    {
        int last = static_cast<int>(instances.size()) - 1;
        bool isCurrent = false;
        auto& myList = EeqProcessor::getInstanceList();
        for (auto* info : myList)
        {
            if (info->instanceId == reinterpret_cast<uintptr_t>(&processor))
                isCurrent = true;
        }
        // keep current instance's own info manageable
        (void)isCurrent;
        removeInstance(last);
    }

    for (int i = 0; i < desiredCount && i < static_cast<int>(instances.size()); ++i)
    {
        auto* info = list[static_cast<size_t>(i)];
        if (info != nullptr)
        {
            instances[static_cast<size_t>(i)].name = info->name;
            // Do NOT overwrite user-controlled visibility from the panel.
        }
    }
    repaint();
}

// === UndoHistoryPanel implementation ===
UndoHistoryPanel::UndoHistoryPanel()
{
    setSize(180, 240);
    undoBtn.setButtonText("Undo");
    redoBtn.setButtonText("Redo");
    historyLabel.setText("History: Empty", juce::dontSendNotification);

    addAndMakeVisible(undoBtn);
    addAndMakeVisible(redoBtn);
    addAndMakeVisible(historyLabel);

    undoBtn.addListener(this);
    redoBtn.addListener(this);

    addHistoryItem("Plugin loaded");
}

UndoHistoryPanel::~UndoHistoryPanel() {}

void UndoHistoryPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient bgGrad(juce::Colour(0xff26262f), 0.0f, 0.0f,
                                juce::Colour(0xff1a1a22), 0.0f, (float)getHeight(), false);
    g.setGradientFill(bgGrad);
    g.fillRoundedRectangle(bounds, 8.0f);
    g.setColour(juce::Colour(0xff3a3a4a));
    g.drawRoundedRectangle(bounds.reduced(0.5f, 0.5f), 8.0f, 1.0f);

    g.setColour(juce::Colour(0xffe94560));
    g.setFont(makeBoldFont(15));
    g.drawText("Undo History", 6, 6, getWidth() - 12, 20, juce::Justification::left, false);

    const auto rowsStart = 78;
    const auto rowHeight = 20;
    g.setFont(makeFont(12));
    g.setColour(juce::Colour(0xffa9a9b5));
    for (size_t i = 0; i < history.size() && i < 7; ++i)
    {
        auto y = static_cast<int>(rowsStart + i * rowHeight);
        g.drawText(juce::String(i + 1) + ". " + history[i].description, 10, y, getWidth() - 20,
                   rowHeight - 2, juce::Justification::left, false);
    }
}

void UndoHistoryPanel::resized()
{
    undoBtn.setBounds(6, 30, 82, 22);
    redoBtn.setBounds(92, 30, 82, 22);
    historyLabel.setBounds(6, 56, getWidth() - 12, 18);
}

void UndoHistoryPanel::buttonClicked(juce::Button* btn)
{
    if (btn == &undoBtn && canUndo())
        undo();
    else if (btn == &redoBtn && canRedo())
        redo();
}

void UndoHistoryPanel::addHistoryItem(const juce::String& description)
{
    history.push_front({description, static_cast<int>(history.size())});
    if (history.size() > static_cast<size_t>(maxHistorySize))
        history.pop_back();
    updateLabel();
    repaint();
}

void UndoHistoryPanel::clearHistory()
{
    history.clear();
    redoStack.clear();
    updateLabel();
    repaint();
}

void UndoHistoryPanel::undo()
{
    if (history.empty())
        return;

    auto item = history.front();
    history.pop_front();
    redoStack.push_front(item);
    updateLabel();
    repaint();
}

void UndoHistoryPanel::redo()
{
    if (redoStack.empty())
        return;

    auto item = redoStack.front();
    redoStack.pop_front();
    history.push_front(item);
    updateLabel();
    repaint();
}

void UndoHistoryPanel::updateLabel()
{
    historyLabel.setText("History: " + juce::String(static_cast<int>(history.size())) + " item(s)",
                         juce::dontSendNotification);
    historyLabel.setColour(juce::Label::ColourIds::textColourId, juce::Colour(0xffa9a9b5));
}

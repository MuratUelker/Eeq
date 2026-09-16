#include "PluginEditor.h"
#include <cmath>
#include <vector>

static const juce::StringArray filterTypeNames = {
    "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut", "Notch", "Band Pass", "Flat Tilt"
};

static const juce::StringArray factoryPresetNames = {
    "Init", "Vocal Presence", "De-Esser", "Guitar Bright", "Bass Tight",
    "Drum Smash", "Master Bright", "Master Warm", "Low Pass 8k", "High Pass 80"
};

static const std::vector<std::vector<float>> factoryPresets = {
    {1000, 0, 0.707, 0, false},
    {3000, 4, 1.5, 0, true},
    {6000, -6, 3.0, 0, true},
    {2000, 3, 1.0, 1, true},
    {100, 3, 0.8, 1, true},
    {4000, 3, 0.5, 0, true},
    {10000, 2, 0.7, 1, true},
    {200, 2, 0.8, 1, true},
    {8000, -80, 0.7, 4, true},
    {80, -80, 0.7, 3, true},
};

EeqEditor::EeqEditor(EeqProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(1000, 650);
    setResizable(true, true);
    setResizeLimits(700, 500, 1920, 1200);
    setWantsKeyboardFocus(true);

    freqSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    freqSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    freqSlider.setRange(20.0, 22000.0, 0.1);
    freqSlider.setTextValueSuffix(" Hz");
    freqSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFFe94560));
    freqSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFFe94560));
    freqSlider.addListener(this);
    addAndMakeVisible(freqSlider);

    gainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    gainSlider.setRange(-30.0, 30.0, 0.01);
    gainSlider.setTextValueSuffix(" dB");
    gainSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF00b4d8));
    gainSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFF00b4d8));
    gainSlider.addListener(this);
    addAndMakeVisible(gainSlider);

    qSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    qSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    qSlider.setRange(0.1, 10.0, 0.01);
    qSlider.setSkewFactor(0.4);
    qSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF533483));
    qSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFF533483));
    qSlider.addListener(this);
    addAndMakeVisible(qSlider);

    for (auto* label : {&freqLabel, &gainLabel, &qLabel})
    {
        label->setJustificationType(juce::Justification::centred);
        label->setFont(juce::Font(11.0f));
        label->setColour(juce::Label::textColourId, juce::Colour(0xFFa0a0c0));
        addAndMakeVisible(*label);
    }

    for (const auto& name : filterTypeNames)
        typeBox.addItem(name, typeBox.getNumItems() + 1);
    typeBox.setSelectedId(1);
    typeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF1a1a2e));
    typeBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    addAndMakeVisible(typeBox);
    typeLabel.setJustificationType(juce::Justification::centred);
    typeLabel.setFont(juce::Font(11.0f));
    typeLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFa0a0c0));
    addAndMakeVisible(typeLabel);

    activeToggle.setButtonText("On");
    activeToggle.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFe94560));
    activeToggle.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xFFe94560));
    addAndMakeVisible(activeToggle);

    bypassToggle.setButtonText("Byp");
    bypassToggle.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    addAndMakeVisible(bypassToggle);

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

    analyzerMode.addItem("Pre", 1);
    analyzerMode.addItem("Post", 2);
    analyzerMode.addItem("Off", 3);
    analyzerMode.setSelectedId(1);
    analyzerMode.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF16213e));
    analyzerMode.setColour(juce::ComboBox::textColourId, juce::Colour(0xFFe0e0ff));
    addAndMakeVisible(analyzerMode);

    globalBypassBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    addAndMakeVisible(globalBypassBtn);

    autoGainBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFa0a0c0));
    autoGainBtn.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(autoGainBtn);

    startTimerHz(30);
}

EeqEditor::~EeqEditor()
{
    stopTimer();
}

void EeqEditor::timerCallback() { repaint(); }

juce::Rectangle<float> EeqEditor::getTopBarBounds() const
{
    return getLocalBounds().toFloat().removeFromTop(40);
}

juce::Rectangle<float> EeqEditor::getDisplayBounds() const
{
    auto b = getLocalBounds().toFloat();
    b.removeFromTop(40);
    return b.removeFromBottom(b.getHeight() - 110);
}

juce::Rectangle<float> EeqEditor::getControlsBounds() const
{
    auto b = getLocalBounds().toFloat();
    b.removeFromTop(40);
    b.removeFromBottom(b.getHeight() - 110);
    return b;
}

float EeqEditor::freqToX(float freq, float w) const
{
    float logMin = std::log10(MIN_FREQ);
    float logMax = std::log10(MAX_FREQ);
    return ((std::log10(freq) - logMin) / (logMax - logMin)) * w;
}

float EeqEditor::xToFreq(float x, float w) const
{
    float logMin = std::log10(MIN_FREQ);
    float logMax = std::log10(MAX_FREQ);
    return std::pow(10.0f, logMin + (x / w) * (logMax - logMin));
}

float EeqEditor::gainToY(float gain, float h) const
{
    return (h * 0.5f) - (gain / MAX_DB) * (h * 0.5f);
}

float EeqEditor::yToGain(float y, float h) const
{
    return ((h * 0.5f) - y) / (h * 0.5f) * MAX_DB;
}

float EeqEditor::qToRadius(float q) const
{
    return juce::jmap(q, 0.1f, 10.0f, 10.0f, 4.0f);
}

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

void EeqEditor::addBandAt(float freq, float gain)
{
    for (int i = 0; i < NUM_BANDS; ++i)
    {
        if (!bandVisuals[i].active)
        {
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

void EeqEditor::selectBand(int idx)
{
    selectedBand = idx;
    for (auto& bv : bandVisuals) bv.selected = false;
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

    float freq = processor.getAPVTS().getParameter("b" + id + "_freq")->getValue();
    float gain = processor.getAPVTS().getParameter("b" + id + "_gain")->getValue();
    float q = processor.getAPVTS().getParameter("b" + id + "_q")->getValue();
    int type = (int)(processor.getAPVTS().getParameter("b" + id + "_type")->getValue() * 7.99f);
    bool active = processor.getAPVTS().getParameter("b" + id + "_active")->getValue() > 0.5f;

    freqSlider.setValue(freq * (MAX_FREQ - MIN_FREQ) + MIN_FREQ, juce::dontSendNotification);
    gainSlider.setValue(gain * (MAX_DB - MIN_DB) + MIN_DB, juce::dontSendNotification);
    qSlider.setValue(q * 9.9f + 0.1f, juce::dontSendNotification);
    typeBox.setSelectedId(type + 1, juce::dontSendNotification);
    activeToggle.setToggleState(active, juce::dontSendNotification);
}

void EeqEditor::updateBandFromControls(int idx)
{
    if (idx < 0 || idx >= NUM_BANDS) return;
    auto id = juce::String(idx + 1);

    float freq = (float)freqSlider.getValue();
    float gain = (float)gainSlider.getValue();
    float q = (float)qSlider.getValue();
    int type = typeBox.getSelectedId() - 1;
    bool active = activeToggle.getToggleState();

    processor.getAPVTS().getParameter("b" + id + "_freq")->setValueNotifyingHost(
        processor.getAPVTS().getParameter("b" + id + "_freq")->convertTo0to1(freq));
    processor.getAPVTS().getParameter("b" + id + "_gain")->setValueNotifyingHost(
        processor.getAPVTS().getParameter("b" + id + "_gain")->convertTo0to1(gain));
    processor.getAPVTS().getParameter("b" + id + "_q")->setValueNotifyingHost(
        processor.getAPVTS().getParameter("b" + id + "_q")->convertTo0to1(q));
    processor.getAPVTS().getParameter("b" + id + "_type")->setValueNotifyingHost(
        processor.getAPVTS().getParameter("b" + id + "_type")->convertTo0to1(type));
    processor.getAPVTS().getParameter("b" + id + "_active")->setValueNotifyingHost(active ? 1.0f : 0.0f);
}

void EeqEditor::updateBandFromMouse(int band, float mx, float my)
{
    auto display = getDisplayBounds();
    float freq = xToFreq(mx - display.getX(), display.getWidth());
    float gain = yToGain(my - display.getY(), display.getHeight());
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
            menu.addItem(100, "Bypass Band");
            menu.addItem(101, "Delete Band");

            menu.showMenuAsync(juce::PopupMenu::Options(), [this, hit](int result)
            {
                if (result == 0) return;
                auto id = juce::String(hit + 1);
                if (result == 100)
                {
                    bool bypassed = bypassToggle.getToggleState();
                    bypassToggle.setToggleState(!bypassed, juce::sendNotification);
                }
                else if (result == 101)
                {
                    processor.getAPVTS().getParameter("b" + id + "_active")->setValueNotifyingHost(0.0f);
                    bandVisuals[hit].active = false;
                    if (selectedBand == hit) selectBand(-1);
                }
                else if (result >= 1 && result <= filterTypeNames.size())
                {
                    processor.getAPVTS().getParameter("b" + id + "_type")->setValueNotifyingHost(
                        processor.getAPVTS().getParameter("b" + id + "_type")->convertTo0to1(result - 1));
                    updateControlsFromBand(hit);
                }
            });
        }
        else
        {
            float freq = xToFreq(mx - display.getX(), display.getWidth());
            float gain = yToGain(my - display.getY(), display.getHeight());
            menu.addItem(1, "Add Bell Band Here");
            menu.addItem(2, "Add Low Cut Here");
            menu.addItem(3, "Add High Cut Here");
            menu.showMenuAsync(juce::PopupMenu::Options(), [this, freq, gain](int result)
            {
                if (result == 0) return;
                for (int i = 0; i < NUM_BANDS; ++i)
                {
                    if (!bandVisuals[i].active)
                    {
                        auto id = juce::String(i + 1);
                        processor.getAPVTS().getParameter("b" + id + "_freq")->setValueNotifyingHost(
                            processor.getAPVTS().getParameter("b" + id + "_freq")->convertTo0to1(freq));
                        processor.getAPVTS().getParameter("b" + id + "_gain")->setValueNotifyingHost(
                            processor.getAPVTS().getParameter("b" + id + "_gain")->convertTo0to1(gain));
                        int type = result == 2 ? 3 : result == 3 ? 4 : 0;
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
            bool bypassed = processor.getAPVTS().getParameter(
                "b" + juce::String(hit + 1) + "_active")->getValue() > 0.5f;
            processor.getAPVTS().getParameter("b" + juce::String(hit + 1) + "_active")
                ->setValueNotifyingHost(bypassed ? 0.0f : 1.0f);
        }
    }
    else if (e.getNumberOfClicks() >= 2)
    {
        float freq = xToFreq(mx - display.getX(), display.getWidth());
        float gain = yToGain(my - display.getY(), display.getHeight());
        addBandAt(freq, gain);
    }
}

void EeqEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (dragging && selectedBand >= 0)
        updateBandFromMouse(selectedBand, e.position.x, e.position.y);
}

void EeqEditor::mouseUp(const juce::MouseEvent&) { dragging = false; }

void EeqEditor::mouseDoubleClick(const juce::MouseEvent& e)
{
    auto display = getDisplayBounds();
    if (!display.contains(e.position.x, e.position.y)) return;
    float freq = xToFreq(e.position.x - display.getX(), display.getWidth());
    float gain = yToGain(e.position.y - display.getY(), display.getHeight());
    addBandAt(freq, gain);
}

void EeqEditor::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (selectedBand < 0) return;
    auto id = juce::String(selectedBand + 1);
    auto* param = processor.getAPVTS().getParameter("b" + id + "_q");

    if (e.mods.isAltDown())
    {
        float newFreq = freqSlider.getValue() * (1.0f + wheel.deltaY * 0.05f);
        newFreq = juce::jlimit(MIN_FREQ, MAX_FREQ, newFreq);
        freqSlider.setValue(newFreq, juce::sendNotification);
    }
    else if (e.mods.isShiftDown())
    {
        float newGain = gainSlider.getValue() + wheel.deltaY * 0.5f;
        newGain = juce::jlimit(MIN_DB, MAX_DB, newGain);
        gainSlider.setValue(newGain, juce::sendNotification);
    }
    else
    {
        float newQ = qSlider.getValue() * (1.0f + wheel.deltaY * 0.1f);
        newQ = juce::jlimit(0.1f, 10.0f, newQ);
        qSlider.setValue(newQ, juce::sendNotification);
    }
}

void EeqEditor::mouseMove(const juce::MouseEvent& e)
{
    auto display = getDisplayBounds();
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
            auto id = juce::String(selectedBand + 1);
            processor.getAPVTS().getParameter("b" + id + "_active")->setValueNotifyingHost(0.0f);
            bandVisuals[selectedBand].active = false;
            selectBand(-1);
        }
        return true;
    }
    return false;
}

void EeqEditor::sliderValueChanged(juce::Slider*)
{
    if (selectedBand >= 0)
        updateBandFromControls(selectedBand);
}

void EeqEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF0a0a1a));

    auto topBar = getTopBarBounds();
    g.setColour(juce::Colour(0xFF0f0f2a));
    g.fillRect(topBar);

    g.setColour(juce::Colour(0xFFe94560));
    g.setFont(juce::Font(18.0f).boldened());
    g.drawText("EEQ", topBar.reduced(10, 0), juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xFF6a6a8e));
    g.setFont(11.0f);
    g.drawText("PRESET", topBar.getX() + 55, topBar.getY() + 2, 45, 16, juce::Justification::centredRight);
    g.drawText("ANALYZER", topBar.getX() + 275, topBar.getY() + 2, 55, 16, juce::Justification::centredRight);

    auto display = getDisplayBounds();
    drawGrid(g, display);
    if (analyzerMode.getSelectedId() != 3)
        drawSpectrum(g, display);
    drawEQCurve(g, display);
    drawBandNodes(g, display);
    drawBandInfo(g, display);
}

void EeqEditor::drawGrid(juce::Graphics& g, juce::Rectangle<float> d)
{
    g.setColour(juce::Colour(0xFF15152a));
    float freqs[] = {20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};
    for (float f : freqs)
    {
        float x = d.getX() + freqToX(f, d.getWidth());
        g.drawLine(x, d.getY(), x, d.getBottom(), 1.0f);
    }

    for (int dB = -30; dB <= 30; dB += 6)
    {
        float y = d.getY() + gainToY((float)dB, d.getHeight());
        g.drawLine(d.getX(), y, d.getRight(), y, 1.0f);
    }

    g.setColour(juce::Colour(0xFF2a2a4a));
    float zeroY = d.getY() + gainToY(0.0f, d.getHeight());
    g.drawLine(d.getX(), zeroY, d.getRight(), zeroY, 1.5f);

    g.setColour(juce::Colour(0xFF4a4a6e));
    g.setFont(9.0f);
    const char* labels[] = {"20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k"};
    for (int i = 0; i < 10; ++i)
    {
        float x = d.getX() + freqToX(freqs[i], d.getWidth());
        g.drawText(labels[i], x - 12, d.getBottom() - 14, 24, 12, juce::Justification::centred);
    }

    for (int dB : {-30, -20, -10, 0, 10, 20, 30})
    {
        float y = d.getY() + gainToY((float)dB, d.getHeight());
        juce::String txt = (dB >= 0 ? "+" : "") + juce::String(dB);
        g.drawText(txt, d.getX() + 2, y - 7, 28, 14, juce::Justification::centredLeft);
    }
}

void EeqEditor::drawSpectrum(juce::Graphics& g, juce::Rectangle<float> d)
{
    const auto& spec = processor.getSpectrumAnalyzer();
    const auto& data = spec.getSpectrumData();

    juce::Path path;
    bool started = false;
    int numDisplayBins = (int)(d.getWidth());

    for (int px = 0; px < numDisplayBins; ++px)
    {
        float freq = xToFreq((float)px, d.getWidth());
        int bin = (int)(freq / (float)(44100.0 / 2.0) * NUM_BINS);
        bin = juce::jlimit(0, NUM_BINS - 1, bin);
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
}

void EeqEditor::drawEQCurve(juce::Graphics& g, juce::Rectangle<float> d)
{
    juce::Path path;
    bool started = false;

    for (int px = 0; px < (int)d.getWidth(); ++px)
    {
        float freq = xToFreq((float)px, d.getWidth());
        float magDB = 20.0f * std::log10(std::max(processor.getEqualizer().getMagnitudeAtFreq(freq), 1e-10f));
        magDB = juce::jlimit(MIN_DB, MAX_DB, magDB);

        float x = d.getX() + (float)px;
        float y = d.getY() + gainToY(magDB, d.getHeight());

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
    for (int i = 0; i < NUM_BANDS; ++i)
    {
        auto id = juce::String(i + 1);
        bool active = processor.getAPVTS().getParameter("b" + id + "_active")->getValue() > 0.5f;
        bandVisuals[i].active = active;

        if (!active) continue;

        float freq = processor.getAPVTS().getParameter("b" + id + "_freq")->getValue()
                     * (MAX_FREQ - MIN_FREQ) + MIN_FREQ;
        float gain = processor.getAPVTS().getParameter("b" + id + "_gain")->getValue()
                     * (MAX_DB - MIN_DB) + MIN_DB;
        float q = processor.getAPVTS().getParameter("b" + id + "_q")->getValue() * 9.9f + 0.1f;

        float x = d.getX() + freqToX(freq, d.getWidth());
        float y = d.getY() + gainToY(gain, d.getHeight());

        bandVisuals[i].x = x;
        bandVisuals[i].y = y;

        float radius = qToRadius(q);
        auto col = bandColours[i % 24];

        if (bandVisuals[i].selected)
        {
            g.setColour(col.withAlpha(0.2f));
            g.fillEllipse(x - radius - 4, y - radius - 4, (radius + 4) * 2, (radius + 4) * 2);

            g.setColour(col.withAlpha(0.4f));
            juce::String info = filterTypeNames[typeBox.getSelectedId() - 1] + "\n"
                + juce::String(freq, 0) + " Hz\n"
                + juce::String(gain, 1) + " dB\nQ: " + juce::String(q, 2);
            g.setFont(10.0f);
            g.drawText(info, x - 40, y - radius - 50, 80, 45, juce::Justification::centred);
        }

        if (bandVisuals[i].hovered && !bandVisuals[i].selected)
        {
            g.setColour(col.withAlpha(0.15f));
            g.fillEllipse(x - radius - 2, y - radius - 2, (radius + 2) * 2, (radius + 2) * 2);
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
    }
}

void EeqEditor::drawBandInfo(juce::Graphics& g, juce::Rectangle<float> d)
{
    g.setColour(juce::Colour(0xFF4a4a6e));
    g.setFont(10.0f);
    int activeCount = 0;
    for (int i = 0; i < NUM_BANDS; ++i)
        if (bandVisuals[i].active) activeCount++;
    g.drawText(juce::String(activeCount) + " / " + juce::String(NUM_BANDS) + " bands",
               d.getX() + 4, d.getY() + 2, 80, 14, juce::Justification::centredLeft);
}

void EeqEditor::resized()
{
    auto bounds = getLocalBounds();
    auto topBar = bounds.removeFromTop(40);

    freqSlider.setBounds(topBar.getX() + 340, topBar.getY() + 2, 55, 36);
    freqLabel.setBounds(topBar.getX() + 340, topBar.getY() + 2, 55, 10);
    gainSlider.setBounds(topBar.getX() + 400, topBar.getY() + 2, 55, 36);
    gainLabel.setBounds(topBar.getX() + 400, topBar.getY() + 2, 55, 10);
    qSlider.setBounds(topBar.getX() + 460, topBar.getY() + 2, 55, 36);
    qLabel.setBounds(topBar.getX() + 460, topBar.getY() + 2, 55, 10);
    typeBox.setBounds(topBar.getX() + 525, topBar.getY() + 8, 80, 24);
    typeLabel.setBounds(topBar.getX() + 525, topBar.getY() + 1, 80, 10);
    activeToggle.setBounds(topBar.getX() + 615, topBar.getY() + 8, 40, 24);
    bypassToggle.setBounds(topBar.getX() + 660, topBar.getY() + 8, 40, 24);

    presetSelector.setBounds(topBar.getX() + 100, topBar.getY() + 8, 130, 24);
    savePresetBtn.setBounds(topBar.getX() + 235, topBar.getY() + 8, 35, 24);

    analyzerMode.setBounds(topBar.getX() + 340, topBar.getY() - 30, 80, 0);

    auto controls = bounds.removeFromBottom(110);
    (void)controls;

    presetSelector.toFront(true);
    savePresetBtn.toFront(true);
    freqSlider.toFront(true);
    gainSlider.toFront(true);
    qSlider.toFront(true);
    typeBox.toFront(true);
}

void EeqEditor::loadPreset(int index)
{
    if (index < 0 || index >= (int)factoryPresets.size()) return;
    const auto& preset = factoryPresets[index];

    for (int i = 0; i < NUM_BANDS; ++i)
    {
        auto id = juce::String(i + 1);
        bool bandActive = (i == 0 && index != 0) ? (i < (int)preset.size() / 5) : false;

        if (index == 0)
        {
            processor.getAPVTS().getParameter("b" + id + "_active")->setValueNotifyingHost(0.0f);
        }
        else if (i < (int)preset.size() / 5)
        {
            int base = i * 5;
            processor.getAPVTS().getParameter("b" + id + "_freq")->setValueNotifyingHost(
                processor.getAPVTS().getParameter("b" + id + "_freq")->convertTo0to1(preset[base]));
            processor.getAPVTS().getParameter("b" + id + "_gain")->setValueNotifyingHost(
                processor.getAPVTS().getParameter("b" + id + "_gain")->convertTo0to1(preset[base + 1]));
            processor.getAPVTS().getParameter("b" + id + "_q")->setValueNotifyingHost(
                processor.getAPVTS().getParameter("b" + id + "_q")->convertTo0to1(preset[base + 2]));
            processor.getAPVTS().getParameter("b" + id + "_type")->setValueNotifyingHost(
                processor.getAPVTS().getParameter("b" + id + "_type")->convertTo0to1((int)preset[base + 3]));
            processor.getAPVTS().getParameter("b" + id + "_active")->setValueNotifyingHost(
                preset[base + 4] > 0.5f ? 1.0f : 0.0f);
        }
        else
        {
            processor.getAPVTS().getParameter("b" + id + "_active")->setValueNotifyingHost(0.0f);
        }
    }

    selectBand(-1);
}

void EeqEditor::savePreset(const juce::String& /*name*/) {}
void EeqEditor::initPresets() {}

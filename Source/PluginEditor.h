#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class EeqEditor : public juce::AudioProcessorEditor,
                   public juce::Timer,
                   private juce::Slider::Listener
{
public:
    EeqEditor(EeqProcessor&);
    ~EeqEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMove(const juce::MouseEvent&) override;
    bool keyPressed(const juce::KeyPress&) override;

    void sliderValueChanged(juce::Slider*) override;

private:
    EeqProcessor& processor;

    static constexpr int NUM_BANDS = EeqProcessor::MAX_BANDS;
    static constexpr float MIN_FREQ = 20.0f;
    static constexpr float MAX_FREQ = 22000.0f;
    static constexpr float MIN_DB = -30.0f;
    static constexpr float MAX_DB = 30.0f;

    struct BandVisual
    {
        float x = 0, y = 0;
        bool active = false;
        bool hovered = false;
        bool selected = false;
    };
    std::array<BandVisual, NUM_BANDS> bandVisuals;
    int selectedBand = -1;
    int hoveredBand = -1;
    bool dragging = false;

    // Band controls panel
    juce::Slider freqSlider, gainSlider, qSlider;
    juce::ComboBox typeBox;
    juce::ToggleButton activeToggle, bypassToggle;
    juce::Label freqLabel{"", "Freq"}, gainLabel{"", "Gain"}, qLabel{"", "Q"}, typeLabel{"", "Type"};

    juce::ComboBox presetSelector;
    juce::TextButton savePresetBtn{"Save"};
    juce::TextButton deletePresetBtn{"Del"};

    juce::ComboBox analyzerMode;
    juce::ToggleButton globalBypassBtn{"Bypass"};
    juce::ToggleButton autoGainBtn{"Auto Gain"};

    juce::Colour bandColours[24] = {
        juce::Colour(0xFFe94560), juce::Colour(0xFF00b4d8), juce::Colour(0xFF533483),
        juce::Colour(0xFFe76f51), juce::Colour(0xFF2a9d8f), juce::Colour(0xFFe9c46a),
        juce::Colour(0xFF0077b6), juce::Colour(0xFFf4a261), juce::Colour(0xFF606c38),
        juce::Colour(0xFFbc6c25), juce::Colour(0xFF90e0ef), juce::Colour(0xFFdda15e),
        juce::Colour(0xFF283618), juce::Colour(0xFF023e8a), juce::Colour(0xFFad2831),
        juce::Colour(0xFF48cae4), juce::Colour(0xFFd62828), juce::Colour(0xFF7209b7),
        juce::Colour(0xFF3a0ca3), juce::Colour(0xFF4cc9f0), juce::Colour(0xFFf72585),
        juce::Colour(0xFF4361ee), juce::Colour(0xFF7209b7), juce::Colour(0xFF560bad),
    };

    float freqToX(float freq, float w) const;
    float xToFreq(float x, float w) const;
    float gainToY(float gain, float h) const;
    float yToGain(float y, float h) const;
    float qToRadius(float q) const;

    int findBandAt(float mx, float my) const;
    void updateBandFromMouse(int band, float mx, float my);
    void selectBand(int idx);
    void updateControlsFromBand(int idx);
    void updateBandFromControls(int idx);
    void addBandAt(float freq, float gain);

    void drawGrid(juce::Graphics&, juce::Rectangle<float>);
    void drawSpectrum(juce::Graphics&, juce::Rectangle<float>);
    void drawEQCurve(juce::Graphics&, juce::Rectangle<float>);
    void drawBandNodes(juce::Graphics&, juce::Rectangle<float>);
    void drawBandInfo(juce::Graphics&, juce::Rectangle<float>);

    void setupSlider(juce::Slider& s, juce::Label& l, juce::String name,
                     std::function<void(float)> cb);

    juce::Rectangle<float> getDisplayBounds() const;
    juce::Rectangle<float> getControlsBounds() const;
    juce::Rectangle<float> getTopBarBounds() const;

    void loadPreset(int index);
    void savePreset(const juce::String& name);
    void initPresets();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EeqEditor)
};

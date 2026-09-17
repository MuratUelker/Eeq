#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class EeqEditor : public juce::AudioProcessorEditor,
                   public juce::Timer,
                   private juce::Slider::Listener,
                   private juce::ComboBox::Listener,
                   private juce::Button::Listener
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
    void comboBoxChanged(juce::ComboBox*) override;
    void buttonClicked(juce::Button*) override;

private:
    EeqProcessor& processor;

    static constexpr int NUM_BANDS = EeqProcessor::MAX_BANDS;
    static constexpr float MIN_FREQ = 20.0f;
    static constexpr float MAX_FREQ = 22000.0f;
    static constexpr float MIN_DB = -30.0f;
    static constexpr float MAX_DB = 30.0f;
    static constexpr int NUM_PIANO_KEYS = 128;

    struct BandVisual
    {
        float x = 0, y = 0;
        bool active = false;
        bool hovered = false;
        bool selected = false;
        bool soloed = false;
        bool bypassed = false;
    };
    std::array<BandVisual, NUM_BANDS> bandVisuals;
    int selectedBand = -1;
    int hoveredBand = -1;
    bool dragging = false;
    bool fullScreen = false;
    bool spectrumGrabbing = false;
    float spectrumGrabFreq = 0.0f;
    float spectrumGrabGain = 0.0f;

    // Horizontal zoom
    float hZoom = 1.0f;
    float hScroll = 0.0f;

    // Multi-band selection
    std::vector<int> multiSelectedBands;

    // === Top bar controls ===
    juce::ComboBox presetSelector;
    juce::TextButton savePresetBtn{"Save"};
    juce::ComboBox procModeBox;
    juce::ComboBox lpResolutionBox;
    juce::ComboBox analyzerMode;
    juce::ToggleButton freezeBtn{"F"};
    juce::ToggleButton abBtn{"A"};
    juce::TextButton undoBtn{"Undo"};
    juce::TextButton redoBtn{"Redo"};
    juce::ToggleButton fullScreenBtn{"FS"};

    // === Floating band controls (Pro-Q3 style) ===
    juce::ToggleButton bandBypassBtn{"B"};
    juce::ComboBox typeBox;
    juce::ComboBox slopeBox;
    juce::Slider freqSlider, gainSlider, qSlider;
    juce::ComboBox channelModeBox;
    juce::ToggleButton gainQBtn{"GQ"};
    juce::ToggleButton prevBandBtn{"<"};
    juce::ToggleButton nextBandBtn{">"};
    juce::TextButton deleteBandBtn{"X"};
    juce::Label bandNumberLabel{"", "1"};

    // Dynamic EQ (row 2 of floating panel)
    juce::Slider dynRangeSlider;
    juce::Slider dynThreshSlider;
    juce::ToggleButton dynAutoBtn{"Auto"};
    juce::ToggleButton scTriggerBtn{"SC"};
    juce::ToggleButton phaseInvertBtn{"Ø"};

    static const juce::StringArray slopeNames;

    // === Bottom bar controls (global) ===
    juce::ToggleButton phaseBtn{"Phase"};
    juce::ToggleButton autoGainBtn{"AG"};
    juce::Slider outputPanSlider;
    juce::Label panLabel{"", "Pan"};
    juce::Slider gainScaleSlider;
    juce::Label gainScaleLabel{"", "Scale"};

    // Output meter
    float outputLevelL = 0.0f, outputLevelR = 0.0f;

    // EQ Match
    juce::ToggleButton eqMatchBtn{"Match"};
    juce::TextButton eqMatchCaptureBtn{"Capture"};
    juce::TextButton eqMatchApplyBtn{"Apply"};
    bool eqMatchCapturing = false;

    // Colours
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

    // Layout helpers
    float freqToX(float freq, juce::Rectangle<float> d) const;
    float xToFreq(float x, juce::Rectangle<float> d) const;
    float gainToY(float gain, juce::Rectangle<float> d) const;
    float yToGain(float y, juce::Rectangle<float> d) const;
    float qToRadius(float q) const;

    int findBandAt(float mx, float my) const;
    void updateBandFromMouse(int band, float mx, float my);
    void selectBand(int idx);
    void updateControlsFromBand(int idx);
    void updateBandFromControls(int idx);
    void addBandAt(float freq, float gain);
    void navigateBand(int direction);

    // Drawing
    void drawGrid(juce::Graphics&, juce::Rectangle<float>);
    void drawSpectrum(juce::Graphics&, juce::Rectangle<float>);
    void drawEQCurve(juce::Graphics&, juce::Rectangle<float>);
    void drawBandNodes(juce::Graphics&, juce::Rectangle<float>);
    void drawBandInfo(juce::Graphics&, juce::Rectangle<float>);
    void drawPianoRoll(juce::Graphics&, juce::Rectangle<float>);
    void drawOutputMeter(juce::Graphics&, juce::Rectangle<float>);
    void drawBandControls(juce::Graphics&, juce::Rectangle<float>);

    // Bounds
    juce::Rectangle<float> getDisplayBounds() const;
    juce::Rectangle<float> getTopBarBounds() const;
    juce::Rectangle<float> getBottomBarBounds() const;
    juce::Rectangle<float> getPianoBounds() const;
    juce::Rectangle<float> getMeterBounds() const;
    juce::Rectangle<float> getBandControlsBounds() const;

    void loadPreset(int index);
    void refreshPresetList();
    void showSavePresetDialog();
    void loadFactoryPreset(int index);
    void updateAllControlsFromProcessor();

    int freqToMidiKey(float freq) const;
    float midiKeyToFreq(int key) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EeqEditor)
};

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_opengl/juce_opengl.h>
#include "PluginProcessor.h"

// ComboBox subclass that fires reload even when the same item is re-selected.
// JUCE's stock ComboBox only fires onChange when the selected ID actually
// changes, so re-picking the already-selected preset would never reload it.
class ReloadableComboBox : public juce::ComboBox
{
public:
    std::function<void(int itemId)> onPopupSelection;
    bool popupVisible = false;

    using juce::ComboBox::ComboBox;

    void showPopup() override
    {
        if (popupVisible)
            return;

        popupVisible = true;

        auto selectedId = getSelectedId();
        juce::PopupMenu menu;
        for (int i = 0; i < getNumItems(); ++i)
        {
            auto itemId = getItemId(i);
            if (itemId != 0)
                menu.addItem(itemId, getItemText(i), true, itemId == selectedId);
        }

        if (menu.getNumItems() > 0)
        {
            auto options = juce::PopupMenu::Options()
                               .withTargetComponent(this)
                               .withItemThatMustBeVisible(selectedId)
                               .withInitiallySelectedItem(selectedId)
                               .withMinimumWidth(getWidth())
                               .withMaximumNumColumns(1)
                               .withStandardItemHeight(26);
            menu.showMenuAsync(options,
                juce::ModalCallbackFunction::create([this](int result)
                {
                    popupVisible = false;
                    hidePopup();
                    if (result != 0)
                    {
                        setSelectedId(result, juce::dontSendNotification);
                        if (onPopupSelection)
                            onPopupSelection(result);
                    }
                }));
        }
        else
        {
            popupVisible = false;
            hidePopup();
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReloadableComboBox)
};

// === Instance List Panel ===
class InstanceListPanel : public juce::Component, private juce::Button::Listener {
public:
    InstanceListPanel();
    ~InstanceListPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void buttonClicked(juce::Button*) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;

    // Instance management
    void addInstance(const juce::String& name);
    void removeInstance(int index);
    void renameInstance(int index, const juce::String& newName);
    void toggleVisibility(int index);
    void setInstanceName(int index, const juce::String& name);
    void syncWithProcessor(EeqProcessor& processor);

    int getInstanceCount() const { return static_cast<int>(instances.size()); }
    bool isVisible(int index) const { return index >= 0 && index < static_cast<int>(instances.size()) ? instances[static_cast<size_t>(index)].visible : false; }
    void setCollision(int index, bool collision) { if (index >= 0 && index < static_cast<int>(instances.size())) instances[static_cast<size_t>(index)].collisionDetected = collision; repaint(); }
    void setVisibleFromPanel(int index, bool vis);
    void toggleCollision(int index);

private:
    struct InstanceRow {
        int id = 0;
        juce::String name;
        bool visible = true;
        bool collisionDetected = false;
        float spectrumPeak = -100.0f;
    };

    std::vector<InstanceRow> instances;
    int selectedInstance = -1;
    juce::TextButton addBtn{"+ Add"};
    juce::TextButton removeBtn{"- Remove"};
    juce::Label instanceNameLabel{"", "Instance Name"};
    juce::ComboBox instanceSelector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstanceListPanel)
};

// === Undo History Panel ===
class UndoHistoryPanel : public juce::Component, private juce::Button::Listener {
public:
    UndoHistoryPanel();
    ~UndoHistoryPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void buttonClicked(juce::Button*) override;

    // History management
    void addHistoryItem(const juce::String& description);
    void clearHistory();
    void undo();
    void redo();
    bool canUndo() const { return !history.empty(); }
    bool canRedo() const { return !redoStack.empty(); }
    void updateLabel();

private:
    struct HistoryItem {
        juce::String description;
        int timestamp = 0;
    };

    std::deque<HistoryItem> history;
    std::deque<HistoryItem> redoStack;
    int maxHistorySize = 50;
    juce::TextButton undoBtn{"Undo"};
    juce::TextButton redoBtn{"Redo"};
    juce::Label historyLabel{"", "History: Empty"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UndoHistoryPanel)
};

class EeqEditor : public juce::AudioProcessorEditor,
                   public juce::Timer,
                   public juce::OpenGLRenderer,
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

    // OpenGLRenderer
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

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
    juce::Rectangle<int> previousBounds;
    bool pianoScale = false;
    bool spectrumGrabbing = false;
    float spectrumGrabFreq = 0.0f;
    float spectrumGrabGain = 0.0f;

    // Horizontal zoom
    float hZoom = 1.0f;
    float hScroll = 0.0f;

    // Multi-band selection
    std::vector<int> multiSelectedBands;

    // === Top bar controls ===
    ReloadableComboBox presetSelector;
    juce::TextButton savePresetBtn{"Save"};
    juce::ComboBox procModeBox;
    juce::ComboBox lpResolutionBox;
    juce::ComboBox npResolutionBox;
    juce::ComboBox analyzerMode;
    juce::ComboBox displayRangeBox;
    juce::ComboBox instanceSelector;
    juce::ToggleButton freezeBtn{"F"};
    juce::ToggleButton abBtn{"A"};
    juce::TextButton undoBtn{"Undo"};
    juce::TextButton redoBtn{"Redo"};
    juce::ToggleButton fullScreenBtn{"FS"};
    juce::ToggleButton pianoScaleBtn{"Piano"};
    juce::ToggleButton instPanelBtn{"Inst"};
    juce::ToggleButton undoPanelBtn{"Hist"};

    // === Side panels (Pro-Q3 style) ===
    InstanceListPanel instancePanel;
    UndoHistoryPanel historyPanel;
    bool instPanelVisible = false;
    bool undoPanelVisible = false;
    int logTickCount = 0;

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
    juce::TextButton invertGainBtn{"Inv"};
    juce::Label bandNumberLabel{"", "1"};

    // Dynamic EQ (row 2 of floating panel)
    juce::Slider dynRangeSlider;
    juce::Slider dynThreshSlider;
    juce::Slider dynAtkSlider;
    juce::Slider dynRelSlider;
    juce::ToggleButton dynAutoBtn{"Auto"};
    juce::ToggleButton scTriggerBtn{"SC"};
    juce::ToggleButton phaseInvertBtn{"Ø"};

    static const juce::StringArray slopeNames;

    // === Bottom bar controls (global) ===
    juce::ToggleButton phaseBtn{"Phase"};
    juce::ToggleButton globalBypassBtn{"Byp"};
    juce::ToggleButton autoGainBtn{"AG"};
    juce::ToggleButton autoGainAdvBtn{"Adv"};
    juce::Slider outputPanSlider;
    juce::Label panLabel{"", "Pan"};
    juce::Slider gainScaleSlider;
    juce::Label gainScaleLabel{"", "Scale"};
    juce::Slider autoGainWeightSlider;
    juce::Label autoGainWeightLabel{"", "Weight"};
    juce::Label spectrumGrabLabel{"", "Grab: —"};

    // Output meter
    float outputLevelL = 0.0f, outputLevelR = 0.0f;

    // EQ Match
    juce::ToggleButton eqMatchBtn{"Match"};
    juce::TextButton eqMatchCaptureBtn{"Capture"};
    juce::TextButton eqMatchApplyBtn{"Apply"};
    bool eqMatchCapturing = false;

    // MIDI Learn
    juce::ToggleButton midiLearnBtn{"MIDI"};
    bool midiLearnActive = false;
    struct MidiMapping { int band; juce::String param; int cc; int channel; };
    std::vector<MidiMapping> midiMappings;

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
    juce::Rectangle<float> getBandControlStripBounds() const;
    float getBandControlStripHeight() const;

    void loadPreset(int index);
    void loadPresetById(int itemId);
    void refreshPresetList();
    void showPresetMenu();
    void savePresetToName(bool overwriteExisting, const juce::String& existingName);
    void showOverwriteConfirm(const juce::String& name);
    void showDeletePresetConfirm(const juce::String& name);
    void loadFactoryPreset(int index);
    void updateAllControlsFromProcessor();

    // OpenGL
    void initializeOpenGL();
    void shutdownOpenGL();

    int freqToMidiKey(float freq) const;
    float midiKeyToFreq(int key) const;

    juce::OpenGLContext openGLContext;
    juce::TooltipWindow tooltipWindow;
    bool useOpenGL = true;

    // === Collision Visual Overlay (for spectrum grab) ===
    struct CollisionOverlay {
        std::array<float, 4096> redShading{};
        bool active = false;
        float peakFreq = 0.0f;
        float peakGain = -100.0f;

        void update(const std::array<float, 4096>& spectrum);
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EeqEditor)
};


#pragma once
#include "BiquadFilter.h"
#include <array>
#include <cstring>
#include <complex>
#include <vector>

static constexpr int MAX_BANDS = 24;

enum class ProcessingMode
{
    ZeroLatency,
    NaturalPhase,
    LinearPhase
};

enum class LinearPhaseResolution
{
    Low = 1024,
    Medium = 2048,
    High = 4096,
    VeryHigh = 8192
};

enum class NaturalPhaseResolution
{
    Low = 1024,
    Medium = 2048,
    High = 4096,
    VeryHigh = 8192
};

class Equalizer
{
public:
    Equalizer() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void process(float* left, float* right, int numSamples);
    void processMultiChannel(float** channels, int numChannels, int numSamples);
    void processLinearPhase(float* left, float* right, int numSamples);
    void setBand(int index, const BandState& state);
    const BandState& getBand(int index) const { return bands[index]; }
    float getMagnitudeAtFreq(float freq) const;
    void setProcessingMode(ProcessingMode mode) { procMode = mode; }
    ProcessingMode getProcessingMode() const { return procMode; }

    void setLinearPhaseResolution(LinearPhaseResolution res) { lpResolution = res; responseDirty = true; }
    LinearPhaseResolution getLinearPhaseResolution() const { return lpResolution; }
    int getFFTSize() const { return static_cast<int>(lpResolution); }

    void setGainScale(float scale) { gainScale = scale; }
    float getGainScale() const { return gainScale; }

    void setSidechainLevels(float scL, float scR) { scLevelL = scL; scLevelR = scR; }

    // Natural Phase
    void processNaturalPhase(float* left, float* right, int numSamples);
    void setNaturalPhaseResolution(NaturalPhaseResolution res) { npResolution = res; responseDirty = true; }
    NaturalPhaseResolution getNaturalPhaseResolution() const { return npResolution; }
    int getNaturalPhaseFFTSize() const { return static_cast<int>(npResolution); }

    // Smart parameter interpolation
    void setBandSmoothed(int index, const BandState& state, int numSamples);
    void setSmoothingTime(float ms) { smoothingTimeMs = ms; }
    float getSmoothingTime() const { return smoothingTimeMs; }

    static constexpr int NUM_BANDS = MAX_BANDS;

private:
    static constexpr int MAX_FILTERS_PER_BAND = 16;

    std::array<std::array<BiquadFilter, MAX_FILTERS_PER_BAND>, MAX_BANDS> filterStages;
    std::array<BandState, MAX_BANDS> bands;
    std::array<BandState, MAX_BANDS> targetBands;
    double currentSampleRate = 44100.0;
    int blockSize = 512;
    ProcessingMode procMode = ProcessingMode::ZeroLatency;
    LinearPhaseResolution lpResolution = LinearPhaseResolution::High;
    NaturalPhaseResolution npResolution = NaturalPhaseResolution::High;
    float gainScale = 1.0f;
    float smoothingTimeMs = 20.0f; // Default 20ms smoothing

    // Dynamic EQ envelope
    std::array<float, MAX_BANDS> envelope{};
    float scLevelL = 0.0f;
    float scLevelR = 0.0f;

    // Linear phase FFT (dynamic size based on resolution)
    int fftSize = 4096;
    int fftHalf = 2048;
    std::vector<float> overlapL;
    std::vector<float> overlapR;
    std::vector<float> inputBufferL;
    std::vector<float> inputBufferR;
    int lpWritePos = 0;
    bool lpReady = false;

    // Natural phase FFT (uses same FFT size as linear phase but different processing)
    int npFftSize = 4096;
    int npFftHalf = 2048;
    std::vector<float> npOverlapL;
    std::vector<float> npOverlapR;
    std::vector<float> npInputBufferL;
    std::vector<float> npInputBufferR;
    int npWritePos = 0;
    bool npReady = false;

    // FFT workspace
    std::vector<std::complex<float>> fftWindow;
    std::vector<float> windowCoeffs;
    std::vector<std::complex<float>> eqResponse;
    bool responseDirty = true;

    void allocateLinearPhaseBuffers(int fftSize);
    void allocateNaturalPhaseBuffers(int fftSize);
    void computeEQFrequencyResponse(std::complex<float>* response, int numBins, float sampleRate);
    void fftInPlace(std::complex<float>* data, int n, bool inverse);
    void processDynamicEQ(int bandIdx, float& gain, float inputLevel);
};

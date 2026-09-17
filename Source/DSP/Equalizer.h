#pragma once
#include "BiquadFilter.h"
#include <array>
#include <cstring>
#include <complex>

static constexpr int MAX_BANDS = 24;

enum class ProcessingMode
{
    ZeroLatency,
    NaturalPhase,
    LinearPhase
};

class Equalizer
{
public:
    Equalizer() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void process(float* left, float* right, int numSamples);
    void processLinearPhase(float* left, float* right, int numSamples);
    void setBand(int index, const BandState& state);
    const BandState& getBand(int index) const { return bands[index]; }
    float getMagnitudeAtFreq(float freq) const;
    void setProcessingMode(ProcessingMode mode) { procMode = mode; }
    ProcessingMode getProcessingMode() const { return procMode; }

    void setGainScale(float scale) { gainScale = scale; }
    float getGainScale() const { return gainScale; }

    void setSidechainLevels(float scL, float scR) { scLevelL = scL; scLevelR = scR; }

    static constexpr int NUM_BANDS = MAX_BANDS;

private:
    static constexpr int MAX_FILTERS_PER_BAND = 8;

    std::array<std::array<BiquadFilter, MAX_FILTERS_PER_BAND>, MAX_BANDS> filterStages;
    std::array<BandState, MAX_BANDS> bands;
    double currentSampleRate = 44100.0;
    int blockSize = 512;
    ProcessingMode procMode = ProcessingMode::ZeroLatency;
    float gainScale = 1.0f;

    // Dynamic EQ envelope
    std::array<float, MAX_BANDS> envelope{};
    float scLevelL = 0.0f;
    float scLevelR = 0.0f;

    // Linear phase FFT
    static constexpr int FFT_SIZE = 4096;
    static constexpr int FFT_HALF = FFT_SIZE / 2;
    static constexpr int OVERLAP = FFT_HALF;
    std::array<float, FFT_SIZE> overlapL{};
    std::array<float, FFT_SIZE> overlapR{};
    std::array<float, FFT_SIZE> inputBufferL{};
    std::array<float, FFT_SIZE> inputBufferR{};
    int lpWritePos = 0;
    bool lpReady = false;

    // FFT workspace
    std::array<std::complex<float>, FFT_SIZE> fftWindow{};
    std::array<float, FFT_SIZE> windowCoeffs{};
    std::array<std::complex<float>, FFT_SIZE> eqResponse{};
    bool responseDirty = true;

    void computeEQFrequencyResponse(std::complex<float>* response, int numBins, float sampleRate);
    void fftInPlace(std::complex<float>* data, int n, bool inverse);
    void processDynamicEQ(int bandIdx, float& gain, float inputLevel);
};

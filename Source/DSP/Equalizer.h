#pragma once
#include "BiquadFilter.h"
#include <array>
#include <cstring>

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

    static constexpr int NUM_BANDS = MAX_BANDS;

private:
    std::array<BiquadFilter, MAX_BANDS> filters;
    std::array<BandState, MAX_BANDS> bands;
    double currentSampleRate = 44100.0;
    int blockSize = 512;
    ProcessingMode procMode = ProcessingMode::ZeroLatency;
    float gainScale = 1.0f;

    // Dynamic EQ envelope
    std::array<float, MAX_BANDS> envelope{};

    // Linear phase FIR
    static constexpr int FIR_SIZE = 4096;
    std::array<float, FIR_SIZE> firBufferL{};
    std::array<float, FIR_SIZE> firBufferR{};
    int firWritePos = 0;

    void processDynamicEQ(int bandIdx, float& gain, float inputLevel);
};

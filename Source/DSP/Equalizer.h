#pragma once
#include "BiquadFilter.h"
#include <array>

static constexpr int MAX_BANDS = 24;

struct BandState
{
    float freq = 1000.0f;
    float gain = 0.0f;
    float q = 0.707f;
    FilterType type = FilterType::Bell;
    bool active = false;
    bool bypassed = false;
};

class Equalizer
{
public:
    Equalizer() = default;

    void prepare(double sampleRate, int samplesPerBlock);
    void process(float* left, float* right, int numSamples);

    void setBand(int index, const BandState& state);
    const BandState& getBand(int index) const { return bands[index]; }
    float getMagnitudeAtFreq(float freq) const;

    static constexpr int NUM_BANDS = MAX_BANDS;

private:
    std::array<BiquadFilter, MAX_BANDS> filters;
    std::array<BandState, MAX_BANDS> bands;
    double currentSampleRate = 44100.0;
};

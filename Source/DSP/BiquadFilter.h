#pragma once
#include <cmath>
#include <algorithm>

enum class FilterType
{
    Bell,
    LowShelf,
    HighShelf,
    LowCut,
    HighCut,
    Notch,
    BandPass,
    FlatTilt
};

struct BiquadCoeffs
{
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;
};

class BiquadFilter
{
public:
    BiquadFilter() = default;

    void prepare(double sampleRate);
    void process(float* data, int numSamples);
    void processStereo(float* left, float* right, int numSamples);
    void reset();
    void setParams(float freq, float gainDb, float q, FilterType type);
    float getMagnitude(float freq) const;

private:
    double fs = 44100.0;
    BiquadCoeffs current{};
    FilterType currentType = FilterType::Bell;
    float currentFreq = 1000.0f;
    float currentGain = 0.0f;
    float currentQ = 1.0f;

    float x1L = 0, x2L = 0, y1L = 0, y2L = 0;
    float x1R = 0, x2R = 0, y1R = 0, y2R = 0;

    void calcCoefficients();
    inline float processSingle(float x, float& x1, float& x2, float& y1, float& y2) const;
};

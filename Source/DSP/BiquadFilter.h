#pragma once
#include <cmath>
#include <algorithm>
#include <cstring>

enum class FilterType
{
    Bell,
    LowShelf,
    HighShelf,
    LowCut,
    HighCut,
    Notch,
    BandPass,
    FlatTilt,
    TiltShelf
};

enum class ChannelMode
{
    Stereo,
    Left,
    Right,
    Mid,
    Side
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
    void processStereo(float* left, float* right, int numSamples);
    void processMidSide(float* left, float* right, int numSamples, bool isMid);
    void processLeft(float* left, int numSamples);
    void processRight(float* right, int numSamples);
    void reset();

    void setParams(float freq, float gainDb, float q, FilterType type);
    float getMagnitude(float freq) const;

    BiquadCoeffs getCoeffs() const { return current; }

private:
    double fs = 44100.0;
    BiquadCoeffs current{};
    FilterType currentType = FilterType::Bell;
    float currentFreq = 1000.0f;
    float currentGain = 0.0f;
    float currentQ = 0.707f;

    float x1L = 0, x2L = 0, y1L = 0, y2L = 0;
    float x1R = 0, x2R = 0, y1R = 0, y2R = 0;

    void calcCoefficients();
    inline float processSingle(float x, float& x1, float& x2, float& y1, float& y2) const;
};

struct DynamicState
{
    bool enabled = false;
    float dynamicRange = 0.0f;
    float threshold = -20.0f;
    bool autoThreshold = true;
    float envelope = 0.0f;
    float attackMs = 10.0f;
    float releaseMs = 100.0f;
};

enum class FilterSlope
{
    Slope6 = 0,
    Slope12,
    Slope18,
    Slope24,
    Slope30,
    Slope36,
    Slope42,
    Slope48
};

inline int slopeToStages(FilterSlope s) { return ((int)s + 1); }
inline float slopeToDB(FilterSlope s) { return 6.0f * ((int)s + 1); }

struct BandState
{
    float freq = 1000.0f;
    float gain = 0.0f;
    float q = 0.707f;
    FilterType type = FilterType::Bell;
    FilterSlope slope = FilterSlope::Slope24;
    bool active = false;
    bool bypassed = false;
    bool soloed = false;
    ChannelMode channelMode = ChannelMode::Stereo;
    DynamicState dynamic;
    bool scTrigger = false;
};

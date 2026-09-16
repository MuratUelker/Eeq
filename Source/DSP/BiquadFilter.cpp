#include "BiquadFilter.h"
#include <cmath>

void BiquadFilter::prepare(double sampleRate)
{
    fs = sampleRate;
    reset();
    calcCoefficients();
}

void BiquadFilter::reset()
{
    x1L = x2L = y1L = y2L = 0.0f;
    x1R = x2R = y1R = y2R = 0.0f;
}

void BiquadFilter::setParams(float freq, float gainDb, float q, FilterType type)
{
    currentType = type;
    currentFreq = freq;
    currentGain = gainDb;
    currentQ = q;
    calcCoefficients();
}

inline float BiquadFilter::processSingle(float x, float& x1, float& x2, float& y1, float& y2) const
{
    float y = current.b0 * x + current.b1 * x1 + current.b2 * x2 - current.a1 * y1 - current.a2 * y2;
    x2 = x1; x1 = x;
    y2 = y1; y1 = y;
    return y;
}

void BiquadFilter::process(float* data, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
        data[i] = processSingle(data[i], x1L, x2L, y1L, y2L);
}

void BiquadFilter::processStereo(float* left, float* right, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        left[i] = processSingle(left[i], x1L, x2L, y1L, y2L);
        right[i] = processSingle(right[i], x1R, x2R, y1R, y2R);
    }
}

float BiquadFilter::getMagnitude(float freq) const
{
    float w = 2.0f * 3.14159265f * freq / (float)fs;
    float cosw = std::cos(w);
    float sinw = std::sin(w);
    float cos2w = std::cos(2.0f * w);

    float numMag = std::sqrt(
        current.b0 * current.b0 + current.b1 * current.b1 + current.b2 * current.b2
        + 2.0f * (current.b0 * current.b1 + current.b1 * current.b2) * cosw
        + 2.0f * current.b0 * current.b2 * cos2w);

    float denMag = std::sqrt(
        1.0f + current.a1 * current.a1 + current.a2 * current.a2
        + 2.0f * (current.a1 + current.a1 * current.a2) * cosw
        + 2.0f * current.a2 * cos2w);

    return (denMag > 1e-10f) ? (numMag / denMag) : numMag;
}

void BiquadFilter::calcCoefficients()
{
    float A = std::pow(10.0f, currentGain / 40.0f);
    float clampedFreq = std::min(currentFreq, (float)fs * 0.499f);
    float w0 = 2.0f * 3.14159265f * clampedFreq / (float)fs;
    float cosw = std::cos(w0);
    float sinw = std::sin(w0);
    float alpha = sinw / (2.0f * currentQ);

    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;

    switch (currentType)
    {
    case FilterType::Bell:
    {
        float aH = alpha * A;
        float aD = alpha / A;
        b0 = 1.0f + aH; b1 = -2.0f * cosw; b2 = 1.0f - aH;
        a0 = 1.0f + aD; a1 = -2.0f * cosw; a2 = 1.0f - aD;
        break;
    }
    case FilterType::LowShelf:
    {
        float s2a = 2.0f * std::sqrt(A) * alpha;
        b0 = A * ((A + 1.0f) - (A - 1.0f) * cosw + s2a);
        b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw);
        b2 = A * ((A + 1.0f) - (A - 1.0f) * cosw - s2a);
        a0 = (A + 1.0f) + (A - 1.0f) * cosw + s2a;
        a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosw);
        a2 = (A + 1.0f) + (A - 1.0f) * cosw - s2a;
        break;
    }
    case FilterType::HighShelf:
    {
        float s2a = 2.0f * std::sqrt(A) * alpha;
        b0 = A * ((A + 1.0f) + (A - 1.0f) * cosw + s2a);
        b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw);
        b2 = A * ((A + 1.0f) + (A - 1.0f) * cosw - s2a);
        a0 = (A + 1.0f) - (A - 1.0f) * cosw + s2a;
        a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosw);
        a2 = (A + 1.0f) - (A - 1.0f) * cosw - s2a;
        break;
    }
    case FilterType::LowCut:
    {
        b0 = (1.0f - cosw) * 0.5f; b1 = 1.0f - cosw; b2 = (1.0f - cosw) * 0.5f;
        a0 = 1.0f + alpha; a1 = -2.0f * cosw; a2 = 1.0f - alpha;
        break;
    }
    case FilterType::HighCut:
    {
        b0 = (1.0f + cosw) * 0.5f; b1 = -(1.0f + cosw); b2 = (1.0f + cosw) * 0.5f;
        a0 = 1.0f + alpha; a1 = -2.0f * cosw; a2 = 1.0f - alpha;
        break;
    }
    case FilterType::Notch:
    {
        b0 = 1.0f; b1 = -2.0f * cosw; b2 = 1.0f;
        a0 = 1.0f + alpha; a1 = -2.0f * cosw; a2 = 1.0f - alpha;
        break;
    }
    case FilterType::BandPass:
    {
        b0 = alpha; b1 = 0.0f; b2 = -alpha;
        a0 = 1.0f + alpha; a1 = -2.0f * cosw; a2 = 1.0f - alpha;
        break;
    }
    case FilterType::FlatTilt:
    {
        float k = std::tan(3.14159265f * clampedFreq / (float)fs);
        float k2 = k * k;
        float sqrtA = std::sqrt(A);
        float denom = 1.0f + k / currentQ + k2;
        float alphaF = (1.0f / sqrtA - sqrtA) * k;
        b0 = (A + alphaF + k2 * A - alphaF * A) / denom;
        b1 = 2.0f * (k2 * A - A) / denom;
        b2 = (A - alphaF + k2 * A + alphaF * A) / denom;
        a1 = 2.0f * (k2 - 1.0f) / denom;
        a2 = (1.0f - k / currentQ + k2) / denom;
        break;
    }
    }

    float inv = 1.0f / a0;
    current.b0 = b0 * inv; current.b1 = b1 * inv; current.b2 = b2 * inv;
    current.a1 = a1 * inv; current.a2 = a2 * inv;
}

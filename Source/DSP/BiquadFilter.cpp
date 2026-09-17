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

void BiquadFilter::processStereo(float* left, float* right, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        left[i] = processSingle(left[i], x1L, x2L, y1L, y2L);
        right[i] = processSingle(right[i], x1R, x2R, y1R, y2R);
    }
}

void BiquadFilter::processMidSide(float* left, float* right, int numSamples, bool isMid)
{
    for (int i = 0; i < numSamples; ++i)
    {
        float mid = left[i] + right[i];
        float side = left[i] - right[i];

        if (isMid)
            mid = processSingle(mid, x1L, x2L, y1L, y2L);
        else
            side = processSingle(side, x1L, x2L, y1L, y2L);

        left[i] = (mid + side) * 0.5f;
        right[i] = (mid - side) * 0.5f;
    }
}

void BiquadFilter::processLeft(float* left, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
        left[i] = processSingle(left[i], x1L, x2L, y1L, y2L);
}

void BiquadFilter::processRight(float* right, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
        right[i] = processSingle(right[i], x1R, x2R, y1R, y2R);
}

float BiquadFilter::getMagnitude(float freq) const
{
    float w = 2.0f * 3.14159265f * freq / (float)fs;
    float cosw = std::cos(w);
    float sinw = std::sin(w);
    float cos2w = std::cos(2.0f * w);
    float sin2w = std::sin(2.0f * w);

    float numR = current.b0 + current.b1 * cosw + current.b2 * cos2w;
    float numI = -(current.b1 * sinw + current.b2 * sin2w);
    float numMag2 = numR * numR + numI * numI;

    float denR = 1.0f + current.a1 * cosw + current.a2 * cos2w;
    float denI = -(current.a1 * sinw + current.a2 * sin2w);
    float denMag2 = denR * denR + denI * denI;

    if (denMag2 < 1e-20f) denMag2 = 1e-20f;

    return std::sqrt(numMag2 / denMag2);
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
        // HighPass (cuts lows, passes highs)
        b0 = (1.0f + cosw) * 0.5f; b1 = -(1.0f + cosw); b2 = (1.0f + cosw) * 0.5f;
        a0 = 1.0f + alpha; a1 = -2.0f * cosw; a2 = 1.0f - alpha;
        break;
    }
    case FilterType::HighCut:
    {
        // LowPass (cuts highs, passes lows)
        b0 = (1.0f - cosw) * 0.5f; b1 = 1.0f - cosw; b2 = (1.0f - cosw) * 0.5f;
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
    case FilterType::TiltShelf:
    {
        float k = std::tan(3.14159265f * clampedFreq / (float)fs);
        float k2 = k * k;
        float sqrtA = std::sqrt(A);
        float alphaF = sqrtA * k;
        float denom = 1.0f + alphaF / currentQ + k2;
        b0 = (A + k2 * A + alphaF * (A - 1.0f)) / denom;
        b1 = 2.0f * (k2 * A - A) / denom;
        b2 = (A + k2 * A - alphaF * (A - 1.0f)) / denom;
        a1 = 2.0f * (k2 - 1.0f) / denom;
        a2 = (1.0f - alphaF / currentQ + k2) / denom;
        break;
    }
    }

    float inv = 1.0f / a0;
    current.b0 = b0 * inv; current.b1 = b1 * inv; current.b2 = b2 * inv;
    current.a1 = a1 * inv; current.a2 = a2 * inv;
}

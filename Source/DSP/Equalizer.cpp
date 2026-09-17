#include "Equalizer.h"
#include <cmath>
#include <algorithm>

void Equalizer::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    blockSize = samplesPerBlock;
    for (auto& f : filters)
        f.prepare(sampleRate);
    envelope.fill(0.0f);

    // Initialize window coefficients (Hann)
    for (int i = 0; i < FFT_SIZE; ++i)
        windowCoeffs[i] = 0.5f * (1.0f - std::cos(2.0f * 3.14159265f * (float)i / (float)(FFT_SIZE - 1)));

    overlapL.fill(0.0f);
    overlapR.fill(0.0f);
    inputBufferL.fill(0.0f);
    inputBufferR.fill(0.0f);
    lpWritePos = 0;
    lpReady = false;
}

void Equalizer::processDynamicEQ(int bandIdx, float& gain, float inputLevel)
{
    auto& dyn = bands[bandIdx].dynamic;
    if (!dyn.enabled) return;

    float threshold = dyn.autoThreshold ? -20.0f : dyn.threshold;
    float attack = 1.0f - std::exp(-1.0f / (currentSampleRate * dyn.attackMs / 1000.0f));
    float release = 1.0f - std::exp(-1.0f / (currentSampleRate * dyn.releaseMs / 1000.0f));

    float levelDB = 20.0f * std::log10(std::max(inputLevel, 1e-10f));

    if (levelDB > envelope[bandIdx])
        envelope[bandIdx] += attack * (levelDB - envelope[bandIdx]);
    else
        envelope[bandIdx] += release * (levelDB - envelope[bandIdx]);

    float over = envelope[bandIdx] - threshold;
    if (over > 0.0f)
    {
        float reduction = over * (dyn.dynamicRange / 30.0f);
        gain += reduction;
    }
}

void Equalizer::setBand(int index, const BandState& state)
{
    if (index < 0 || index >= MAX_BANDS) return;
    bands[index] = state;
    if (state.active)
    {
        float scaledGain = state.gain * gainScale;
        filters[index].setParams(state.freq, scaledGain, state.q, state.type);
    }
}

void Equalizer::process(float* left, float* right, int numSamples)
{
    bool hasSolo = false;
    for (int i = 0; i < MAX_BANDS; ++i)
        if (bands[i].soloed) { hasSolo = true; break; }

    for (int i = 0; i < MAX_BANDS; ++i)
    {
        if (!bands[i].active || bands[i].bypassed) continue;
        if (hasSolo && !bands[i].soloed) continue;

        float effectiveGain = bands[i].gain * gainScale;

        if (bands[i].dynamic.enabled)
        {
            float inputLevel = 0.0f;
            for (int s = 0; s < numSamples; ++s)
                inputLevel += left[s] * left[s];
            inputLevel = std::sqrt(inputLevel / (float)numSamples);
            processDynamicEQ(i, effectiveGain, inputLevel);
        }

        filters[i].setParams(bands[i].freq, effectiveGain, bands[i].q, bands[i].type);

        switch (bands[i].channelMode)
        {
        case ChannelMode::Stereo:
            filters[i].processStereo(left, right, numSamples);
            break;
        case ChannelMode::Mid:
            filters[i].processMidSide(left, right, numSamples, true);
            break;
        case ChannelMode::Side:
            filters[i].processMidSide(left, right, numSamples, false);
            break;
        case ChannelMode::Left:
            filters[i].processLeft(left, numSamples);
            break;
        case ChannelMode::Right:
            filters[i].processRight(right, numSamples);
            break;
        }
    }
}

void Equalizer::fftInPlace(std::complex<float>* data, int n, bool inverse)
{
    // Bit-reversal permutation
    int bits = 0;
    for (int temp = n; temp > 1; temp >>= 1) ++bits;

    for (int i = 0; i < n; ++i)
    {
        int j = 0;
        for (int b = 0; b < bits; ++b)
            j = (j << 1) | ((i >> b) & 1);
        if (i < j)
            std::swap(data[i], data[j]);
    }

    // Butterfly stages
    float sign = inverse ? 1.0f : -1.0f;
    for (int size = 2; size <= n; size *= 2)
    {
        int halfSize = size / 2;
        float angle = sign * 2.0f * 3.14159265f / (float)size;
        std::complex<float> wLen(std::cos(angle), std::sin(angle));

        for (int start = 0; start < n; start += size)
        {
            std::complex<float> w(1.0f, 0.0f);
            for (int k = 0; k < halfSize; ++k)
            {
                int even = start + k;
                int odd = start + k + halfSize;
                std::complex<float> t = w * data[odd];
                data[odd] = data[even] - t;
                data[even] = data[even] + t;
                w *= wLen;
            }
        }
    }

    if (inverse)
    {
        float invN = 1.0f / (float)n;
        for (int i = 0; i < n; ++i)
            data[i] *= invN;
    }
}

void Equalizer::computeEQFrequencyResponse(std::complex<float>* response, int numBins, float sampleRate)
{
    // Initialize to flat response
    for (int i = 0; i < numBins; ++i)
        response[i] = std::complex<float>(1.0f, 0.0f);

    bool hasSolo = false;
    for (int i = 0; i < MAX_BANDS; ++i)
        if (bands[i].soloed) { hasSolo = true; break; }

    for (int b = 0; b < MAX_BANDS; ++b)
    {
        if (!bands[b].active || bands[b].bypassed) continue;
        if (hasSolo && !bands[b].soloed) continue;

        float effectiveGain = bands[b].gain * gainScale;
        float freq = bands[b].freq;
        float q = bands[b].q;
        float A = std::pow(10.0f, effectiveGain / 40.0f);
        float w0 = 2.0f * 3.14159265f * freq / sampleRate;
        float cosw = std::cos(w0);
        float sinw = std::sin(w0);
        float alpha = sinw / (2.0f * q);

        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;

        switch (bands[b].type)
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
            float k = std::tan(3.14159265f * freq / sampleRate);
            float k2 = k * k;
            float sqrtA = std::sqrt(A);
            float denom = 1.0f + k / q + k2;
            float alphaF = (1.0f / sqrtA - sqrtA) * k;
            b0 = (A + alphaF + k2 * A - alphaF * A) / denom;
            b1 = 2.0f * (k2 * A - A) / denom;
            b2 = (A - alphaF + k2 * A + alphaF * A) / denom;
            a1 = 2.0f * (k2 - 1.0f) / denom;
            a2 = (1.0f - k / q + k2) / denom;
            a0 = 1.0f;
            break;
        }
        case FilterType::TiltShelf:
        {
            float k = std::tan(3.14159265f * freq / sampleRate);
            float k2 = k * k;
            float sqrtA = std::sqrt(A);
            float alphaF = sqrtA * k;
            float denom = 1.0f + alphaF / q + k2;
            b0 = (A + k2 * A + alphaF * (A - 1.0f)) / denom;
            b1 = 2.0f * (k2 * A - A) / denom;
            b2 = (A + k2 * A - alphaF * (A - 1.0f)) / denom;
            a1 = 2.0f * (k2 - 1.0f) / denom;
            a2 = (1.0f - alphaF / q + k2) / denom;
            a0 = 1.0f;
            break;
        }
        }

        float invA0 = 1.0f / a0;
        b0 *= invA0; b1 *= invA0; b2 *= invA0;
        a1 *= invA0; a2 *= invA0;

        // Compute H(e^jw) for each FFT bin
        for (int k = 0; k < numBins; ++k)
        {
            float w = 2.0f * 3.14159265f * (float)k / (float)(numBins * 2);
            float cosW = std::cos(w);
            float sinW = std::sin(w);
            float cos2W = std::cos(2.0f * w);
            float sin2W = std::sin(2.0f * w);

            float numR = b0 + b1 * cosW + b2 * cos2W;
            float numI = -(b1 * sinW + b2 * sin2W);
            float denR = 1.0f + a1 * cosW + a2 * cos2W;
            float denI = -(a1 * sinW + a2 * sin2W);

            float denMag2 = denR * denR + denI * denI;
            if (denMag2 < 1e-20f) denMag2 = 1e-20f;

            float hR = (numR * denR + numI * denI) / denMag2;
            float hI = (numI * denR - numR * denI) / denMag2;

            response[k] *= std::complex<float>(hR, hI);
        }
    }
}

void Equalizer::processLinearPhase(float* left, float* right, int numSamples)
{
    if (numSamples <= 0) return;

    // Compute frequency response once per block if parameters changed
    static std::array<std::complex<float>, FFT_SIZE> eqResponse{};
    static bool responseDirty = true;

    // Check if any band changed (simplified dirty check)
    eqResponse[0] = std::complex<float>(1.0f, 0.0f);
    computeEQFrequencyResponse(eqResponse.data(), FFT_HALF, (float)currentSampleRate);

    // Mirror for negative frequencies
    for (int k = 1; k < FFT_HALF; ++k)
        eqResponse[FFT_SIZE - k] = std::conj(eqResponse[k]);

    for (int s = 0; s < numSamples; ++s)
    {
        inputBufferL[lpWritePos] = left[s];
        inputBufferR[lpWritePos] = right[s];
        lpWritePos++;

        if (lpWritePos >= FFT_SIZE)
        {
            lpWritePos = 0;
            lpReady = true;

            // Apply window and FFT left channel
            std::array<std::complex<float>, FFT_SIZE> fftL{};
            for (int i = 0; i < FFT_SIZE; ++i)
                fftL[i] = std::complex<float>(inputBufferL[i] * windowCoeffs[i], 0.0f);
            fftInPlace(fftL.data(), FFT_SIZE, false);

            // Apply EQ response
            for (int i = 0; i < FFT_SIZE; ++i)
                fftL[i] *= eqResponse[i];

            // IFFT
            fftInPlace(fftL.data(), FFT_SIZE, true);

            // Overlap-add for left
            for (int i = 0; i < FFT_HALF; ++i)
            {
                float sample = fftL[i].real() * windowCoeffs[i] + overlapL[i];
                overlapL[i] = fftL[i + FFT_HALF].real() * windowCoeffs[i + FFT_HALF];
                inputBufferL[i] = sample;
            }

            // Apply window and FFT right channel
            std::array<std::complex<float>, FFT_SIZE> fftR{};
            for (int i = 0; i < FFT_SIZE; ++i)
                fftR[i] = std::complex<float>(inputBufferR[i] * windowCoeffs[i], 0.0f);
            fftInPlace(fftR.data(), FFT_SIZE, false);

            // Apply EQ response
            for (int i = 0; i < FFT_SIZE; ++i)
                fftR[i] *= eqResponse[i];

            // IFFT
            fftInPlace(fftR.data(), FFT_SIZE, true);

            // Overlap-add for right
            for (int i = 0; i < FFT_HALF; ++i)
            {
                float sample = fftR[i].real() * windowCoeffs[i] + overlapR[i];
                overlapR[i] = fftR[i + FFT_HALF].real() * windowCoeffs[i + FFT_HALF];
                inputBufferR[i] = sample;
            }
        }
    }

    // Output processed samples from the overlap-add buffer
    int readStart = lpReady ? 0 : 0;
    int avail = lpReady ? FFT_HALF : lpWritePos;
    int toRead = std::min(numSamples, avail);

    for (int s = 0; s < toRead; ++s)
    {
        left[s] = inputBufferL[s];
        right[s] = inputBufferR[s];
    }

    // If we need more samples than available, fill rest with zero-latency fallback
    if (numSamples > avail)
    {
        for (int i = 0; i < MAX_BANDS; ++i)
        {
            if (!bands[i].active || bands[i].bypassed) continue;
            float effectiveGain = bands[i].gain * gainScale;
            filters[i].setParams(bands[i].freq, effectiveGain, bands[i].q, bands[i].type);

            switch (bands[i].channelMode)
            {
            case ChannelMode::Stereo:
                filters[i].processStereo(left + avail, right + avail, numSamples - avail);
                break;
            case ChannelMode::Mid:
                filters[i].processMidSide(left + avail, right + avail, numSamples - avail, true);
                break;
            case ChannelMode::Side:
                filters[i].processMidSide(left + avail, right + avail, numSamples - avail, false);
                break;
            case ChannelMode::Left:
                filters[i].processLeft(left + avail, numSamples - avail);
                break;
            case ChannelMode::Right:
                filters[i].processRight(right + avail, numSamples - avail);
                break;
            }
        }
    }
}

float Equalizer::getMagnitudeAtFreq(float freq) const
{
    float mag = 1.0f;
    for (int i = 0; i < MAX_BANDS; ++i)
    {
        if (bands[i].active && !bands[i].bypassed)
        {
            float scaledGain = bands[i].gain * gainScale;
            BiquadFilter tempFilter;
            const_cast<BiquadFilter&>(tempFilter).prepare(currentSampleRate);
            const_cast<BiquadFilter&>(tempFilter).setParams(bands[i].freq, scaledGain, bands[i].q, bands[i].type);
            mag *= tempFilter.getMagnitude(freq);
        }
    }
    return mag;
}

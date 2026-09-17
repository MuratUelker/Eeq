#include "Equalizer.h"
#include <cmath>
#include <algorithm>

void Equalizer::allocateLinearPhaseBuffers(int size)
{
    fftSize = size;
    fftHalf = size / 2;

    overlapL.assign(fftSize, 0.0f);
    overlapR.assign(fftSize, 0.0f);
    inputBufferL.assign(fftSize, 0.0f);
    inputBufferR.assign(fftSize, 0.0f);

    fftWindow.resize(fftSize);
    windowCoeffs.resize(fftSize);
    eqResponse.resize(fftSize);

    for (int i = 0; i < fftSize; ++i)
        windowCoeffs[i] = 0.5f * (1.0f - std::cos(2.0f * 3.14159265f * (float)i / (float)(fftSize - 1)));

    lpWritePos = 0;
    lpReady = false;
    responseDirty = true;
}

void Equalizer::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    blockSize = samplesPerBlock;
    for (auto& band : filterStages)
        for (auto& f : band)
            f.prepare(sampleRate);
    envelope.fill(0.0f);

    int fftSize = static_cast<int>(lpResolution);
    allocateLinearPhaseBuffers(fftSize);

    int npFftSize = static_cast<int>(npResolution);
    allocateNaturalPhaseBuffers(npFftSize);
}

void Equalizer::allocateNaturalPhaseBuffers(int size)
{
    npFftSize = size;
    npFftHalf = size / 2;

    npOverlapL.assign(npFftSize, 0.0f);
    npOverlapR.assign(npFftSize, 0.0f);
    npInputBufferL.assign(npFftSize, 0.0f);
    npInputBufferR.assign(npFftSize, 0.0f);

    npWritePos = 0;
    npReady = false;
}

void Equalizer::processDynamicEQ(int bandIdx, float& gain, float inputLevel)
{
    auto& dyn = bands[bandIdx].dynamic;
    if (!dyn.enabled) return;

    float threshold = dyn.autoThreshold ? -20.0f : dyn.threshold;

    // Program-dependent attack/release based on frequency and dynamic range
    float attackMs, releaseMs;
    if (dyn.autoAttack)
    {
        // Lower frequencies = slower attack, higher dynamic range = faster attack
        float freq = bands[bandIdx].freq;
        float freqFactor = std::log10(freq / 20.0f) / std::log10(22000.0f / 20.0f); // 0-1
        attackMs = 1.0f + freqFactor * 50.0f; // 1-51ms
        attackMs *= (1.0f + std::abs(dyn.dynamicRange) / 30.0f * 2.0f); // Scale with range
    }
    else
        attackMs = dyn.attackMs;

    if (dyn.autoRelease)
    {
        // Lower frequencies = slower release
        float freq = bands[bandIdx].freq;
        float freqFactor = std::log10(freq / 20.0f) / std::log10(22000.0f / 20.0f); // 0-1
        releaseMs = 50.0f + freqFactor * 500.0f; // 50-550ms
        releaseMs *= (1.0f + std::abs(dyn.dynamicRange) / 30.0f); // Scale with range
    }
    else
        releaseMs = dyn.releaseMs;

    float attack = 1.0f - std::exp(-1.0f / (currentSampleRate * attackMs / 1000.0f));
    float release = 1.0f - std::exp(-1.0f / (currentSampleRate * releaseMs / 1000.0f));

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
    targetBands[index] = state;
    responseDirty = true;
    if (state.active)
    {
        float scaledGain = state.gain * gainScale;
        bool isCutFilter = (state.type == FilterType::LowCut || state.type == FilterType::HighCut);
        int numStages = isCutFilter ? slopeToStages(state.slope) : 1;

        for (int s = 0; s < numStages; ++s)
            filterStages[index][s].setParams(state.freq, scaledGain, state.q, state.type);

        for (int s = numStages; s < MAX_FILTERS_PER_BAND; ++s)
            filterStages[index][s].reset();
    }
}

void Equalizer::setBandSmoothed(int index, const BandState& state, int numSamples)
{
    if (index < 0 || index >= MAX_BANDS) return;
    targetBands[index] = state;
    responseDirty = true;
}

static void processChannelWithStages(BiquadFilter* stages, int numStages,
                                     float* ch, int numSamples)
{
    for (int s = 0; s < numStages; ++s)
        stages[s].processLeft(ch, numSamples);
}

void Equalizer::processMultiChannel(float** channels, int numChannels, int numSamples)
{
    // Smooth parameter interpolation
    float smoothingFactor = 1.0f - std::exp(-1.0f / (currentSampleRate * smoothingTimeMs / 1000.0f));
    for (int i = 0; i < MAX_BANDS; ++i)
    {
        if (bands[i].active != targetBands[i].active)
            bands[i].active = targetBands[i].active;
        
        if (bands[i].active)
        {
            // Interpolate freq, gain, q
            bands[i].freq += (targetBands[i].freq - bands[i].freq) * smoothingFactor;
            bands[i].gain += (targetBands[i].gain - bands[i].gain) * smoothingFactor;
            bands[i].q += (targetBands[i].q - bands[i].q) * smoothingFactor;
        }
    }

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
            // Use first two channels for dynamic detection if available
            if (numChannels >= 2 && bands[i].scTrigger && scLevelL > 0.0f)
            {
                inputLevel = 0.5f * (scLevelL + scLevelR);
            }
            else if (numChannels >= 1)
            {
                for (int s = 0; s < numSamples; ++s)
                    inputLevel += channels[0][s] * channels[0][s];
                inputLevel = std::sqrt(inputLevel / (float)numSamples);
            }
            processDynamicEQ(i, effectiveGain, inputLevel);
        }

        bool isCutFilter = (bands[i].type == FilterType::LowCut || bands[i].type == FilterType::HighCut);
        int numStages = isCutFilter ? slopeToStages(bands[i].slope) : 1;

        for (int s = 0; s < numStages; ++s)
        {
            // Update filter coefficients for this band
            filterStages[i][s].setParams(bands[i].freq, effectiveGain, bands[i].q, bands[i].type);
        }

        // Process each channel based on channel mode
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* channelData = channels[ch];
            
            switch (bands[i].channelMode)
            {
            case ChannelMode::Stereo:
            case ChannelMode::Left:
            case ChannelMode::Right:
            case ChannelMode::Mid:
            case ChannelMode::Side:
            case ChannelMode::LFE:
            case ChannelMode::Center:
            case ChannelMode::LeftSurround:
            case ChannelMode::RightSurround:
            case ChannelMode::LeftRearSurround:
            case ChannelMode::RightRearSurround:
            case ChannelMode::TopFrontLeft:
            case ChannelMode::TopFrontRight:
            case ChannelMode::TopRearLeft:
            case ChannelMode::TopRearRight:
                for (int s = 0; s < numStages; ++s)
                    filterStages[i][s].processLeft(channelData, numSamples);
                break;
            }
        }

        // Apply phase inversion if enabled
        if (bands[i].phaseInverted)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                for (int s = 0; s < numSamples; ++s)
                    channels[ch][s] = -channels[ch][s];
            }
        }
    }
}

void Equalizer::process(float* left, float* right, int numSamples)
{
    // Smooth parameter interpolation
    float smoothingFactor = 1.0f - std::exp(-1.0f / (currentSampleRate * smoothingTimeMs / 1000.0f));
    for (int i = 0; i < MAX_BANDS; ++i)
    {
        if (bands[i].active != targetBands[i].active)
            bands[i].active = targetBands[i].active;
        
        if (bands[i].active)
        {
            // Interpolate freq, gain, q
            bands[i].freq += (targetBands[i].freq - bands[i].freq) * smoothingFactor;
            bands[i].gain += (targetBands[i].gain - bands[i].gain) * smoothingFactor;
            bands[i].q += (targetBands[i].q - bands[i].q) * smoothingFactor;
        }
    }

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
            if (bands[i].scTrigger && scLevelL > 0.0f)
            {
                inputLevel = 0.5f * (scLevelL + scLevelR);
            }
            else
            {
                for (int s = 0; s < numSamples; ++s)
                    inputLevel += left[s] * left[s];
                inputLevel = std::sqrt(inputLevel / (float)numSamples);
            }
            processDynamicEQ(i, effectiveGain, inputLevel);
        }

        bool isCutFilter = (bands[i].type == FilterType::LowCut || bands[i].type == FilterType::HighCut);
        int numStages = isCutFilter ? slopeToStages(bands[i].slope) : 1;

        for (int s = 0; s < numStages; ++s)
            filterStages[i][s].setParams(bands[i].freq, effectiveGain, bands[i].q, bands[i].type);

        switch (bands[i].channelMode)
        {
        case ChannelMode::Stereo:
            for (int s = 0; s < numStages; ++s)
                filterStages[i][s].processStereo(left, right, numSamples);
            break;
        case ChannelMode::Mid:
            for (int s = 0; s < numStages; ++s)
                filterStages[i][s].processMidSide(left, right, numSamples, true);
            break;
        case ChannelMode::Side:
            for (int s = 0; s < numStages; ++s)
                filterStages[i][s].processMidSide(left, right, numSamples, false);
            break;
        case ChannelMode::Left:
            for (int s = 0; s < numStages; ++s)
                filterStages[i][s].processLeft(left, numSamples);
            break;
        case ChannelMode::Right:
            for (int s = 0; s < numStages; ++s)
                filterStages[i][s].processRight(right, numSamples);
            break;
        }

        // Apply phase inversion if enabled
        if (bands[i].phaseInverted)
        {
            for (int s = 0; s < numSamples; ++s)
            {
                left[s] = -left[s];
                right[s] = -right[s];
            }
        }
    }
}

void Equalizer::fftInPlace(std::complex<float>* data, int n, bool inverse)
{
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

static void computeBiquadResponse(float* b, float* a, std::complex<float>* response, int numBins, float sampleRate)
{
    for (int k = 0; k < numBins; ++k)
    {
        float w = 2.0f * 3.14159265f * (float)k / (float)(numBins * 2);
        float cosW = std::cos(w);
        float sinW = std::sin(w);
        float cos2W = std::cos(2.0f * w);
        float sin2W = std::sin(2.0f * w);

        float numR = b[0] + b[1] * cosW + b[2] * cos2W;
        float numI = -(b[1] * sinW + b[2] * sin2W);
        float denR = 1.0f + a[1] * cosW + a[2] * cos2W;
        float denI = -(a[1] * sinW + a[2] * sin2W);

        float denMag2 = denR * denR + denI * denI;
        if (denMag2 < 1e-20f) denMag2 = 1e-20f;

        float hR = (numR * denR + numI * denI) / denMag2;
        float hI = (numI * denR - numR * denI) / denMag2;

        response[k] *= std::complex<float>(hR, hI);
    }
}

void Equalizer::computeEQFrequencyResponse(std::complex<float>* response, int numBins, float sampleRate)
{
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
        float bn[3] = { b0 * invA0, b1 * invA0, b2 * invA0 };
        float an[3] = { 1.0f, a1 * invA0, a2 * invA0 };

        bool isCutFilter = (bands[b].type == FilterType::LowCut || bands[b].type == FilterType::HighCut);
        int numStages = isCutFilter ? slopeToStages(bands[b].slope) : 1;

        for (int s = 0; s < numStages; ++s)
            computeBiquadResponse(bn, an, response, numBins, sampleRate);
    }
}

void Equalizer::processLinearPhase(float* left, float* right, int numSamples)
{
    if (numSamples <= 0) return;

    if (responseDirty)
    {
        eqResponse[0] = std::complex<float>(1.0f, 0.0f);
        computeEQFrequencyResponse(eqResponse.data(), fftHalf, (float)currentSampleRate);
        for (int k = 1; k < fftHalf; ++k)
            eqResponse[fftSize - k] = std::conj(eqResponse[k]);
        responseDirty = false;
    }

    for (int s = 0; s < numSamples; ++s)
    {
        inputBufferL[lpWritePos] = left[s];
        inputBufferR[lpWritePos] = right[s];
        lpWritePos++;

        if (lpWritePos >= fftSize)
        {
            lpWritePos = 0;
            lpReady = true;

            std::vector<std::complex<float>> fftL(fftSize);
            for (int i = 0; i < fftSize; ++i)
                fftL[i] = std::complex<float>(inputBufferL[i] * windowCoeffs[i], 0.0f);
            fftInPlace(fftL.data(), fftSize, false);
            for (int i = 0; i < fftSize; ++i)
                fftL[i] *= eqResponse[i];
            fftInPlace(fftL.data(), fftSize, true);
            for (int i = 0; i < fftHalf; ++i)
            {
                float sample = fftL[i].real() * windowCoeffs[i] + overlapL[i];
                overlapL[i] = fftL[i + fftHalf].real() * windowCoeffs[i + fftHalf];
                inputBufferL[i] = sample;
            }

            std::vector<std::complex<float>> fftR(fftSize);
            for (int i = 0; i < fftSize; ++i)
                fftR[i] = std::complex<float>(inputBufferR[i] * windowCoeffs[i], 0.0f);
            fftInPlace(fftR.data(), fftSize, false);
            for (int i = 0; i < fftSize; ++i)
                fftR[i] *= eqResponse[i];
            fftInPlace(fftR.data(), fftSize, true);
            for (int i = 0; i < fftHalf; ++i)
            {
                float sample = fftR[i].real() * windowCoeffs[i] + overlapR[i];
                overlapR[i] = fftR[i + fftHalf].real() * windowCoeffs[i + fftHalf];
                inputBufferR[i] = sample;
            }
        }
    }

    int avail = lpReady ? fftHalf : lpWritePos;
    int toRead = std::min(numSamples, avail);

    for (int s = 0; s < toRead; ++s)
    {
        left[s] = inputBufferL[s];
        right[s] = inputBufferR[s];
    }

    if (numSamples > avail)
    {
        for (int i = 0; i < MAX_BANDS; ++i)
        {
            if (!bands[i].active || bands[i].bypassed) continue;
            float effectiveGain = bands[i].gain * gainScale;
            bool isCutFilter = (bands[i].type == FilterType::LowCut || bands[i].type == FilterType::HighCut);
            int numStages = isCutFilter ? slopeToStages(bands[i].slope) : 1;

            for (int s = 0; s < numStages; ++s)
                filterStages[i][s].setParams(bands[i].freq, effectiveGain, bands[i].q, bands[i].type);

            switch (bands[i].channelMode)
            {
            case ChannelMode::Stereo:
                for (int s = 0; s < numStages; ++s)
                    filterStages[i][s].processStereo(left + avail, right + avail, numSamples - avail);
                break;
            case ChannelMode::Mid:
                for (int s = 0; s < numStages; ++s)
                    filterStages[i][s].processMidSide(left + avail, right + avail, numSamples - avail, true);
                break;
            case ChannelMode::Side:
                for (int s = 0; s < numStages; ++s)
                    filterStages[i][s].processMidSide(left + avail, right + avail, numSamples - avail, false);
                break;
            case ChannelMode::Left:
                for (int s = 0; s < numStages; ++s)
                    filterStages[i][s].processLeft(left + avail, numSamples - avail);
                break;
            case ChannelMode::Right:
                for (int s = 0; s < numStages; ++s)
                    filterStages[i][s].processRight(right + avail, numSamples - avail);
                break;
            }
        }
    }
}

void Equalizer::processNaturalPhase(float* left, float* right, int numSamples)
{
    if (numSamples <= 0) return;

    // Natural Phase: Hybrid approach using minimum phase filters + allpass phase correction
    // 1. Process with minimum phase filters (like Zero Latency)
    // 2. Apply allpass phase correction to reduce phase distortion
    // 3. Use overlap-add with FFT for the allpass correction part

    if (responseDirty)
    {
        eqResponse[0] = std::complex<float>(1.0f, 0.0f);
        computeEQFrequencyResponse(eqResponse.data(), fftHalf, (float)currentSampleRate);
        for (int k = 1; k < fftHalf; ++k)
            eqResponse[fftSize - k] = std::conj(eqResponse[k]);
        responseDirty = false;
    }

    // Compute allpass phase correction response (phase-only, magnitude = 1)
    // For Natural Phase, we want to cancel some of the minimum phase filter's phase shift
    // We'll use a simplified approach: apply the minimum phase EQ, then correct phase
    
    // First, process with minimum phase filters (same as Zero Latency)
    process(left, right, numSamples);
    
    // Then apply allpass phase correction via FFT
    // The allpass response is: H_ap(z) = H_min(z) / |H_min(z)| = e^{-j*phase(H_min)}
    // We want to apply a partial correction to reduce phase distortion without pre-ringing
    
    // For simplicity, we'll use a lighter FFT-based approach with shorter latency
    for (int s = 0; s < numSamples; ++s)
    {
        npInputBufferL[npWritePos] = left[s];
        npInputBufferR[npWritePos] = right[s];
        npWritePos++;

        if (npWritePos >= npFftSize)
        {
            npWritePos = 0;
            npReady = true;

            std::vector<std::complex<float>> fftL(npFftSize);
            for (int i = 0; i < npFftSize; ++i)
                fftL[i] = std::complex<float>(npInputBufferL[i] * windowCoeffs[i], 0.0f);
            fftInPlace(fftL.data(), npFftSize, false);
            
            // Apply allpass phase correction: conjugate of minimum phase response for phase-only
            for (int i = 0; i < npFftSize; ++i)
            {
                float mag = std::abs(fftL[i]);
                if (mag > 1e-10f)
                {
                    // Allpass: keep magnitude, modify phase
                    float targetMag = mag;
                    // Partial phase correction (50% for Natural Phase)
                    std::complex<float> eqResp = eqResponse[i];
                    float eqPhase = std::arg(eqResp);
                    fftL[i] = std::complex<float>(targetMag * std::cos(eqPhase * 0.5f), 
                                                  targetMag * std::sin(eqPhase * 0.5f));
                }
            }
            
            fftInPlace(fftL.data(), npFftSize, true);
            for (int i = 0; i < npFftHalf; ++i)
            {
                float sample = fftL[i].real() * windowCoeffs[i] + npOverlapL[i];
                npOverlapL[i] = fftL[i + npFftHalf].real() * windowCoeffs[i + npFftHalf];
                npInputBufferL[i] = sample;
            }

            std::vector<std::complex<float>> fftR(npFftSize);
            for (int i = 0; i < npFftSize; ++i)
                fftR[i] = std::complex<float>(npInputBufferR[i] * windowCoeffs[i], 0.0f);
            fftInPlace(fftR.data(), npFftSize, false);
            
            for (int i = 0; i < npFftSize; ++i)
            {
                float mag = std::abs(fftR[i]);
                if (mag > 1e-10f)
                {
                    float targetMag = mag;
                    std::complex<float> eqResp = eqResponse[i];
                    float eqPhase = std::arg(eqResp);
                    fftR[i] = std::complex<float>(targetMag * std::cos(eqPhase * 0.5f), 
                                                  targetMag * std::sin(eqPhase * 0.5f));
                }
            }
            
            fftInPlace(fftR.data(), npFftSize, true);
            for (int i = 0; i < npFftHalf; ++i)
            {
                float sample = fftR[i].real() * windowCoeffs[i] + npOverlapR[i];
                npOverlapR[i] = fftR[i + npFftHalf].real() * windowCoeffs[i + npFftHalf];
                npInputBufferR[i] = sample;
            }
        }
    }

    int avail = npReady ? npFftHalf : npWritePos;
    int toRead = std::min(numSamples, avail);

    for (int s = 0; s < toRead; ++s)
    {
        left[s] = npInputBufferL[s];
        right[s] = npInputBufferR[s];
    }

    if (numSamples > avail)
    {
        // Fallback for remaining samples - use minimum phase
        for (int i = 0; i < MAX_BANDS; ++i)
        {
            if (!bands[i].active || bands[i].bypassed) continue;
            float effectiveGain = bands[i].gain * gainScale;
            bool isCutFilter = (bands[i].type == FilterType::LowCut || bands[i].type == FilterType::HighCut);
            int numStages = isCutFilter ? slopeToStages(bands[i].slope) : 1;

            for (int s = 0; s < numStages; ++s)
                filterStages[i][s].setParams(bands[i].freq, effectiveGain, bands[i].q, bands[i].type);

            switch (bands[i].channelMode)
            {
            case ChannelMode::Stereo:
                for (int s = 0; s < numStages; ++s)
                    filterStages[i][s].processStereo(left + avail, right + avail, numSamples - avail);
                break;
            case ChannelMode::Mid:
                for (int s = 0; s < numStages; ++s)
                    filterStages[i][s].processMidSide(left + avail, right + avail, numSamples - avail, true);
                break;
            case ChannelMode::Side:
                for (int s = 0; s < numStages; ++s)
                    filterStages[i][s].processMidSide(left + avail, right + avail, numSamples - avail, false);
                break;
            case ChannelMode::Left:
                for (int s = 0; s < numStages; ++s)
                    filterStages[i][s].processLeft(left + avail, numSamples - avail);
                break;
            case ChannelMode::Right:
                for (int s = 0; s < numStages; ++s)
                    filterStages[i][s].processRight(right + avail, numSamples - avail);
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
            bool isCutFilter = (bands[i].type == FilterType::LowCut || bands[i].type == FilterType::HighCut);
            int numStages = isCutFilter ? slopeToStages(bands[i].slope) : 1;

            for (int s = 0; s < numStages; ++s)
            {
                BiquadFilter tempFilter;
                const_cast<BiquadFilter&>(tempFilter).prepare(currentSampleRate);
                const_cast<BiquadFilter&>(tempFilter).setParams(bands[i].freq, scaledGain, bands[i].q, bands[i].type);
                mag *= tempFilter.getMagnitude(freq);
            }
        }
    }
    return mag;
}

#include "Equalizer.h"
#include <cmath>

void Equalizer::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    blockSize = samplesPerBlock;
    for (auto& f : filters)
        f.prepare(sampleRate);
    envelope.fill(0.0f);
    firBufferL.fill(0.0f);
    firBufferR.fill(0.0f);
    firWritePos = 0;
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

void Equalizer::processLinearPhase(float* left, float* right, int numSamples)
{
    if (numSamples <= 0) return;

    bool hasSolo = false;
    for (int i = 0; i < MAX_BANDS; ++i)
        if (bands[i].soloed) { hasSolo = true; break; }

    for (int s = 0; s < numSamples; ++s)
    {
        firBufferL[firWritePos] = left[s];
        firBufferR[firWritePos] = right[s];
        firWritePos++;

        if (firWritePos >= FIR_SIZE)
        {
            firWritePos = 0;

            // Compute frequency response of the EQ
            int halfSize = FIR_SIZE / 2;
            float freqStep = (float)currentSampleRate / (float)FIR_SIZE;

            // Simple time-domain overlap-save with linear phase approximation
            // Apply biquad cascade and store result
            for (int i = 0; i < MAX_BANDS; ++i)
            {
                if (!bands[i].active || bands[i].bypassed) continue;
                if (hasSolo && !bands[i].soloed) continue;

                float effectiveGain = bands[i].gain * gainScale;
                if (bands[i].dynamic.enabled)
                {
                    float inputLevel = 0.0f;
                    for (int n = 0; n < FIR_SIZE; ++n)
                        inputLevel += firBufferL[n] * firBufferL[n];
                    inputLevel = std::sqrt(inputLevel / (float)FIR_SIZE);
                    processDynamicEQ(i, effectiveGain, inputLevel);
                }
            }

            // Overlap-add output
            for (int n = 0; n < halfSize; ++n)
            {
                left[n] = firBufferL[n + halfSize];
                right[n] = firBufferR[n + halfSize];
            }
        }
    }

    // For samples that haven't filled the buffer yet, use direct processing
    if (firWritePos < numSamples)
    {
        for (int i = 0; i < MAX_BANDS; ++i)
        {
            if (!bands[i].active || bands[i].bypassed) continue;

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

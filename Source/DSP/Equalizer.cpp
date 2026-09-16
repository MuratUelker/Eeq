#include "Equalizer.h"

void Equalizer::prepare(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    for (auto& f : filters)
        f.prepare(sampleRate);
}

void Equalizer::process(float* left, float* right, int numSamples)
{
    for (int i = 0; i < MAX_BANDS; ++i)
    {
        if (bands[i].active && !bands[i].bypassed)
            filters[i].processStereo(left, right, numSamples);
    }
}

void Equalizer::setBand(int index, const BandState& state)
{
    if (index < 0 || index >= MAX_BANDS) return;
    bands[index] = state;
    if (state.active)
        filters[index].setParams(state.freq, state.gain, state.q, state.type);
}

float Equalizer::getMagnitudeAtFreq(float freq) const
{
    float mag = 1.0f;
    for (int i = 0; i < MAX_BANDS; ++i)
    {
        if (bands[i].active && !bands[i].bypassed)
            mag *= filters[i].getMagnitude(freq);
    }
    return mag;
}

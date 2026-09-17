#include "SpectrumAnalyzer.h"
#include <cmath>
#include <algorithm>

void SpectrumAnalyzer::prepare(double sampleRate)
{
    fs = sampleRate;
    fftBuffer.fill(0.0f);
    spectrum.fill(0.0f);
    peakHold.fill(0.0f);
    peakDecay.fill(0.0f);
    captureSpectrum.fill(0.0f);
    writePos = 0;
    captureCount = 0;
}

void SpectrumAnalyzer::setResolution(AnalyzerResolution res)
{
    resolution = res;
    fftSize = (int)res;
    numBins = fftSize / 2;
    fftBuffer.fill(0.0f);
    spectrum.fill(0.0f);
    writePos = 0;
}

void SpectrumAnalyzer::pushSamples(const float* data, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        fftBuffer[writePos] = data[i];
        writePos++;

        if (writePos >= fftSize)
        {
            writePos = 0;
            processFFT();
        }
    }
}

void SpectrumAnalyzer::processFFT()
{
    // Hann window
    for (int i = 0; i < fftSize; ++i)
        fftBuffer[i] *= 0.5f * (1.0f - std::cos(2.0f * 3.14159265f * i / (fftSize - 1)));

    std::array<float, MAX_FFT_SIZE> real{}, imag{};
    for (int i = 0; i < fftSize; ++i)
        real[i] = fftBuffer[i];
    std::fill(imag.begin(), imag.end(), 0.0f);

    fftRadix2(real.data(), imag.data(), fftSize);

    float nyquist = (float)fs * 0.5f;
    for (int i = 1; i < numBins && i < MAX_BINS; ++i)
    {
        float mag = std::sqrt(real[i] * real[i] + imag[i] * imag[i]) / (float)fftSize;
        float db = 20.0f * std::log10f(std::max(mag, 1e-10f));

        // Apply tilt (pink noise normalization)
        float freq = (float)i / (float)numBins * nyquist;
        if (freq > 1.0f)
            db += tiltDB * std::log10(freq / 1000.0f);

        float norm = (db + dbRange) / dbRange;
        if (norm < 0.0f) norm = 0.0f;
        if (norm > 1.0f) norm = 1.0f;

        if (!frozen)
        {
            spectrum[i] = spectrum[i] * decayRate + norm * (1.0f - decayRate);
        }

        // Peak hold
        if (spectrum[i] > peakHold[i])
        {
            peakHold[i] = spectrum[i];
            peakDecay[i] = 0.0f;
        }
        else
        {
            peakDecay[i] += 0.002f;
            peakHold[i] -= peakDecay[i];
            if (peakHold[i] < 0.0f) peakHold[i] = 0.0f;
        }

        // EQ Match capture
        if (captureActive)
        {
            if (captureCount == 0)
                captureSpectrum[i] = norm;
            else
                captureSpectrum[i] = captureSpectrum[i] * ((float)captureCount / (float)(captureCount + 1))
                                   + norm * (1.0f / (float)(captureCount + 1));
        }
    }
    if (captureActive) captureCount++;
}

void SpectrumAnalyzer::fftRadix2(float* real, float* imag, int n)
{
    int bits = 0;
    for (int temp = n; temp > 1; temp >>= 1) ++bits;

    for (int i = 0; i < n; ++i)
    {
        int j = 0;
        for (int b = 0; b < bits; ++b)
            j = (j << 1) | ((i >> b) & 1);
        if (i < j)
        {
            std::swap(real[i], real[j]);
            std::swap(imag[i], imag[j]);
        }
    }

    for (int size = 2; size <= n; size *= 2)
    {
        int halfSize = size / 2;
        float angle = -2.0f * 3.14159265f / (float)size;
        float wR = std::cos(angle), wI = std::sin(angle);

        for (int start = 0; start < n; start += size)
        {
            float curR = 1.0f, curI = 0.0f;
            for (int k = 0; k < halfSize; ++k)
            {
                int even = start + k;
                int odd = start + k + halfSize;
                float tR = curR * real[odd] - curI * imag[odd];
                float tI = curR * imag[odd] + curI * real[odd];
                real[odd] = real[even] - tR;
                imag[odd] = imag[even] - tI;
                real[even] += tR;
                imag[even] += tI;
                float newR = curR * wR - curI * wI;
                curI = curR * wI + curI * wR;
                curR = newR;
            }
        }
    }
}

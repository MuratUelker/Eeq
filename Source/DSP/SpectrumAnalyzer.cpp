#include "SpectrumAnalyzer.h"
#include <cmath>
#include <algorithm>

void SpectrumAnalyzer::prepare(double sampleRate)
{
    fs = sampleRate;
    fftBuffer.fill(0.0f);
    spectrum.fill(0.0f);
    writePos = 0;
    for (auto& h : history)
        h.fill(0.0f);
}

void SpectrumAnalyzer::pushSamples(const float* data, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        fftBuffer[writePos] = data[i];
        fftBuffer[writePos + FFT_SIZE] = 0.0f;
        writePos++;

        if (writePos >= FFT_SIZE)
        {
            writePos = 0;
            processFFT();
        }
    }
}

void SpectrumAnalyzer::processFFT()
{
    // Hann window
    for (int i = 0; i < FFT_SIZE; ++i)
    {
        float w = 0.5f * (1.0f - std::cos(2.0f * 3.14159265f * i / (FFT_SIZE - 1)));
        fftBuffer[i] *= w;
    }

    // In-place radix-2 FFT
    std::array<float, FFT_SIZE> real{}, imag{};
    for (int i = 0; i < FFT_SIZE; ++i)
        real[i] = fftBuffer[i];
    std::fill(imag.begin(), imag.end(), 0.0f);

    fftRadix2(real.data(), imag.data(), FFT_SIZE);

    // Magnitude spectrum in dB, normalized
    for (int i = 0; i < NUM_BINS; ++i)
    {
        float mag = std::sqrt(real[i] * real[i] + imag[i] * imag[i]) / (float)FFT_SIZE;
        float db = 20.0f * std::log10f(std::max(mag, 1e-10f));
        float norm = (db + 90.0f) / 90.0f;
        if (norm < 0.0f) norm = 0.0f;
        if (norm > 1.0f) norm = 1.0f;
        spectrum[i] = spectrum[i] * decayRate + norm * (1.0f - decayRate);
    }

    // Store in history for peak hold
    history[historyPos] = spectrum;
    historyPos = (historyPos + 1) % HIST_SIZE;
}

void SpectrumAnalyzer::fftRadix2(float* real, float* imag, int n)
{
    // Bit reversal
    int bits = 0;
    for (int temp = n; temp > 1; temp >>= 1) ++bits;

    for (int i = 0; i < n; ++i)
    {
        int j = 0;
        for (int b = 0; b < bits; ++b)
        {
            j = (j << 1) | ((i >> b) & 1);
        }
        if (i < j)
        {
            std::swap(real[i], real[j]);
            std::swap(imag[i], imag[j]);
        }
    }

    // FFT butterfly
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

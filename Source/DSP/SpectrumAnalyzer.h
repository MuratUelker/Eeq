#pragma once
#include <array>
#include <cmath>
#include <cstring>

static constexpr int FFT_SIZE = 4096;
static constexpr int NUM_BINS = FFT_SIZE / 2;

class SpectrumAnalyzer
{
public:
    SpectrumAnalyzer() = default;

    void prepare(double sampleRate);
    void pushSamples(const float* data, int numSamples);
    const std::array<float, NUM_BINS>& getSpectrumData() const { return spectrum; }

    static constexpr int HIST_SIZE = 4;
    std::array<std::array<float, NUM_BINS>, HIST_SIZE> history{};
    int historyPos = 0;

private:
    double fs = 44100.0;
    std::array<float, FFT_SIZE * 2> fftBuffer{};
    std::array<float, NUM_BINS> spectrum{};
    int writePos = 0;
    float decayRate = 0.85f;

    void processFFT();
    void fftRadix2(float* real, float* imag, int n);
};

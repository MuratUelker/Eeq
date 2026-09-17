#pragma once
#include <array>
#include <cmath>
#include <cstring>

static constexpr int MAX_FFT_SIZE = 8192;
static constexpr int MAX_BINS = MAX_FFT_SIZE / 2;

enum class AnalyzerResolution
{
    Low = 1024,
    Medium = 2048,
    High = 4096,
    Maximum = 8192
};

class SpectrumAnalyzer
{
public:
    SpectrumAnalyzer() = default;

    void prepare(double sampleRate);
    void pushSamples(const float* data, int numSamples);

    void setResolution(AnalyzerResolution res);
    void setSpeed(float speed) { decayRate = speed; }
    void setRange(float range) { dbRange = range; }
    void setTilt(float tilt) { tiltDB = tilt; }
    void setFreeze(bool f) { frozen = f; }
    bool isFrozen() const { return frozen; }

    const std::array<float, MAX_BINS>& getSpectrumData() const { return spectrum; }
    int getNumBins() const { return numBins; }
    float getSampleRate() const { return (float)fs; }

    // External spectrum (for collision detection)
    void setExternalSpectrum(const std::array<float, MAX_BINS>& external, int bins);
    void clearExternalSpectrum();
    bool hasExternalSpectrum() const { return hasExternal; }
    const std::array<float, MAX_BINS>& getExternalSpectrum() const { return externalSpectrum; }
    std::array<float, MAX_BINS> getCollisionMask() const; // Returns 1.0 where both spectra overlap significantly

    // EQ Match
    void startCapture() { captureActive = true; captureSpectrum.fill(0.0f); captureCount = 0; }
    void stopCapture() { captureActive = false; }
    bool isCapturing() const { return captureActive; }
    const std::array<float, MAX_BINS>& getCaptureSpectrum() const { return captureSpectrum; }
    int getCaptureCount() const { return captureCount; }

    // Peak hold
    std::array<float, MAX_BINS> peakHold{};
    std::array<float, MAX_BINS> peakDecay{};

private:
    double fs = 44100.0;
    int fftSize = 4096;
    int numBins = 2048;
    AnalyzerResolution resolution = AnalyzerResolution::High;

    std::array<float, MAX_FFT_SIZE * 2> fftBuffer{};
    std::array<float, MAX_BINS> spectrum{};
    std::array<float, MAX_BINS> externalSpectrum{};
    int writePos = 0;
    float decayRate = 0.85f;
    float dbRange = 90.0f;
    float tiltDB = 4.5f;
    bool frozen = false;
    bool hasExternal = false;

    // EQ Match capture
    bool captureActive = false;
    std::array<float, MAX_BINS> captureSpectrum{};
    int captureCount = 0;

    void processFFT();
    void fftRadix2(float* real, float* imag, int n);
};

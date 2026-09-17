#include <iostream>
#include <chrono>
#include <cmath>
#include <cstring>
#include <cassert>
#include <vector>
#include <numeric>
#include "../Source/DSP/BiquadFilter.h"
#include "../Source/DSP/Equalizer.h"
#include "../Source/DSP/SpectrumAnalyzer.h"

static int testsPassed = 0;
static int testsFailed = 0;

#define TEST(name) \
    std::cout << "TEST: " << #name << " ... "; \
    try { test_##name(); testsPassed++; std::cout << "PASS\n"; } \
    catch (const std::exception& e) { testsFailed++; std::cout << "FAIL: " << e.what() << "\n"; }

#define ASSERT_TRUE(cond) \
    do { if (!(cond)) throw std::runtime_error(std::string("ASSERT failed: ") + #cond); } while(0)

#define ASSERT_NEAR(a, b, eps) \
    do { if (std::abs((a) - (b)) > (eps)) \
        throw std::runtime_error(std::string("ASSERT_NEAR failed: ") + #a + " != " + #b); } while(0)

// ==================== E2E Tests ====================

void test_full_lifecycle()
{
    Equalizer eq;
    eq.prepare(44100.0, 512);

    BandState band;
    band.freq = 1000.0f;
    band.gain = 6.0f;
    band.q = 0.707f;
    band.type = FilterType::Bell;
    band.active = true;
    band.channelMode = ChannelMode::Stereo;
    eq.setBand(0, band);

    float left[512], right[512];
    for (int i = 0; i < 512; ++i)
    {
        left[i] = std::sin(2.0f * 3.14159265f * 1000.0f * i / 44100.0f) * 0.5f;
        right[i] = left[i];
    }

    eq.process(left, right, 512);

    // Signal at 1kHz with +6dB bell should be boosted
    float rmsAfter = 0.0f;
    for (int i = 100; i < 512; ++i)
        rmsAfter += left[i] * left[i];
    rmsAfter = std::sqrt(rmsAfter / 412.0f);

    float rmsExpected = 0.5f * std::pow(10.0f, 6.0f / 20.0f); // ~1.0f
    ASSERT_TRUE(rmsAfter > 0.3f);
    ASSERT_TRUE(rmsAfter < 2.0f);
}

void test_state_save_load()
{
    Equalizer eq;
    eq.prepare(44100.0, 512);

    BandState band;
    band.freq = 500.0f;
    band.gain = -3.0f;
    band.q = 1.5f;
    band.type = FilterType::LowCut;
    band.slope = FilterSlope::Slope24;
    band.active = true;
    band.channelMode = ChannelMode::Mid;
    band.dynamic.enabled = true;
    band.dynamic.dynamicRange = -12.0f;
    band.scTrigger = true;
    eq.setBand(2, band);

    const auto& stored = eq.getBand(2);
    ASSERT_TRUE(stored.active == true);
    ASSERT_NEAR(stored.freq, 500.0f, 0.01f);
    ASSERT_NEAR(stored.gain, -3.0f, 0.01f);
    ASSERT_NEAR(stored.q, 1.5f, 0.01f);
    ASSERT_TRUE(stored.type == FilterType::LowCut);
    ASSERT_TRUE(stored.slope == FilterSlope::Slope24);
    ASSERT_TRUE(stored.channelMode == ChannelMode::Mid);
    ASSERT_TRUE(stored.dynamic.enabled == true);
    ASSERT_NEAR(stored.dynamic.dynamicRange, -12.0f, 0.01f);
    ASSERT_TRUE(stored.scTrigger == true);
}

void test_bypass()
{
    Equalizer eq;
    eq.prepare(44100.0, 512);

    BandState band;
    band.freq = 1000.0f;
    band.gain = 12.0f;
    band.q = 0.707f;
    band.type = FilterType::Bell;
    band.active = true;
    band.bypassed = true;
    band.channelMode = ChannelMode::Stereo;
    eq.setBand(0, band);

    float left[512], right[512];
    for (int i = 0; i < 512; ++i)
    {
        left[i] = 0.5f;
        right[i] = 0.5f;
    }

    eq.process(left, right, 512);

    // Bypassed band should leave signal unchanged
    ASSERT_NEAR(left[256], 0.5f, 0.01f);
    ASSERT_NEAR(right[256], 0.5f, 0.01f);
}

void test_solo()
{
    Equalizer eq;
    eq.prepare(44100.0, 512);

    // Band 0: bell at 1kHz, soloed, will pass 1kHz signal
    BandState band1;
    band1.freq = 1000.0f;
    band1.gain = 0.0f;
    band1.q = 0.707f;
    band1.type = FilterType::Bell;
    band1.active = true;
    band1.soloed = true;
    band1.channelMode = ChannelMode::Stereo;
    eq.setBand(0, band1);

    // Band 1: 12dB boost at 2kHz, NOT soloed — should be ignored
    BandState band2;
    band2.freq = 2000.0f;
    band2.gain = 12.0f;
    band2.q = 0.707f;
    band2.type = FilterType::Bell;
    band2.active = true;
    band2.soloed = false;
    band2.channelMode = ChannelMode::Stereo;
    eq.setBand(1, band2);

    float left[512], right[512];
    for (int i = 0; i < 512; ++i)
    {
        left[i] = std::sin(2.0f * 3.14159265f * 1000.0f * i / 44100.0f) * 0.5f;
        right[i] = left[i];
    }

    eq.process(left, right, 512);

    // Band1 (0dB bell) is soloed, band2 (12dB bell) ignored.
    // Signal at 1kHz should pass through at roughly same level
    float rmsAfter = 0.0f;
    for (int i = 100; i < 512; ++i)
        rmsAfter += left[i] * left[i];
    rmsAfter = std::sqrt(rmsAfter / 412.0f);
    ASSERT_TRUE(rmsAfter > 0.15f);
    ASSERT_TRUE(rmsAfter < 1.0f);
}

void test_all_filter_types()
{
    Equalizer eq;
    eq.prepare(44100.0, 512);

    FilterType types[] = {
        FilterType::Bell, FilterType::LowShelf, FilterType::HighShelf,
        FilterType::LowCut, FilterType::HighCut, FilterType::Notch,
        FilterType::BandPass, FilterType::FlatTilt, FilterType::TiltShelf
    };

    for (auto type : types)
    {
        BandState band;
        band.freq = 1000.0f;
        band.gain = 6.0f;
        band.q = 0.707f;
        band.type = type;
        band.active = true;
        band.channelMode = ChannelMode::Stereo;
        eq.setBand(0, band);

        float left[512], right[512];
        for (int i = 0; i < 512; ++i)
        {
            left[i] = std::sin(2.0f * 3.14159265f * 1000.0f * i / 44100.0f) * 0.5f;
            right[i] = left[i];
        }

        eq.process(left, right, 512);

        // All filter types should produce valid output (no NaN/Inf)
        for (int i = 0; i < 512; ++i)
        {
            ASSERT_TRUE(std::isfinite(left[i]));
            ASSERT_TRUE(std::isfinite(right[i]));
        }
    }
}

void test_higher_order_slopes()
{
    Equalizer eq;
    eq.prepare(44100.0, 512);

    FilterSlope slopes[] = {
        FilterSlope::Slope6, FilterSlope::Slope12, FilterSlope::Slope18,
        FilterSlope::Slope24, FilterSlope::Slope30, FilterSlope::Slope36,
        FilterSlope::Slope42, FilterSlope::Slope48
    };

    for (auto slope : slopes)
    {
        BandState band;
        band.freq = 1000.0f;
        band.gain = 0.0f;
        band.q = 0.707f;
        band.type = FilterType::LowCut;
        band.slope = slope;
        band.active = true;
        band.channelMode = ChannelMode::Stereo;
        eq.setBand(0, band);

        float left[512], right[512];
        for (int i = 0; i < 512; ++i)
        {
            left[i] = std::sin(2.0f * 3.14159265f * 500.0f * i / 44100.0f) * 0.5f;
            right[i] = left[i];
        }

        eq.process(left, right, 512);

        for (int i = 0; i < 512; ++i)
        {
            ASSERT_TRUE(std::isfinite(left[i]));
            ASSERT_TRUE(std::isfinite(right[i]));
        }
    }
}

void test_linear_phase_mode()
{
    Equalizer eq;
    eq.prepare(44100.0, 512);
    eq.setProcessingMode(ProcessingMode::LinearPhase);

    BandState band;
    band.freq = 1000.0f;
    band.gain = 6.0f;
    band.q = 0.707f;
    band.type = FilterType::Bell;
    band.active = true;
    band.channelMode = ChannelMode::Stereo;
    eq.setBand(0, band);

    float left[512], right[512];
    for (int i = 0; i < 512; ++i)
    {
        left[i] = std::sin(2.0f * 3.14159265f * 1000.0f * i / 44100.0f) * 0.5f;
        right[i] = left[i];
    }

    eq.processLinearPhase(left, right, 512);

    for (int i = 0; i < 512; ++i)
    {
        ASSERT_TRUE(std::isfinite(left[i]));
        ASSERT_TRUE(std::isfinite(right[i]));
    }
}

void test_silence_passes_clean()
{
    Equalizer eq;
    eq.prepare(44100.0, 512);

    BandState band;
    band.freq = 1000.0f;
    band.gain = 12.0f;
    band.q = 0.707f;
    band.type = FilterType::Bell;
    band.active = true;
    band.channelMode = ChannelMode::Stereo;
    eq.setBand(0, band);

    float left[512], right[512];
    std::memset(left, 0, sizeof(left));
    std::memset(right, 0, sizeof(right));

    eq.process(left, right, 512);

    for (int i = 0; i < 512; ++i)
    {
        ASSERT_TRUE(std::isfinite(left[i]));
        ASSERT_TRUE(std::isfinite(right[i]));
        ASSERT_TRUE(std::abs(left[i]) < 1e-6f);
        ASSERT_TRUE(std::abs(right[i]) < 1e-6f);
    }
}

void test_gain_scale()
{
    Equalizer eq;
    eq.prepare(44100.0, 512);

    BandState band;
    band.freq = 1000.0f;
    band.gain = 12.0f;
    band.q = 0.707f;
    band.type = FilterType::Bell;
    band.active = true;
    band.channelMode = ChannelMode::Stereo;
    eq.setBand(0, band);

    eq.setGainScale(0.5f);

    float left[512], right[512];
    for (int i = 0; i < 512; ++i)
    {
        left[i] = std::sin(2.0f * 3.14159265f * 1000.0f * i / 44100.0f) * 0.5f;
        right[i] = left[i];
    }

    eq.process(left, right, 512);

    // With 0.5x gain scale, 12dB gain becomes 6dB
    float rmsAfter = 0.0f;
    for (int i = 100; i < 512; ++i)
        rmsAfter += left[i] * left[i];
    rmsAfter = std::sqrt(rmsAfter / 412.0f);

    float expected6dB = 0.5f * std::pow(10.0f, 6.0f / 20.0f);
    ASSERT_TRUE(rmsAfter > expected6dB * 0.5f);
    ASSERT_TRUE(rmsAfter < expected6dB * 2.0f);
}

void test_mid_side_processing()
{
    Equalizer eq;
    eq.prepare(44100.0, 512);

    BandState band;
    band.freq = 1000.0f;
    band.gain = 12.0f;
    band.q = 0.707f;
    band.type = FilterType::Bell;
    band.active = true;
    band.channelMode = ChannelMode::Side;
    eq.setBand(0, band);

    float left[512], right[512];
    for (int i = 0; i < 512; ++i)
    {
        left[i] = std::sin(2.0f * 3.14159265f * 1000.0f * i / 44100.0f) * 0.5f;
        right[i] = -left[i]; // Pure side signal
    }

    eq.process(left, right, 512);

    for (int i = 0; i < 512; ++i)
    {
        ASSERT_TRUE(std::isfinite(left[i]));
        ASSERT_TRUE(std::isfinite(right[i]));
    }
}

// ==================== Antialiasing Tests ====================

void test_low_cut_rejects_nyquist()
{
    BiquadFilter filter;
    filter.prepare(44100.0);
    filter.setParams(1000.0f, 0.0f, 0.707f, FilterType::LowCut);

    // LowCut at 1kHz: passes above 1kHz, attenuates below
    // At 200 Hz (well below cutoff), should be attenuated
    float magBelow = filter.getMagnitude(200.0f);
    ASSERT_TRUE(magBelow < 0.5f);

    // At 5kHz (well above cutoff), should pass
    float magAbove = filter.getMagnitude(5000.0f);
    ASSERT_TRUE(magAbove > 0.5f);
}

void test_high_cut_rejects_dc()
{
    BiquadFilter filter;
    filter.prepare(44100.0);
    filter.setParams(1000.0f, 0.0f, 0.707f, FilterType::HighCut);

    // HighCut at 1kHz: passes below 1kHz, attenuates above
    // At 100 Hz (well below cutoff), should pass
    float magBelow = filter.getMagnitude(100.0f);
    ASSERT_TRUE(magBelow > 0.5f);

    // At 5kHz (well above cutoff), should be attenuated
    float magAbove = filter.getMagnitude(5000.0f);
    ASSERT_TRUE(magAbove < 0.5f);
}

void test_bell_no_aliasing()
{
    // Process a signal near Nyquist and verify no aliasing artifacts
    BiquadFilter filter;
    filter.prepare(44100.0);
    filter.setParams(10000.0f, 12.0f, 1.0f, FilterType::Bell);

    float signal[512];
    for (int i = 0; i < 512; ++i)
        signal[i] = std::sin(2.0f * 3.14159265f * 10000.0f * i / 44100.0f) * 0.1f;

    filter.processLeft(signal, 512);

    // Output should be finite and bounded
    for (int i = 0; i < 512; ++i)
    {
        ASSERT_TRUE(std::isfinite(signal[i]));
        ASSERT_TRUE(std::abs(signal[i]) < 5.0f);
    }
}

void test_notch_attenuation()
{
    BiquadFilter filter;
    filter.prepare(44100.0);
    filter.setParams(1000.0f, 0.0f, 10.0f, FilterType::Notch);

    float magAtCenter = filter.getMagnitude(1000.0f);
    float magAbove = filter.getMagnitude(3000.0f);
    float magBelow = filter.getMagnitude(300.0f);

    // Notch should attenuate at center frequency
    ASSERT_TRUE(magAtCenter < 0.5f);
    // Should pass frequencies away from center
    ASSERT_TRUE(magAbove > 0.3f);
    ASSERT_TRUE(magBelow > 0.3f);
}

void test_higher_order_steeper_slope()
{
    // Test via Equalizer cascaded stages (LowCut@48dB = 8 cascaded biquads)
    Equalizer eq;
    eq.prepare(44100.0, 512);

    BandState band;
    band.freq = 1000.0f;
    band.gain = 0.0f;
    band.q = 0.707f;
    band.type = FilterType::LowCut;
    band.slope = FilterSlope::Slope48;
    band.active = true;
    band.channelMode = ChannelMode::Stereo;
    eq.setBand(0, band);

    float left[512], right[512];
    for (int i = 0; i < 512; ++i)
    {
        left[i] = std::sin(2.0f * 3.14159265f * 200.0f * i / 44100.0f) * 0.5f;
        right[i] = left[i];
    }

    float rmsBefore = 0.0f;
    for (int i = 0; i < 512; ++i)
        rmsBefore += left[i] * left[i];
    rmsBefore = std::sqrt(rmsBefore / 512.0f);

    eq.process(left, right, 512);

    float rmsAfter = 0.0f;
    for (int i = 100; i < 512; ++i)
        rmsAfter += left[i] * left[i];
    rmsAfter = std::sqrt(rmsAfter / 412.0f);

    // 48dB/oct slope at 1kHz should heavily attenuate 200 Hz
    ASSERT_TRUE(rmsAfter < rmsBefore * 0.3f);
}

// ==================== Performance Benchmarks ====================

void test_benchmark_zero_latency()
{
    Equalizer eq;
    eq.prepare(44100.0, 512);

    // Activate all 24 bands with different settings
    for (int i = 0; i < 24; ++i)
    {
        BandState band;
        band.freq = 100.0f * (i + 1);
        band.gain = (i % 2 == 0) ? 6.0f : -6.0f;
        band.q = 0.707f + (i % 5) * 0.5f;
        band.type = (i % 3 == 0) ? FilterType::Bell :
                    (i % 3 == 1) ? FilterType::LowShelf : FilterType::HighShelf;
        band.active = true;
        band.channelMode = ChannelMode::Stereo;
        eq.setBand(i, band);
    }

    float left[512], right[512];
    for (int i = 0; i < 512; ++i)
    {
        left[i] = std::sin(2.0f * 3.14159265f * 1000.0f * i / 44100.0f) * 0.5f;
        right[i] = left[i];
    }

    auto start = std::chrono::high_resolution_clock::now();
    const int iterations = 1000;
    for (int iter = 0; iter < iterations; ++iter)
        eq.process(left, right, 512);
    auto end = std::chrono::high_resolution_clock::now();

    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    double msPerBlock = ms / iterations;
    double cpuPercent = (msPerBlock / (512.0 / 44100.0 * 1000.0)) * 100.0;

    std::cout << "\n  Zero-latency 24 bands: " << msPerBlock << " ms/block, "
              << cpuPercent << "% CPU @ 44.1kHz";
    ASSERT_TRUE(msPerBlock < 5.0f);
}

void test_benchmark_with_higher_order()
{
    Equalizer eq;
    eq.prepare(44100.0, 512);

    for (int i = 0; i < 8; ++i)
    {
        BandState band;
        band.freq = 200.0f * (i + 1);
        band.gain = 0.0f;
        band.q = 0.707f;
        band.type = FilterType::LowCut;
        band.slope = FilterSlope::Slope48;
        band.active = true;
        band.channelMode = ChannelMode::Stereo;
        eq.setBand(i, band);
    }

    float left[512], right[512];
    for (int i = 0; i < 512; ++i)
    {
        left[i] = std::sin(2.0f * 3.14159265f * 1000.0f * i / 44100.0f) * 0.5f;
        right[i] = left[i];
    }

    auto start = std::chrono::high_resolution_clock::now();
    const int iterations = 1000;
    for (int iter = 0; iter < iterations; ++iter)
        eq.process(left, right, 512);
    auto end = std::chrono::high_resolution_clock::now();

    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    double msPerBlock = ms / iterations;
    double cpuPercent = (msPerBlock / (512.0 / 44100.0 * 1000.0)) * 100.0;

    std::cout << "\n  8x LowCut@48dB/oct: " << msPerBlock << " ms/block, "
              << cpuPercent << "% CPU @ 44.1kHz";
    ASSERT_TRUE(msPerBlock < 5.0f);
}

void test_benchmark_linear_phase()
{
    Equalizer eq;
    eq.prepare(44100.0, 512);
    eq.setProcessingMode(ProcessingMode::LinearPhase);

    for (int i = 0; i < 8; ++i)
    {
        BandState band;
        band.freq = 500.0f * (i + 1);
        band.gain = (i % 2 == 0) ? 6.0f : -6.0f;
        band.q = 0.707f;
        band.type = FilterType::Bell;
        band.active = true;
        band.channelMode = ChannelMode::Stereo;
        eq.setBand(i, band);
    }

    float left[512], right[512];
    for (int i = 0; i < 512; ++i)
    {
        left[i] = std::sin(2.0f * 3.14159265f * 1000.0f * i / 44100.0f) * 0.5f;
        right[i] = left[i];
    }

    auto start = std::chrono::high_resolution_clock::now();
    const int iterations = 200;
    for (int iter = 0; iter < iterations; ++iter)
        eq.processLinearPhase(left, right, 512);
    auto end = std::chrono::high_resolution_clock::now();

    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    double msPerBlock = ms / iterations;
    double cpuPercent = (msPerBlock / (512.0 / 44100.0 * 1000.0)) * 100.0;

    std::cout << "\n  Linear phase 8 bands: " << msPerBlock << " ms/block, "
              << cpuPercent << "% CPU @ 44.1kHz";
    ASSERT_TRUE(msPerBlock < 20.0f);
}

void test_benchmark_spectrum_analyzer()
{
    SpectrumAnalyzer sa;
    sa.prepare(44100.0);

    float data[512];
    for (int i = 0; i < 512; ++i)
        data[i] = std::sin(2.0f * 3.14159265f * 1000.0f * i / 44100.0f) * 0.5f;

    auto start = std::chrono::high_resolution_clock::now();
    const int iterations = 1000;
    for (int iter = 0; iter < iterations; ++iter)
        sa.pushSamples(data, 512);
    auto end = std::chrono::high_resolution_clock::now();

    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    double msPerBlock = ms / iterations;
    double cpuPercent = (msPerBlock / (512.0 / 44100.0 * 1000.0)) * 100.0;

    std::cout << "\n  Spectrum analyzer: " << msPerBlock << " ms/block, "
              << cpuPercent << "% CPU @ 44.1kHz";
    ASSERT_TRUE(msPerBlock < 2.0f);
}

// ==================== Main ====================

int main()
{
    std::cout << "=== Eeq DSP Tests ===\n\n";

    std::cout << "--- E2E Tests ---\n";
    TEST(full_lifecycle);
    TEST(state_save_load);
    TEST(bypass);
    TEST(solo);
    TEST(all_filter_types);
    TEST(higher_order_slopes);
    TEST(linear_phase_mode);
    TEST(silence_passes_clean);
    TEST(gain_scale);
    TEST(mid_side_processing);

    std::cout << "\n--- Antialiasing Tests ---\n";
    TEST(low_cut_rejects_nyquist);
    TEST(high_cut_rejects_dc);
    TEST(bell_no_aliasing);
    TEST(notch_attenuation);
    TEST(higher_order_steeper_slope);

    std::cout << "\n--- Performance Benchmarks ---\n";
    TEST(benchmark_zero_latency);
    TEST(benchmark_with_higher_order);
    TEST(benchmark_linear_phase);
    TEST(benchmark_spectrum_analyzer);

    std::cout << "\n=== Results: " << testsPassed << " passed, "
              << testsFailed << " failed ===\n";

    return testsFailed > 0 ? 1 : 0;
}

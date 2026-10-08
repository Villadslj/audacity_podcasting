/*
* Audacity: A Digital Audio Editor
*/
#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "internal/speechdetectioncore.h"

using namespace au::podcast;
using namespace au::podcast::core;

namespace {
constexpr double SAMPLE_RATE = 8000.0;
constexpr double FRAME_MS = 20.0;
constexpr size_t FRAME_SIZE = 160; // 20 ms at 8 kHz

//! Deterministic pseudo noise, so that the tests do not depend on a random generator
float noiseAt(size_t index, float amplitude)
{
    const double phase = std::sin(static_cast<double>(index) * 12.9898) * 43758.5453;
    return static_cast<float>((phase - std::floor(phase) - 0.5) * 2.0 * amplitude);
}

std::vector<float> makeNoise(double durationSecs, float amplitude)
{
    const size_t count = static_cast<size_t>(durationSecs * SAMPLE_RATE);
    std::vector<float> samples(count, 0.f);
    for (size_t i = 0; i < count; ++i) {
        samples[i] = noiseAt(i, amplitude);
    }
    return samples;
}

void addTone(std::vector<float>& samples, double fromSecs, double toSecs, float amplitude)
{
    const size_t from = static_cast<size_t>(fromSecs * SAMPLE_RATE);
    const size_t to = std::min(samples.size(), static_cast<size_t>(toSecs * SAMPLE_RATE));
    for (size_t i = from; i < to; ++i) {
        samples[i] += static_cast<float>(amplitude * std::sin(2.0 * M_PI * 220.0 * static_cast<double>(i) / SAMPLE_RATE));
    }
}

SpeechDetectionSettings makeSettings()
{
    SpeechDetectionSettings settings;
    settings.frameMs = FRAME_MS;
    settings.hangoverMs = 0.0;
    settings.minSpeechMs = 150.0;
    settings.minSilenceMs = 300.0;
    settings.marginMs = 50.0;
    settings.bleedRejection = false;
    return settings;
}

TimeSpanList detect(const std::vector<float>& samples, const SpeechDetectionSettings& settings,
                    const std::vector<bool>& allowedFrames = {})
{
    const std::vector<double> framesDb = frameRmsDb(samples.data(), samples.size(), FRAME_SIZE);
    const FrameRanges ranges = detectFrameRanges(framesDb, noiseFloorDb(framesDb), settings, allowedFrames);
    const double duration = static_cast<double>(samples.size()) / SAMPLE_RATE;
    return framesToSpans(ranges, settings.frameMs, 0.0, 0.0, duration, settings.marginMs);
}
}

TEST(SpeechDetectionCoreTests, FrameRmsDbOfSilenceAndFullScale)
{
    const std::vector<float> silence(FRAME_SIZE * 4, 0.f);
    const std::vector<double> silenceDb = frameRmsDb(silence.data(), silence.size(), FRAME_SIZE);

    ASSERT_EQ(silenceDb.size(), 4u);
    for (double value : silenceDb) {
        EXPECT_DOUBLE_EQ(value, SILENCE_DB);
    }

    const std::vector<float> fullScale(FRAME_SIZE * 2, 1.f);
    const std::vector<double> fullScaleDb = frameRmsDb(fullScale.data(), fullScale.size(), FRAME_SIZE);

    ASSERT_EQ(fullScaleDb.size(), 2u);
    for (double value : fullScaleDb) {
        EXPECT_NEAR(value, 0.0, 1e-9);
    }
}

TEST(SpeechDetectionCoreTests, FrameRmsDbIgnoresTrailingPartialFrame)
{
    const std::vector<float> samples(FRAME_SIZE * 3 + 7, 0.5f);
    EXPECT_EQ(frameRmsDb(samples.data(), samples.size(), FRAME_SIZE).size(), 3u);
}

TEST(SpeechDetectionCoreTests, NoiseFloorIsTheTenthPercentile)
{
    std::vector<double> framesDb;
    for (int i = 0; i < 100; ++i) {
        framesDb.push_back(static_cast<double>(i));
    }

    EXPECT_DOUBLE_EQ(noiseFloorDb(framesDb), 9.0);
    EXPECT_DOUBLE_EQ(noiseFloorDb({}), SILENCE_DB);
}

TEST(SpeechDetectionCoreTests, ToneBurstInNoiseIsDetected)
{
    std::vector<float> samples = makeNoise(4.0, 0.001f);
    addTone(samples, 1.0, 2.0, 0.2f);

    const SpeechDetectionSettings settings = makeSettings();
    const TimeSpanList spans = detect(samples, settings);

    ASSERT_EQ(spans.size(), 1u);

    const double tolerance = FRAME_MS / 1000.0;
    const double margin = settings.marginMs / 1000.0;
    EXPECT_NEAR(spans[0].start().to_double(), 1.0 - margin, tolerance);
    EXPECT_NEAR(spans[0].end().to_double(), 2.0 + margin, tolerance);
}

TEST(SpeechDetectionCoreTests, TwoToneBurstsAreDetectedSeparately)
{
    std::vector<float> samples = makeNoise(5.0, 0.001f);
    addTone(samples, 1.0, 2.0, 0.2f);
    addTone(samples, 3.0, 4.0, 0.2f);

    const SpeechDetectionSettings settings = makeSettings();
    const TimeSpanList spans = detect(samples, settings);

    ASSERT_EQ(spans.size(), 2u);

    const double tolerance = FRAME_MS / 1000.0;
    const double margin = settings.marginMs / 1000.0;
    EXPECT_NEAR(spans[0].start().to_double(), 1.0 - margin, tolerance);
    EXPECT_NEAR(spans[0].end().to_double(), 2.0 + margin, tolerance);
    EXPECT_NEAR(spans[1].start().to_double(), 3.0 - margin, tolerance);
    EXPECT_NEAR(spans[1].end().to_double(), 4.0 + margin, tolerance);
}

TEST(SpeechDetectionCoreTests, ShortBurstsAreDropped)
{
    std::vector<float> samples = makeNoise(3.0, 0.001f);
    // 60 ms, which is below the 150 ms minimum
    addTone(samples, 1.0, 1.06, 0.2f);

    EXPECT_TRUE(detect(samples, makeSettings()).empty());
}

TEST(SpeechDetectionCoreTests, HangoverKeepsTheRegionOpen)
{
    std::vector<float> samples = makeNoise(4.0, 0.001f);
    addTone(samples, 1.0, 2.0, 0.2f);

    SpeechDetectionSettings settings = makeSettings();
    settings.hangoverMs = 200.0;
    settings.marginMs = 0.0;

    const TimeSpanList spans = detect(samples, settings);

    ASSERT_EQ(spans.size(), 1u);

    const double tolerance = FRAME_MS / 1000.0;
    EXPECT_NEAR(spans[0].start().to_double(), 1.0, tolerance);
    EXPECT_NEAR(spans[0].end().to_double(), 2.2, tolerance);
}

TEST(SpeechDetectionCoreTests, ShortSilencesAreMerged)
{
    std::vector<float> samples = makeNoise(5.0, 0.001f);
    addTone(samples, 1.0, 1.5, 0.2f);
    // 200 ms pause, which is below the 300 ms minimum silence
    addTone(samples, 1.7, 2.2, 0.2f);

    SpeechDetectionSettings settings = makeSettings();
    settings.marginMs = 0.0;

    const TimeSpanList spans = detect(samples, settings);

    ASSERT_EQ(spans.size(), 1u);

    const double tolerance = FRAME_MS / 1000.0;
    EXPECT_NEAR(spans[0].start().to_double(), 1.0, tolerance);
    EXPECT_NEAR(spans[0].end().to_double(), 2.2, tolerance);
}

TEST(SpeechDetectionCoreTests, LongSilencesAreNotMerged)
{
    std::vector<float> samples = makeNoise(5.0, 0.001f);
    addTone(samples, 1.0, 1.5, 0.2f);
    // 400 ms pause, which is above the 300 ms minimum silence
    addTone(samples, 1.9, 2.4, 0.2f);

    SpeechDetectionSettings settings = makeSettings();
    settings.marginMs = 0.0;

    EXPECT_EQ(detect(samples, settings).size(), 2u);
}

TEST(SpeechDetectionCoreTests, BleedRejectionStopsTheQuieterTrack)
{
    // Track A carries the speech, track B carries the same signal 10 dB quieter
    std::vector<float> trackA = makeNoise(4.0, 0.001f);
    std::vector<float> trackB = makeNoise(4.0, 0.001f);

    addTone(trackA, 1.0, 2.0, 0.2f);
    addTone(trackB, 1.0, 2.0, 0.2f * 0.3162f); // -10 dB

    const SpeechDetectionSettings settings = makeSettings();

    const std::vector<double> framesA = frameRmsDb(trackA.data(), trackA.size(), FRAME_SIZE);
    const std::vector<double> framesB = frameRmsDb(trackB.data(), trackB.size(), FRAME_SIZE);

    // Without bleed rejection both tracks are marked as speech
    EXPECT_EQ(detect(trackA, settings).size(), 1u);
    EXPECT_EQ(detect(trackB, settings).size(), 1u);

    const std::vector<bool> maskA = bleedMask(framesA, { framesB }, 6.0);
    const std::vector<bool> maskB = bleedMask(framesB, { framesA }, 6.0);

    EXPECT_EQ(detect(trackA, settings, maskA).size(), 1u);
    EXPECT_TRUE(detect(trackB, settings, maskB).empty());
}

TEST(SpeechDetectionCoreTests, BleedMaskComparesAgainstTheLoudestOtherTrack)
{
    const std::vector<double> track { -20.0, -20.0, -20.0 };
    const std::vector<double> quiet { -40.0, -40.0, -40.0 };
    const std::vector<double> loud { -10.0, -30.0, -26.0 };

    const std::vector<bool> mask = bleedMask(track, { quiet, loud }, 6.0);

    ASSERT_EQ(mask.size(), 3u);
    EXPECT_FALSE(mask[0]); // the loud track wins
    EXPECT_TRUE(mask[1]);  // 10 dB above the loudest other track
    EXPECT_TRUE(mask[2]);  // exactly 6 dB above the loudest other track
}

/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <cstddef>
#include <vector>

#include "podcasttypes.h"

//! NOTE Pure, side effect free building blocks of the speech detection and of the
//! `Tighten gaps` maths. They know nothing about the project or about au3, which
//! makes them straightforward to unit test with synthetic buffers.
namespace au::podcast::core {
//! Energy reported for a frame that holds digital silence
constexpr double SILENCE_DB = -120.0;

//! A half open range of frame indices: [begin, end)
struct FrameRange {
    size_t begin = 0;
    size_t end = 0;

    bool operator==(const FrameRange& other) const { return begin == other.begin && end == other.end; }
    bool operator!=(const FrameRange& other) const { return !operator==(other); }
};

using FrameRanges = std::vector<FrameRange>;

//! Root mean square energy of every complete frame, in dBFS.
//! A trailing partial frame is ignored.
std::vector<double> frameRmsDb(const float* samples, size_t sampleCount, size_t frameSize);

//! Noise floor estimate: the 10th percentile of the frame energies
double noiseFloorDb(const std::vector<double>& framesDb);

//! A frame passes bleed rejection when the track is at least `bleedMarginDb`
//! louder than the loudest of the other analyzed tracks in that frame.
std::vector<bool> bleedMask(const std::vector<double>& trackFramesDb, const std::vector<std::vector<double> >& otherTracksFramesDb,
                            double bleedMarginDb);

//! Hysteresis with hangover, followed by dropping short speech regions and
//! merging regions separated by a short silence.
//! `allowedFrames` is the bleed rejection mask; an empty vector allows every frame.
FrameRanges detectFrameRanges(const std::vector<double>& framesDb, double noiseFloorDb, const SpeechDetectionSettings& settings,
                              const std::vector<bool>& allowedFrames = {});

//! Converts frame ranges to time spans, pads them by `marginMs`, clamps them to
//! [clampStart, clampEnd] and merges the spans that overlap after padding.
TimeSpanList framesToSpans(const FrameRanges& ranges, double frameMs, double offset, double clampStart, double clampEnd, double marginMs);

//! The parts of [begin, end] that are not covered by `spans`.
//! `spans` must be sorted and non-overlapping.
TimeSpanList complement(const TimeSpanList& spans, double begin, double end);

//! Overlap of two sorted, non-overlapping span lists
TimeSpanList intersect(const TimeSpanList& a, const TimeSpanList& b);

//! Overlap of any number of sorted, non-overlapping span lists.
//! An empty list of lists yields an empty result.
TimeSpanList intersectAll(const std::vector<TimeSpanList>& lists);

//! For every shared gap of length g that is at least `minGapMs`, the span of
//! `min(g * percent / 100, g - minGapMs)` taken from the middle of the gap.
//! The result is sorted ascending; callers are expected to apply it backwards.
TimeSpanList gapsToRemove(const TimeSpanList& sharedGaps, const TightenSettings& settings);
}

/*
* Audacity: A Digital Audio Editor
*/
#include "speechdetectioncore.h"

#include <algorithm>
#include <cmath>

namespace au::podcast::core {
namespace {
constexpr double MIN_RMS = 1e-6;

size_t msToFrames(double ms, double frameMs)
{
    if (frameMs <= 0.0 || ms <= 0.0) {
        return 0;
    }
    return static_cast<size_t>(std::ceil(ms / frameMs));
}

void appendMerged(TimeSpanList& spans, double start, double end)
{
    if (end <= start) {
        return;
    }

    if (!spans.empty() && start <= spans.back().end().to_double()) {
        const double mergedStart = spans.back().start().to_double();
        const double mergedEnd = std::max(spans.back().end().to_double(), end);
        spans.pop_back();
        spans.emplace_back(mergedStart, mergedEnd);
        return;
    }

    spans.emplace_back(start, end);
}
}

std::vector<double> frameRmsDb(const float* samples, size_t sampleCount, size_t frameSize)
{
    std::vector<double> framesDb;
    if (!samples || frameSize == 0) {
        return framesDb;
    }

    const size_t frameCount = sampleCount / frameSize;
    framesDb.reserve(frameCount);

    for (size_t frame = 0; frame < frameCount; ++frame) {
        const float* begin = samples + frame * frameSize;

        double sumOfSquares = 0.0;
        for (size_t i = 0; i < frameSize; ++i) {
            const double value = static_cast<double>(begin[i]);
            sumOfSquares += value * value;
        }

        const double rms = std::sqrt(sumOfSquares / static_cast<double>(frameSize));
        framesDb.push_back(rms <= MIN_RMS ? SILENCE_DB : std::max(SILENCE_DB, 20.0 * std::log10(rms)));
    }

    return framesDb;
}

double noiseFloorDb(const std::vector<double>& framesDb)
{
    if (framesDb.empty()) {
        return SILENCE_DB;
    }

    std::vector<double> sorted = framesDb;
    std::sort(sorted.begin(), sorted.end());

    const size_t index = static_cast<size_t>(0.1 * static_cast<double>(sorted.size() - 1));
    return sorted[index];
}

std::vector<bool> bleedMask(const std::vector<double>& trackFramesDb, const std::vector<std::vector<double> >& otherTracksFramesDb,
                            double bleedMarginDb)
{
    std::vector<bool> mask(trackFramesDb.size(), true);

    for (size_t frame = 0; frame < trackFramesDb.size(); ++frame) {
        double loudestOther = SILENCE_DB;
        for (const std::vector<double>& other : otherTracksFramesDb) {
            if (frame < other.size()) {
                loudestOther = std::max(loudestOther, other[frame]);
            }
        }

        mask[frame] = trackFramesDb[frame] >= loudestOther + bleedMarginDb;
    }

    return mask;
}

FrameRanges detectFrameRanges(const std::vector<double>& framesDb, double noiseFloorDb, const SpeechDetectionSettings& settings,
                              const std::vector<bool>& allowedFrames)
{
    FrameRanges ranges;
    if (framesDb.empty()) {
        return ranges;
    }

    const double openThreshold = noiseFloorDb + settings.openDb;
    const double closeThreshold = noiseFloorDb + settings.closeDb;
    const size_t hangoverFrames = msToFrames(settings.hangoverMs, settings.frameMs);

    const auto isAllowed = [&allowedFrames](size_t frame) {
        return allowedFrames.empty() || (frame < allowedFrames.size() && allowedFrames[frame]);
    };

    bool open = false;
    size_t regionBegin = 0;
    size_t lastAbove = 0;

    for (size_t frame = 0; frame < framesDb.size(); ++frame) {
        if (!open) {
            if (framesDb[frame] >= openThreshold && isAllowed(frame)) {
                open = true;
                regionBegin = frame;
                lastAbove = frame;
            }
            continue;
        }

        if (framesDb[frame] >= closeThreshold && isAllowed(frame)) {
            lastAbove = frame;
            continue;
        }

        if (frame - lastAbove > hangoverFrames) {
            ranges.push_back({ regionBegin, std::min(framesDb.size(), lastAbove + 1 + hangoverFrames) });
            open = false;
        }
    }

    if (open) {
        ranges.push_back({ regionBegin, std::min(framesDb.size(), lastAbove + 1 + hangoverFrames) });
    }

    // Drop speech regions that are too short to be speech
    const size_t minSpeechFrames = msToFrames(settings.minSpeechMs, settings.frameMs);
    FrameRanges longEnough;
    longEnough.reserve(ranges.size());
    for (const FrameRange& range : ranges) {
        if (range.end - range.begin >= minSpeechFrames) {
            longEnough.push_back(range);
        }
    }

    // Merge regions that are separated by a silence that is too short to be a real pause
    const size_t minSilenceFrames = msToFrames(settings.minSilenceMs, settings.frameMs);
    FrameRanges merged;
    merged.reserve(longEnough.size());
    for (const FrameRange& range : longEnough) {
        if (!merged.empty() && range.begin - merged.back().end < minSilenceFrames) {
            merged.back().end = std::max(merged.back().end, range.end);
            continue;
        }
        merged.push_back(range);
    }

    return merged;
}

TimeSpanList framesToSpans(const FrameRanges& ranges, double frameMs, double offset, double clampStart, double clampEnd, double marginMs)
{
    TimeSpanList spans;
    spans.reserve(ranges.size());

    const double frameSecs = frameMs / 1000.0;
    const double marginSecs = marginMs / 1000.0;

    for (const FrameRange& range : ranges) {
        double start = offset + static_cast<double>(range.begin) * frameSecs - marginSecs;
        double end = offset + static_cast<double>(range.end) * frameSecs + marginSecs;

        start = std::max(start, clampStart);
        end = std::min(end, clampEnd);

        appendMerged(spans, start, end);
    }

    return spans;
}

TimeSpanList complement(const TimeSpanList& spans, double begin, double end)
{
    TimeSpanList result;
    if (end <= begin) {
        return result;
    }

    double position = begin;
    for (const TimeSpan& span : spans) {
        const double spanStart = std::max(span.start().to_double(), begin);
        const double spanEnd = std::min(span.end().to_double(), end);
        if (spanEnd <= position) {
            continue;
        }

        if (spanStart > position) {
            result.emplace_back(position, spanStart);
        }
        position = spanEnd;
    }

    if (position < end) {
        result.emplace_back(position, end);
    }

    return result;
}

TimeSpanList intersect(const TimeSpanList& a, const TimeSpanList& b)
{
    TimeSpanList result;

    size_t i = 0;
    size_t j = 0;
    while (i < a.size() && j < b.size()) {
        const double start = std::max(a[i].start().to_double(), b[j].start().to_double());
        const double end = std::min(a[i].end().to_double(), b[j].end().to_double());
        if (start < end) {
            result.emplace_back(start, end);
        }

        if (a[i].end().to_double() < b[j].end().to_double()) {
            ++i;
        } else {
            ++j;
        }
    }

    return result;
}

TimeSpanList intersectAll(const std::vector<TimeSpanList>& lists)
{
    if (lists.empty()) {
        return {};
    }

    //! NOTE TimeSpan holds const members and is therefore not assignable,
    //! so the intermediate results are swapped in rather than assigned
    TimeSpanList result(lists.front());
    for (size_t i = 1; i < lists.size(); ++i) {
        TimeSpanList overlap = intersect(result, lists[i]);
        result.swap(overlap);
        if (result.empty()) {
            break;
        }
    }

    return result;
}

TimeSpanList gapsToRemove(const TimeSpanList& sharedGaps, const TightenSettings& settings)
{
    TimeSpanList result;

    for (const TimeSpan& gap : sharedGaps) {
        const double gapMs = gap.duration().to_double() * 1000.0;
        if (gapMs < settings.minGapMs) {
            continue;
        }

        const double removeMs = std::min(gapMs * settings.percent / 100.0, gapMs - settings.minGapMs);
        if (removeMs <= 0.0) {
            continue;
        }

        const double removeSecs = removeMs / 1000.0;
        const double start = gap.start().to_double() + (gap.duration().to_double() - removeSecs) / 2.0;
        result.emplace_back(start, start + removeSecs);
    }

    return result;
}
}

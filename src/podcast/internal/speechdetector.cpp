/*
* Audacity: A Digital Audio Editor
*/
#include "speechdetector.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <future>

#include "au3-math/SampleFormat.h"
#include "au3-wave-track/WaveTrack.h"

#include "au3wrap/internal/domaccessor.h"

#include "trackedit/itrackeditproject.h"

#include "../podcasterrors.h"

#include "log.h"
#include "translation.h"

using namespace au::podcast;
using namespace au::au3;

namespace {
//! Number of samples per channel read in one go. Keeps the memory use bounded
//! no matter how long the processed range is.
constexpr size_t BLOCK_SIZE = 65536;
}

Au3Project& SpeechDetector::projectRef() const
{
    return *reinterpret_cast<Au3Project*>(globalContext()->currentProject()->au3ProjectPtr());
}

muse::Progress SpeechDetector::progress() const
{
    return m_progress;
}

SpeechDetectionSettings SpeechDetector::settingsForTrack(const SpeechDetectionRequest& request, TrackId trackId) const
{
    const auto project = globalContext()->currentTrackeditProject();
    if (!project) {
        return request.settings;
    }

    const std::optional<std::string> title = project->trackName(trackId);
    if (!title.has_value()) {
        return request.settings;
    }

    const auto it = request.overrides.find(title.value());
    if (it == request.overrides.end()) {
        return request.settings;
    }

    SpeechDetectionSettings settings = it->second;
    //! NOTE All tracks share one frame grid, otherwise the frames of the
    //! different tracks could not be compared for bleed rejection.
    settings.frameMs = request.settings.frameMs;
    return settings;
}

bool SpeechDetector::readTrackFrames(TrackId trackId, double begin, double end, double frameMs, TrackFrames& result) const
{
    Au3WaveTrack* waveTrack = DomAccessor::findWaveTrack(projectRef(), Au3TrackId(trackId));
    if (!waveTrack) {
        return false;
    }

    const double rate = waveTrack->GetRate();
    const size_t nChannels = waveTrack->NChannels();
    if (rate <= 0.0 || nChannels == 0) {
        return false;
    }

    const size_t frameSize = std::max<size_t>(1, static_cast<size_t>(std::llround(frameMs / 1000.0 * rate)));

    result.trackId = trackId;
    result.clampStart = std::max(begin, waveTrack->GetStartTime());
    result.clampEnd = std::min(end, waveTrack->GetEndTime());

    const sampleCount firstSample = waveTrack->TimeToLongSamples(begin);
    const sampleCount lastSample = waveTrack->TimeToLongSamples(end);
    if (lastSample <= firstSample) {
        return true;
    }

    std::vector<std::vector<float> > channelBuffers(nChannels, std::vector<float>(BLOCK_SIZE, 0.f));
    std::vector<float*> buffers(nChannels, nullptr);
    for (size_t channel = 0; channel < nChannels; ++channel) {
        buffers[channel] = channelBuffers[channel].data();
    }

    std::vector<float> mono;
    std::vector<float> pending;
    pending.reserve(BLOCK_SIZE + frameSize);

    sampleCount position = firstSample;
    while (position < lastSample) {
        if (m_progress.isCanceled()) {
            return false;
        }

        const size_t length = std::min<size_t>(BLOCK_SIZE, (lastSample - position).as_size_t());

        //! NOTE `WaveTrack` is a `WideSampleSequence`; `GetFloats` fills one buffer per
        //! channel and zero-fills the parts of the range that hold no audio.
        if (!waveTrack->GetFloats(0, nChannels, buffers.data(), position, length, false, FillFormat::fillZero, false)) {
            LOGW() << "could not read samples of track " << trackId << " at " << position.as_long_long();
            return false;
        }

        mono.resize(length);
        for (size_t i = 0; i < length; ++i) {
            double sum = 0.0;
            for (size_t channel = 0; channel < nChannels; ++channel) {
                sum += static_cast<double>(channelBuffers[channel][i]);
            }
            mono[i] = static_cast<float>(sum / static_cast<double>(nChannels));
        }

        pending.insert(pending.end(), mono.begin(), mono.end());

        const size_t completeFrames = pending.size() / frameSize;
        if (completeFrames > 0) {
            const std::vector<double> framesDb = core::frameRmsDb(pending.data(), completeFrames * frameSize, frameSize);
            result.framesDb.insert(result.framesDb.end(), framesDb.begin(), framesDb.end());
            pending.erase(pending.begin(), pending.begin() + completeFrames * frameSize);
        }

        position += length;

        m_progress.progress((position - firstSample).as_long_long(), (lastSample - firstSample).as_long_long(),
                            muse::trc("podcast", "Analysing speech"));
    }

    return true;
}

muse::RetVal<SpeechRanges> SpeechDetector::detect(const SpeechDetectionRequest& request)
{
    muse::RetVal<SpeechRanges> result;

    if (!globalContext()->currentProject()) {
        result.ret = make_ret(Err::NoProjectOpened);
        return result;
    }

    const double begin = request.begin.to_double();
    const double end = request.end.to_double();
    if (request.trackIds.empty() || end <= begin) {
        result.ret = make_ret(Err::EmptyRange);
        return result;
    }

    const double frameMs = request.settings.frameMs > 0.0 ? request.settings.frameMs : SpeechDetectionSettings().frameMs;

    m_progress.start();

    std::vector<TrackFrames> tracks;
    tracks.reserve(request.trackIds.size());

    for (const TrackId& trackId : request.trackIds) {
        TrackFrames frames;
        frames.settings = settingsForTrack(request, trackId);

        if (!readTrackFrames(trackId, begin, end, frameMs, frames)) {
            if (m_progress.isCanceled()) {
                result.ret = make_ret(Err::Cancel);
                m_progress.finish(result.ret);
                return result;
            }
            continue;
        }

        tracks.push_back(std::move(frames));
    }

    if (tracks.empty()) {
        result.ret = make_ret(Err::NoAudioTracks);
        m_progress.finish(result.ret);
        return result;
    }

    //! NOTE The frame analysis is pure maths on the extracted energies and does not
    //! touch the project, so it is moved off the UI thread.
    std::future<SpeechRanges> analysis = std::async(std::launch::async, [&tracks, frameMs, begin]() {
        SpeechRanges ranges;

        std::vector<double> noiseFloors;
        noiseFloors.reserve(tracks.size());
        for (const TrackFrames& track : tracks) {
            noiseFloors.push_back(core::noiseFloorDb(track.framesDb));
        }

        for (size_t i = 0; i < tracks.size(); ++i) {
            const TrackFrames& track = tracks[i];

            std::vector<bool> allowedFrames;
            if (track.settings.bleedRejection && tracks.size() > 1) {
                std::vector<std::vector<double> > others;
                others.reserve(tracks.size() - 1);
                for (size_t j = 0; j < tracks.size(); ++j) {
                    if (j != i) {
                        others.push_back(tracks[j].framesDb);
                    }
                }
                allowedFrames = core::bleedMask(track.framesDb, others, track.settings.bleedMarginDb);
            }

            const core::FrameRanges frameRanges = core::detectFrameRanges(track.framesDb, noiseFloors[i], track.settings, allowedFrames);

            ranges.emplace(track.trackId,
                           core::framesToSpans(frameRanges, frameMs, begin, track.clampStart, track.clampEnd,
                                               track.settings.marginMs));
        }

        return ranges;
    });

    result.val = analysis.get();

    if (m_progress.isCanceled()) {
        result.ret = make_ret(Err::Cancel);
        result.val.clear();
    } else {
        result.ret = make_ret(Err::NoError);
    }

    m_progress.finish(result.ret);

    return result;
}

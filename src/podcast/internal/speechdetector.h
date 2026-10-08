/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include "global/modularity/ioc.h"

#include "context/iglobalcontext.h"

#include "au3wrap/au3types.h"

#include "../ispeechdetector.h"
#include "speechdetectioncore.h"

namespace au::podcast {
class SpeechDetector : public ISpeechDetector, public muse::Contextable
{
    muse::ContextInject<au::context::IGlobalContext> globalContext { this };

public:
    SpeechDetector(const muse::modularity::ContextPtr& ctx)
        : muse::Contextable(ctx) {}

    muse::RetVal<SpeechRanges> detect(const SpeechDetectionRequest& request) override;

    muse::Progress progress() const override;

private:
    struct TrackFrames {
        TrackId trackId = trackedit::INVALID_TRACK;
        std::vector<double> framesDb;
        //! Bounds the detected spans get clamped to
        double clampStart = 0.0;
        double clampEnd = 0.0;
        SpeechDetectionSettings settings;
    };

    au::au3::Au3Project& projectRef() const;

    SpeechDetectionSettings settingsForTrack(const SpeechDetectionRequest& request, TrackId trackId) const;

    //! Reads the samples of one track in bounded blocks, mixes them down to mono
    //! and returns the energy of every analysis frame.
    //! NOTE The au3 sample access is not thread safe, so this part runs on the
    //! calling (project) thread; only the frame analysis is moved to a worker.
    bool readTrackFrames(TrackId trackId, double begin, double end, double frameMs, TrackFrames& result) const;

    mutable muse::Progress m_progress;
};
}

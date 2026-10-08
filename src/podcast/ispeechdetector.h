/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include "global/modularity/imoduleinterface.h"
#include "global/progress.h"
#include "global/types/retval.h"

#include "podcasttypes.h"

namespace au::podcast {
struct SpeechDetectionRequest {
    //! The tracks to analyze. All of them take part in bleed rejection.
    TrackIdList trackIds;
    //! The time range to analyze
    trackedit::secs_t begin = 0.0;
    trackedit::secs_t end = 0.0;
    //! Settings used for every track that has no override
    SpeechDetectionSettings settings;
    //! Overrides keyed by track title
    SpeechDetectionOverrides overrides;
};

//! NOTE Analysis only: this service never modifies the project
class ISpeechDetector : MODULE_EXPORT_INTERFACE
{
    INTERFACE_ID(au::podcast::ISpeechDetector)

public:
    virtual ~ISpeechDetector() = default;

    //! Returns the detected speech ranges per track.
    //! The frame analysis runs off the UI thread; progress is reported and
    //! cancellation is observed through `progress()`.
    virtual muse::RetVal<SpeechRanges> detect(const SpeechDetectionRequest& request) = 0;

    virtual muse::Progress progress() const = 0;
};
}

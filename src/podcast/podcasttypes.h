/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <map>
#include <string>
#include <vector>

#include "trackedit/timespan.h"
#include "trackedit/trackedittypes.h"

namespace au::podcast {
using trackedit::TimeSpan;
using trackedit::TrackId;
using trackedit::TrackIdList;

using TimeSpanList = std::vector<TimeSpan>;
using SpeechRanges = std::map<TrackId, TimeSpanList>;

//! NOTE Analysis parameters, mirroring the per-track configuration of audiome's `strip_config.json`
struct SpeechDetectionSettings {
    //! Length of one analysis frame, in milliseconds
    double frameMs = 20.0;
    //! A region opens when the frame energy rises above `noiseFloor + openDb`
    double openDb = 10.0;
    //! A region closes when the frame energy drops below `noiseFloor + closeDb`
    double closeDb = 6.0;
    //! A region is kept open for that long after the energy dropped below the close threshold
    double hangoverMs = 200.0;
    //! Speech regions shorter than that are dropped
    double minSpeechMs = 150.0;
    //! Silences shorter than that do not split a speech region
    double minSilenceMs = 300.0;
    //! Every speech region is padded by that much on both sides
    double marginMs = 50.0;
    //! Multitrack bleed rejection: a frame is speech on a track only if it is
    //! `bleedMarginDb` louder than the loudest other analyzed track in that frame
    bool bleedRejection = true;
    double bleedMarginDb = 6.0;

    bool operator==(const SpeechDetectionSettings& other) const
    {
        return frameMs == other.frameMs
               && openDb == other.openDb
               && closeDb == other.closeDb
               && hangoverMs == other.hangoverMs
               && minSpeechMs == other.minSpeechMs
               && minSilenceMs == other.minSilenceMs
               && marginMs == other.marginMs
               && bleedRejection == other.bleedRejection
               && bleedMarginDb == other.bleedMarginDb;
    }

    bool operator!=(const SpeechDetectionSettings& other) const { return !operator==(other); }
};

//! NOTE Settings of the `Tighten gaps` operation, mirroring audiome's `tighten` command
struct TightenSettings {
    //! Shared gaps shorter than that are left alone
    double minGapMs = 250.0;
    //! Share of the gap that gets removed, in percent
    double percent = 30.0;
};

//! NOTE Per-track overrides, keyed by track title (as in audiome's `strip_config.json`)
using SpeechDetectionOverrides = std::map<std::string, SpeechDetectionSettings>;
}

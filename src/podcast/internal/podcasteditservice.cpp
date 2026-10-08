/*
* Audacity: A Digital Audio Editor
*/
#include "podcasteditservice.h"

#include <algorithm>

#include "containers.h"

#include "trackedit/itrackeditproject.h"

#include "../podcasterrors.h"
#include "speechdetectioncore.h"

#include "log.h"
#include "translation.h"

using namespace au::podcast;
using namespace au::trackedit;

namespace {
//! Removals shorter than that are not worth an edit
constexpr double MIN_REMOVAL_SECS = 1e-6;

bool isAudioTrack(const Track& track)
{
    return track.type == TrackType::Mono || track.type == TrackType::Stereo;
}
}

muse::RetVal<PodcastEditService::Scope> PodcastEditService::currentScope() const
{
    muse::RetVal<Scope> result;

    const ITrackeditProjectPtr project = globalContext()->currentTrackeditProject();
    if (!project) {
        result.ret = make_ret(Err::NoProjectOpened);
        return result;
    }

    const TrackList tracks = project->trackList();

    const TrackIdList selectedTracks = selectionController()->selectedTracks();
    for (const Track& track : tracks) {
        if (!isAudioTrack(track)) {
            continue;
        }
        if (selectedTracks.empty() || muse::contains(selectedTracks, track.id)) {
            result.val.trackIds.push_back(track.id);
        }
    }

    if (result.val.trackIds.empty()) {
        result.ret = make_ret(Err::NoAudioTracks);
        return result;
    }

    double begin = selectionController()->dataSelectedStartTime().to_double();
    double end = selectionController()->dataSelectedEndTime().to_double();

    if (selectionController()->timeSelectionIsEmpty() || end <= begin) {
        //! NOTE Nothing is selected in time: process the whole project
        begin = 0.0;
        end = 0.0;
        for (const TrackId& trackId : result.val.trackIds) {
            for (const Clip& clip : project->clipList(trackId)) {
                end = std::max(end, clip.endTime);
            }
        }
    }

    if (end <= begin) {
        result.ret = make_ret(Err::EmptyRange);
        return result;
    }

    result.val.begin = begin;
    result.val.end = end;
    result.ret = make_ret(Err::NoError);

    return result;
}

muse::RetVal<SpeechRanges> PodcastEditService::detectSpeech(const Scope& scope) const
{
    SpeechDetectionRequest request;
    request.trackIds = scope.trackIds;
    request.begin = scope.begin;
    request.end = scope.end;
    request.settings = configuration()->speechDetectionSettings();
    request.overrides = configuration()->speechDetectionOverrides();

    return speechDetector()->detect(request);
}

bool PodcastEditService::removeBackwards(const TrackIdList& trackIds, const TimeSpanList& removals, bool moveClips)
{
    for (auto it = removals.rbegin(); it != removals.rend(); ++it) {
        if (it->duration().to_double() < MIN_REMOVAL_SECS) {
            continue;
        }

        //! NOTE `ITracksInteraction` is the layer below `ITrackeditInteraction`:
        //! `ITrackeditInteraction::removeTracksData` would push one history entry per call,
        //! while an operation of this module has to be a single undo step.
        if (!tracksInteraction()->removeTracksData(trackIds, it->start(), it->end(), moveClips)) {
            return false;
        }
    }

    return true;
}

muse::Ret PodcastEditService::stripSilence()
{
    const muse::RetVal<Scope> scope = currentScope();
    if (!scope.ret) {
        return scope.ret;
    }

    const muse::RetVal<SpeechRanges> speech = detectSpeech(scope.val);
    if (!speech.ret) {
        return speech.ret;
    }

    bool anySpeech = false;
    for (const auto& it : speech.val) {
        anySpeech = anySpeech || !it.second.empty();
    }
    if (!anySpeech) {
        return make_ret(Err::NoSpeechDetected);
    }

    projectHistory()->startUserInteraction();

    for (const TrackId& trackId : scope.val.trackIds) {
        const auto it = speech.val.find(trackId);
        if (it == speech.val.end()) {
            continue;
        }

        const TimeSpanList silences = core::complement(it->second, scope.val.begin, scope.val.end);

        //! NOTE Nothing after a removed range moves, so the tracks stay in sync
        if (!removeBackwards({ trackId }, silences, false)) {
            projectHistory()->rollbackState();
            projectHistory()->endUserInteraction();
            return make_ret(Err::FailedToApplyEdits);
        }
    }

    //: Undo history entry name; shown after Undo and Redo in the Edit menu
    projectHistory()->pushHistoryState(muse::trc("podcast", "Strip silence"), muse::trc("podcast", "Strip silence"));
    projectHistory()->endUserInteraction();

    return make_ret(Err::NoError);
}

muse::Ret PodcastEditService::tightenGaps()
{
    const muse::RetVal<Scope> scope = currentScope();
    if (!scope.ret) {
        return scope.ret;
    }

    const muse::RetVal<SpeechRanges> speech = detectSpeech(scope.val);
    if (!speech.ret) {
        return speech.ret;
    }

    std::vector<TimeSpanList> silencesPerTrack;
    silencesPerTrack.reserve(scope.val.trackIds.size());
    for (const TrackId& trackId : scope.val.trackIds) {
        const auto it = speech.val.find(trackId);
        if (it == speech.val.end()) {
            continue;
        }
        silencesPerTrack.push_back(core::complement(it->second, scope.val.begin, scope.val.end));
    }

    const TimeSpanList sharedGaps = core::intersectAll(silencesPerTrack);
    const TimeSpanList removals = core::gapsToRemove(sharedGaps, configuration()->tightenSettings());
    if (removals.empty()) {
        return make_ret(Err::NoError);
    }

    projectHistory()->startUserInteraction();

    //! NOTE Everything after a gap moves left on every track at once
    if (!removeBackwards(scope.val.trackIds, removals, true)) {
        projectHistory()->rollbackState();
        projectHistory()->endUserInteraction();
        return make_ret(Err::FailedToApplyEdits);
    }

    //: Undo history entry name; shown after Undo and Redo in the Edit menu
    projectHistory()->pushHistoryState(muse::trc("podcast", "Tighten gaps"), muse::trc("podcast", "Tighten gaps"));
    projectHistory()->endUserInteraction();

    return make_ret(Err::NoError);
}

muse::Ret PodcastEditService::markSpeech()
{
    const muse::RetVal<Scope> scope = currentScope();
    if (!scope.ret) {
        return scope.ret;
    }

    const muse::RetVal<SpeechRanges> speech = detectSpeech(scope.val);
    if (!speech.ret) {
        return speech.ret;
    }

    const ITrackeditProjectPtr project = globalContext()->currentTrackeditProject();
    if (!project) {
        return make_ret(Err::NoProjectOpened);
    }

    projectHistory()->startUserInteraction();

    for (const TrackId& trackId : scope.val.trackIds) {
        const auto it = speech.val.find(trackId);
        if (it == speech.val.end() || it->second.empty()) {
            continue;
        }

        const std::optional<Track> track = project->track(trackId);
        const muse::String title = muse::String(u"Speech – ") + (track.has_value() ? track->title : muse::String());

        const muse::RetVal<TrackId> labelTrack = tracksInteraction()->newLabelTrack(title);
        if (!labelTrack.ret) {
            projectHistory()->rollbackState();
            projectHistory()->endUserInteraction();
            return make_ret(Err::FailedToCreateLabelTrack);
        }

        for (const TimeSpan& span : it->second) {
            const muse::RetVal<LabelKey> label = labelsInteraction()->addLabel(labelTrack.val);
            if (!label.ret) {
                continue;
            }

            //! NOTE A new label is created empty at the origin, so it is first
            //! stretched to its end and then moved to its start.
            labelsInteraction()->stretchLabelRight(label.val, span.end(), true);
            labelsInteraction()->resetLabelStretchState();
            labelsInteraction()->stretchLabelLeft(label.val, span.start(), true);
            labelsInteraction()->resetLabelStretchState();

            labelsInteraction()->changeLabelTitle(label.val, muse::String(u"Speech"));
        }
    }

    //: Undo history entry name; shown after Undo and Redo in the Edit menu
    projectHistory()->pushHistoryState(muse::trc("podcast", "Mark speech"), muse::trc("podcast", "Mark speech"));
    projectHistory()->endUserInteraction();

    return make_ret(Err::NoError);
}

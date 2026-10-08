/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include "global/modularity/ioc.h"

#include "context/iglobalcontext.h"

#include "trackedit/ilabelsinteraction.h"
#include "trackedit/iprojecthistory.h"
#include "trackedit/iselectioncontroller.h"
#include "trackedit/itracksinteraction.h"

#include "../ipodcastconfiguration.h"
#include "../ipodcasteditservice.h"
#include "../ispeechdetector.h"

namespace au::podcast {
class PodcastEditService : public IPodcastEditService, public muse::Contextable
{
    muse::GlobalInject<IPodcastConfiguration> configuration;

    muse::ContextInject<au::context::IGlobalContext> globalContext { this };
    muse::ContextInject<ISpeechDetector> speechDetector { this };
    muse::ContextInject<trackedit::ISelectionController> selectionController { this };
    muse::ContextInject<trackedit::IProjectHistory> projectHistory { this };
    muse::ContextInject<trackedit::ITracksInteraction> tracksInteraction { this };
    muse::ContextInject<trackedit::ILabelsInteraction> labelsInteraction { this };

public:
    PodcastEditService(const muse::modularity::ContextPtr& ctx)
        : muse::Contextable(ctx) {}

    muse::Ret stripSilence() override;
    muse::Ret tightenGaps() override;
    muse::Ret markSpeech() override;

private:
    struct Scope {
        TrackIdList trackIds;
        double begin = 0.0;
        double end = 0.0;
    };

    //! The selected tracks and time range, or the whole project if nothing is selected
    muse::RetVal<Scope> currentScope() const;

    muse::RetVal<SpeechRanges> detectSpeech(const Scope& scope) const;

    //! Applies `removals` from the end to the start, so that the positions of the
    //! not yet applied removals stay valid.
    bool removeBackwards(const TrackIdList& trackIds, const TimeSpanList& removals, bool moveClips);
};
}

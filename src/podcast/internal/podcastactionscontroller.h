/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include "framework/actions/actionable.h"
#include "framework/actions/iactionsdispatcher.h"
#include "framework/global/async/asyncable.h"
#include "framework/global/modularity/ioc.h"
#include "framework/interactive/iinteractive.h"

#include "context/iglobalcontext.h"

#include "../ipodcasteditservice.h"

namespace au::podcast {
static const muse::actions::ActionCode PODCAST_STRIP_SILENCE_CODE("podcast-strip-silence");
static const muse::actions::ActionCode PODCAST_TIGHTEN_GAPS_CODE("podcast-tighten-gaps");
static const muse::actions::ActionCode PODCAST_MARK_SPEECH_CODE("podcast-mark-speech");
static const muse::actions::ActionCode PODCAST_SETTINGS_CODE("podcast-settings");

class PodcastActionsController : public muse::actions::Actionable, public muse::async::Asyncable, public muse::Contextable
{
    muse::ContextInject<au::context::IGlobalContext> globalContext { this };
    muse::ContextInject<muse::actions::IActionsDispatcher> dispatcher { this };
    muse::ContextInject<muse::IInteractive> interactive { this };
    muse::ContextInject<IPodcastEditService> editService { this };

public:
    PodcastActionsController(const muse::modularity::ContextPtr& ctx)
        : muse::Contextable(ctx) {}

    void init();

    bool canReceiveAction(const muse::actions::ActionCode& actionCode) const override;

    muse::async::Channel<muse::actions::ActionCode> actionEnabledChanged() const;

private:
    void stripSilence();
    void tightenGaps();
    void markSpeech();
    void openSettings();

    void showErrorIfNeeded(const muse::Ret& ret);

    muse::async::Channel<muse::actions::ActionCode> m_actionEnabledChanged;
};
}

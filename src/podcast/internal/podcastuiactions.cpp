/*
* Audacity: A Digital Audio Editor
*/
#include "podcastuiactions.h"

#include "context/shortcutcontext.h"
#include "context/uicontext.h"
#include "types/translatablestring.h"

using namespace au::podcast;
using namespace muse;
using namespace muse::ui;
using namespace muse::actions;

namespace {
const UiActionList STATIC_ACTIONS = {
    UiAction(PODCAST_STRIP_SILENCE_CODE,
             au::context::UiCtxProjectOpened,
             au::context::CTX_PROJECT_OPENED,
             //: Action title: shown as a menu item or a button label; keep it short
             TranslatableString("action", "Strip silence"),
             //: Action description: shown as a tooltip; can be a full sentence
             TranslatableString("action_description", "Remove the silent parts of every processed track, keeping the tracks in sync")
             ),
    UiAction(PODCAST_TIGHTEN_GAPS_CODE,
             au::context::UiCtxProjectOpened,
             au::context::CTX_PROJECT_OPENED,
             //: Action title: shown as a menu item or a button label; keep it short
             TranslatableString("action", "Tighten gaps"),
             //: Action description: shown as a tooltip; can be a full sentence
             TranslatableString("action_description", "Shorten the pauses that all processed tracks share")
             ),
    UiAction(PODCAST_MARK_SPEECH_CODE,
             au::context::UiCtxProjectOpened,
             au::context::CTX_PROJECT_OPENED,
             //: Action title: shown as a menu item or a button label; keep it short
             TranslatableString("action", "Mark speech as labels"),
             //: Action description: shown as a tooltip; can be a full sentence
             TranslatableString("action_description", "Add a label track holding the detected speech ranges")
             ),
    UiAction(PODCAST_SETTINGS_CODE,
             au::context::UiCtxProjectOpened,
             au::context::CTX_PROJECT_OPENED,
             //: Action title: shown as a menu item or a button label; keep it short
             TranslatableString("action", "Podcast settings…"),
             //: Action description: shown as a tooltip; can be a full sentence
             TranslatableString("action_description", "Edit the speech detection and gap tightening settings")
             )
};
}

PodcastUiActions::PodcastUiActions(const muse::modularity::ContextPtr& ctx, std::shared_ptr<PodcastActionsController> controller)
    : muse::Contextable(ctx), m_controller(controller)
{
}

void PodcastUiActions::init()
{
    m_controller->actionEnabledChanged().onReceive(this, [this](const ActionCode& code) {
        m_actionEnabledChanged.send({ code });
    });
}

const UiActionList& PodcastUiActions::actionsList() const
{
    return STATIC_ACTIONS;
}

bool PodcastUiActions::actionEnabled(const UiAction& act) const
{
    return m_controller->canReceiveAction(act.code);
}

bool PodcastUiActions::actionChecked(const UiAction&) const
{
    return false;
}

muse::async::Channel<ActionCodeList> PodcastUiActions::actionEnabledChanged() const
{
    return m_actionEnabledChanged;
}

muse::async::Channel<ActionCodeList> PodcastUiActions::actionCheckedChanged() const
{
    return m_actionCheckedChanged;
}

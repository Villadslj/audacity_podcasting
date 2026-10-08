/*
* Audacity: A Digital Audio Editor
*/
#include "podcastactionscontroller.h"

#include "../podcasterrors.h"

#include "log.h"
#include "translation.h"

using namespace au::podcast;
using namespace muse;
using namespace muse::actions;

void PodcastActionsController::init()
{
    dispatcher()->reg(this, PODCAST_STRIP_SILENCE_CODE, this, &PodcastActionsController::stripSilence);
    dispatcher()->reg(this, PODCAST_TIGHTEN_GAPS_CODE, this, &PodcastActionsController::tightenGaps);
    dispatcher()->reg(this, PODCAST_MARK_SPEECH_CODE, this, &PodcastActionsController::markSpeech);
    dispatcher()->reg(this, PODCAST_SETTINGS_CODE, this, &PodcastActionsController::openSettings);

    globalContext()->currentProjectChanged().onNotify(this, [this]() {
        m_actionEnabledChanged.send(PODCAST_STRIP_SILENCE_CODE);
        m_actionEnabledChanged.send(PODCAST_TIGHTEN_GAPS_CODE);
        m_actionEnabledChanged.send(PODCAST_MARK_SPEECH_CODE);
        m_actionEnabledChanged.send(PODCAST_SETTINGS_CODE);
    });
}

bool PodcastActionsController::canReceiveAction(const ActionCode&) const
{
    return globalContext()->currentProject() != nullptr;
}

muse::async::Channel<ActionCode> PodcastActionsController::actionEnabledChanged() const
{
    return m_actionEnabledChanged;
}

void PodcastActionsController::showErrorIfNeeded(const muse::Ret& ret)
{
    if (ret) {
        return;
    }

    if (ret.code() == static_cast<int>(Err::Cancel)) {
        return;
    }

    LOGE() << "podcast operation failed: " << ret.toString();
    interactive()->error(muse::trc("podcast", "Podcast"), ret.text());
}

void PodcastActionsController::stripSilence()
{
    showErrorIfNeeded(editService()->stripSilence());
}

void PodcastActionsController::tightenGaps()
{
    showErrorIfNeeded(editService()->tightenGaps());
}

void PodcastActionsController::markSpeech()
{
    showErrorIfNeeded(editService()->markSpeech());
}

void PodcastActionsController::openSettings()
{
    interactive()->open(muse::Uri("audacity://podcast/settings"));
}

/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include "framework/global/async/asyncable.h"
#include "framework/global/modularity/ioc.h"
#include "framework/ui/iuiactionsmodule.h"

#include "podcastactionscontroller.h"

namespace au::podcast {
class PodcastUiActions : public muse::ui::IUiActionsModule, public muse::async::Asyncable, public muse::Contextable
{
public:
    PodcastUiActions(const muse::modularity::ContextPtr& ctx, std::shared_ptr<PodcastActionsController> controller);

    void init();

    const muse::ui::UiActionList& actionsList() const override;

    bool actionEnabled(const muse::ui::UiAction& act) const override;
    muse::async::Channel<muse::actions::ActionCodeList> actionEnabledChanged() const override;

    bool actionChecked(const muse::ui::UiAction& act) const override;
    muse::async::Channel<muse::actions::ActionCodeList> actionCheckedChanged() const override;

private:
    std::shared_ptr<PodcastActionsController> m_controller;
    muse::async::Channel<muse::actions::ActionCodeList> m_actionEnabledChanged;
    muse::async::Channel<muse::actions::ActionCodeList> m_actionCheckedChanged;
};
}

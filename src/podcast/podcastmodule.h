/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include "modularity/imodulesetup.h"

namespace au::podcast {
class PodcastConfiguration;
class PodcastActionsController;
class PodcastUiActions;

class PodcastModule : public muse::modularity::IModuleSetup
{
public:
    PodcastModule();

    std::string moduleName() const override;
    void registerExports() override;
    void registerResources() override;
    void registerUiTypes() override;
    void resolveImports() override;
    void onInit(const muse::IApplication::RunMode& mode) override;

    muse::modularity::IContextSetup* newContext(const muse::modularity::ContextPtr& ctx) const override;

private:
    std::shared_ptr<PodcastConfiguration> m_configuration;
};

class PodcastContext : public muse::modularity::IContextSetup
{
public:
    PodcastContext(const muse::modularity::ContextPtr& ctx)
        : muse::modularity::IContextSetup(ctx) {}

    void registerExports() override;
    void onInit(const muse::IApplication::RunMode& mode) override;
    void onDeinit() override;

private:
    std::shared_ptr<PodcastActionsController> m_actionsController;
    std::shared_ptr<PodcastUiActions> m_uiActions;
};
}

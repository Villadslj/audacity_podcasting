/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore BVBA and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "podcastmodule.h"

#include <QtQml>

#include "framework/global/modularity/ioc.h"

#include "framework/interactive/iinteractiveuriregister.h"
#include "framework/ui/iuiactionsregister.h"

#include "internal/podcastactionscontroller.h"
#include "internal/podcastconfiguration.h"
#include "internal/podcasteditservice.h"
#include "internal/podcastuiactions.h"
#include "internal/speechdetector.h"

#include "view/podcastsettingsmodel.h"

using namespace au::podcast;
using namespace muse;
using namespace muse::modularity;

static const std::string mname("podcast");

PodcastModule::PodcastModule()
{
}

static void podcast_init_qrc()
{
    Q_INIT_RESOURCE(podcast);
}

std::string PodcastModule::moduleName() const
{
    return mname;
}

void PodcastModule::registerExports()
{
    m_configuration = std::make_shared<PodcastConfiguration>();

    globalIoc()->registerExport<IPodcastConfiguration>(mname, m_configuration);
}

void PodcastModule::registerUiTypes()
{
    qmlRegisterType<PodcastSettingsModel>("Audacity.Podcast", 1, 0, "PodcastSettingsModel");
}

void PodcastModule::resolveImports()
{
    auto ir = globalIoc()->resolve<muse::interactive::IInteractiveUriRegister>(mname);
    if (ir) {
        ir->registerQmlUri(muse::Uri("audacity://podcast/settings"), "Audacity/Podcast/PodcastSettingsDialog.qml");
    }
}

void PodcastModule::registerResources()
{
    podcast_init_qrc();
}

void PodcastModule::onInit(const muse::IApplication::RunMode&)
{
    m_configuration->init();
}

IContextSetup* PodcastModule::newContext(const muse::modularity::ContextPtr& ctx) const
{
    return new PodcastContext(ctx);
}

// =====================================================
// PodcastContext
// =====================================================

void PodcastContext::registerExports()
{
    m_actionsController = std::make_shared<PodcastActionsController>(iocContext());
    m_uiActions = std::make_shared<PodcastUiActions>(iocContext(), m_actionsController);

    ioc()->registerExport<ISpeechDetector>(mname, new SpeechDetector(iocContext()));
    ioc()->registerExport<IPodcastEditService>(mname, new PodcastEditService(iocContext()));
}

void PodcastContext::onInit(const muse::IApplication::RunMode&)
{
    m_actionsController->init();
    m_uiActions->init();

    auto ar = ioc()->resolve<muse::ui::IUiActionsRegister>(mname);
    if (ar) {
        ar->reg(m_uiActions);
    }
}

void PodcastContext::onDeinit()
{
}

/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include "global/async/notification.h"
#include "global/modularity/imoduleinterface.h"

#include "podcasttypes.h"

namespace au::podcast {
class IPodcastConfiguration : MODULE_EXPORT_INTERFACE
{
    INTERFACE_ID(au::podcast::IPodcastConfiguration)

public:
    virtual ~IPodcastConfiguration() = default;

    virtual void init() = 0;

    virtual SpeechDetectionSettings speechDetectionSettings() const = 0;
    virtual void setSpeechDetectionSettings(const SpeechDetectionSettings& settings) = 0;

    virtual TightenSettings tightenSettings() const = 0;
    virtual void setTightenSettings(const TightenSettings& settings) = 0;

    //! Per-track overrides, keyed by track title, like audiome's `strip_config.json`
    virtual SpeechDetectionOverrides speechDetectionOverrides() const = 0;
    virtual void setSpeechDetectionOverrides(const SpeechDetectionOverrides& overrides) = 0;

    virtual muse::async::Notification settingsChanged() const = 0;
};
}

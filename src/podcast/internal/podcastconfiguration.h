/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include "../ipodcastconfiguration.h"

namespace au::podcast {
class PodcastConfiguration : public IPodcastConfiguration
{
public:
    PodcastConfiguration() = default;

    void init() override;

    SpeechDetectionSettings speechDetectionSettings() const override;
    void setSpeechDetectionSettings(const SpeechDetectionSettings& settings) override;

    TightenSettings tightenSettings() const override;
    void setTightenSettings(const TightenSettings& settings) override;

    SpeechDetectionOverrides speechDetectionOverrides() const override;
    void setSpeechDetectionOverrides(const SpeechDetectionOverrides& overrides) override;

    muse::async::Notification settingsChanged() const override;

private:
    muse::async::Notification m_settingsChanged;
};
}

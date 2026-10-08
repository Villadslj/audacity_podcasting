/*
* Audacity: A Digital Audio Editor
*/
#include "podcastconfiguration.h"

#include "serialization/json.h"
#include "settings.h"
#include "types/bytearray.h"

namespace au::podcast {
static const std::string moduleName("podcast");

static const muse::Settings::Key FRAME_MS(moduleName, "podcast/frameMs");
static const muse::Settings::Key OPEN_DB(moduleName, "podcast/openDb");
static const muse::Settings::Key CLOSE_DB(moduleName, "podcast/closeDb");
static const muse::Settings::Key HANGOVER_MS(moduleName, "podcast/hangoverMs");
static const muse::Settings::Key MIN_SPEECH_MS(moduleName, "podcast/minSpeechMs");
static const muse::Settings::Key MIN_SILENCE_MS(moduleName, "podcast/minSilenceMs");
static const muse::Settings::Key MARGIN_MS(moduleName, "podcast/marginMs");
static const muse::Settings::Key BLEED_REJECTION(moduleName, "podcast/bleedRejection");
static const muse::Settings::Key BLEED_MARGIN_DB(moduleName, "podcast/bleedMarginDb");
static const muse::Settings::Key MIN_GAP_MS(moduleName, "podcast/minGapMs");
static const muse::Settings::Key TIGHTEN_PERCENT(moduleName, "podcast/tightenPercent");
static const muse::Settings::Key TRACK_OVERRIDES(moduleName, "podcast/trackOverrides");

static const std::string KEY_OPEN_DB("openDb");
static const std::string KEY_CLOSE_DB("closeDb");
static const std::string KEY_HANGOVER_MS("hangoverMs");
static const std::string KEY_MIN_SPEECH_MS("minSpeechMs");
static const std::string KEY_MIN_SILENCE_MS("minSilenceMs");
static const std::string KEY_MARGIN_MS("marginMs");
static const std::string KEY_BLEED_REJECTION("bleedRejection");
static const std::string KEY_BLEED_MARGIN_DB("bleedMarginDb");

void PodcastConfiguration::init()
{
    const SpeechDetectionSettings defaults;
    const TightenSettings tightenDefaults;

    const std::vector<std::pair<muse::Settings::Key, muse::Val> > keys {
        { FRAME_MS, muse::Val(defaults.frameMs) },
        { OPEN_DB, muse::Val(defaults.openDb) },
        { CLOSE_DB, muse::Val(defaults.closeDb) },
        { HANGOVER_MS, muse::Val(defaults.hangoverMs) },
        { MIN_SPEECH_MS, muse::Val(defaults.minSpeechMs) },
        { MIN_SILENCE_MS, muse::Val(defaults.minSilenceMs) },
        { MARGIN_MS, muse::Val(defaults.marginMs) },
        { BLEED_REJECTION, muse::Val(defaults.bleedRejection) },
        { BLEED_MARGIN_DB, muse::Val(defaults.bleedMarginDb) },
        { MIN_GAP_MS, muse::Val(tightenDefaults.minGapMs) },
        { TIGHTEN_PERCENT, muse::Val(tightenDefaults.percent) },
        { TRACK_OVERRIDES, muse::Val(std::string("{}")) }
    };

    for (const auto& key : keys) {
        muse::settings()->setDefaultValue(key.first, key.second);
        muse::settings()->valueChanged(key.first).onReceive(nullptr, [this](const muse::Val&) {
            m_settingsChanged.notify();
        });
    }
}

SpeechDetectionSettings PodcastConfiguration::speechDetectionSettings() const
{
    SpeechDetectionSettings settings;
    settings.frameMs = muse::settings()->value(FRAME_MS).toDouble();
    settings.openDb = muse::settings()->value(OPEN_DB).toDouble();
    settings.closeDb = muse::settings()->value(CLOSE_DB).toDouble();
    settings.hangoverMs = muse::settings()->value(HANGOVER_MS).toDouble();
    settings.minSpeechMs = muse::settings()->value(MIN_SPEECH_MS).toDouble();
    settings.minSilenceMs = muse::settings()->value(MIN_SILENCE_MS).toDouble();
    settings.marginMs = muse::settings()->value(MARGIN_MS).toDouble();
    settings.bleedRejection = muse::settings()->value(BLEED_REJECTION).toBool();
    settings.bleedMarginDb = muse::settings()->value(BLEED_MARGIN_DB).toDouble();
    return settings;
}

void PodcastConfiguration::setSpeechDetectionSettings(const SpeechDetectionSettings& settings)
{
    muse::settings()->setSharedValue(FRAME_MS, muse::Val(settings.frameMs));
    muse::settings()->setSharedValue(OPEN_DB, muse::Val(settings.openDb));
    muse::settings()->setSharedValue(CLOSE_DB, muse::Val(settings.closeDb));
    muse::settings()->setSharedValue(HANGOVER_MS, muse::Val(settings.hangoverMs));
    muse::settings()->setSharedValue(MIN_SPEECH_MS, muse::Val(settings.minSpeechMs));
    muse::settings()->setSharedValue(MIN_SILENCE_MS, muse::Val(settings.minSilenceMs));
    muse::settings()->setSharedValue(MARGIN_MS, muse::Val(settings.marginMs));
    muse::settings()->setSharedValue(BLEED_REJECTION, muse::Val(settings.bleedRejection));
    muse::settings()->setSharedValue(BLEED_MARGIN_DB, muse::Val(settings.bleedMarginDb));
}

TightenSettings PodcastConfiguration::tightenSettings() const
{
    TightenSettings settings;
    settings.minGapMs = muse::settings()->value(MIN_GAP_MS).toDouble();
    settings.percent = muse::settings()->value(TIGHTEN_PERCENT).toDouble();
    return settings;
}

void PodcastConfiguration::setTightenSettings(const TightenSettings& settings)
{
    muse::settings()->setSharedValue(MIN_GAP_MS, muse::Val(settings.minGapMs));
    muse::settings()->setSharedValue(TIGHTEN_PERCENT, muse::Val(settings.percent));
}

SpeechDetectionOverrides PodcastConfiguration::speechDetectionOverrides() const
{
    SpeechDetectionOverrides overrides;

    const std::string raw = muse::settings()->value(TRACK_OVERRIDES).toString();
    if (raw.empty()) {
        return overrides;
    }

    const muse::JsonDocument doc = muse::JsonDocument::fromJson(muse::ByteArray::fromRawData(raw.data(), raw.size()));
    if (!doc.isObject()) {
        return overrides;
    }

    const SpeechDetectionSettings globalSettings = speechDetectionSettings();

    const muse::JsonObject root = doc.rootObject();
    for (const std::string& title : root.keys()) {
        const muse::JsonObject object = root.value(title).toObject();
        if (!object.isValid()) {
            continue;
        }

        SpeechDetectionSettings settings = globalSettings;
        if (object.contains(KEY_OPEN_DB)) {
            settings.openDb = object.value(KEY_OPEN_DB).toDouble();
        }
        if (object.contains(KEY_CLOSE_DB)) {
            settings.closeDb = object.value(KEY_CLOSE_DB).toDouble();
        }
        if (object.contains(KEY_HANGOVER_MS)) {
            settings.hangoverMs = object.value(KEY_HANGOVER_MS).toDouble();
        }
        if (object.contains(KEY_MIN_SPEECH_MS)) {
            settings.minSpeechMs = object.value(KEY_MIN_SPEECH_MS).toDouble();
        }
        if (object.contains(KEY_MIN_SILENCE_MS)) {
            settings.minSilenceMs = object.value(KEY_MIN_SILENCE_MS).toDouble();
        }
        if (object.contains(KEY_MARGIN_MS)) {
            settings.marginMs = object.value(KEY_MARGIN_MS).toDouble();
        }
        if (object.contains(KEY_BLEED_REJECTION)) {
            settings.bleedRejection = object.value(KEY_BLEED_REJECTION).toBool();
        }
        if (object.contains(KEY_BLEED_MARGIN_DB)) {
            settings.bleedMarginDb = object.value(KEY_BLEED_MARGIN_DB).toDouble();
        }

        overrides.emplace(title, settings);
    }

    return overrides;
}

void PodcastConfiguration::setSpeechDetectionOverrides(const SpeechDetectionOverrides& overrides)
{
    muse::JsonObject root;
    for (const auto& it : overrides) {
        muse::JsonObject object;
        object.set(KEY_OPEN_DB, it.second.openDb);
        object.set(KEY_CLOSE_DB, it.second.closeDb);
        object.set(KEY_HANGOVER_MS, it.second.hangoverMs);
        object.set(KEY_MIN_SPEECH_MS, it.second.minSpeechMs);
        object.set(KEY_MIN_SILENCE_MS, it.second.minSilenceMs);
        object.set(KEY_MARGIN_MS, it.second.marginMs);
        object.set(KEY_BLEED_REJECTION, it.second.bleedRejection);
        object.set(KEY_BLEED_MARGIN_DB, it.second.bleedMarginDb);

        root.set(it.first, object);
    }

    const muse::ByteArray data = muse::JsonDocument(root).toJson(muse::JsonDocument::Format::Compact);
    muse::settings()->setSharedValue(TRACK_OVERRIDES, muse::Val(std::string(data.constChar(), data.size())));
}

muse::async::Notification PodcastConfiguration::settingsChanged() const
{
    return m_settingsChanged;
}
}

/*
* Audacity: A Digital Audio Editor
*/
#include "podcastsettingsmodel.h"

#include <QVariantMap>

#include <algorithm>

#include "realfn.h"

#include "trackedit/itrackeditproject.h"

using namespace au::podcast;

namespace {
const QString TITLE_KEY("title");
const QString OVERRIDDEN_KEY("overridden");
const QString OPEN_DB_KEY("openDb");
const QString CLOSE_DB_KEY("closeDb");
const QString BLEED_REJECTION_KEY("bleedRejection");
const QString BLEED_MARGIN_DB_KEY("bleedMarginDb");

QVariantMap makeEntry(const QString& title, bool overridden, const SpeechDetectionSettings& settings)
{
    QVariantMap entry;
    entry[TITLE_KEY] = title;
    entry[OVERRIDDEN_KEY] = overridden;
    entry[OPEN_DB_KEY] = settings.openDb;
    entry[CLOSE_DB_KEY] = settings.closeDb;
    entry[BLEED_REJECTION_KEY] = settings.bleedRejection;
    entry[BLEED_MARGIN_DB_KEY] = settings.bleedMarginDb;
    return entry;
}
}

PodcastSettingsModel::PodcastSettingsModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

void PodcastSettingsModel::load()
{
    m_settings = configuration()->speechDetectionSettings();
    m_tightenSettings = configuration()->tightenSettings();

    const SpeechDetectionOverrides overrides = configuration()->speechDetectionOverrides();

    m_trackOverrides.clear();

    std::vector<std::string> handledTitles;

    const auto project = globalContext()->currentTrackeditProject();
    if (project) {
        for (const trackedit::Track& track : project->trackList()) {
            if (track.type != trackedit::TrackType::Mono && track.type != trackedit::TrackType::Stereo) {
                continue;
            }

            const std::string title = track.title.toStdString();
            const auto it = overrides.find(title);
            const bool overridden = it != overrides.end();

            m_trackOverrides.append(makeEntry(QString::fromStdString(title), overridden,
                                              overridden ? it->second : m_settings));
            handledTitles.push_back(title);
        }
    }

    for (const auto& it : overrides) {
        if (std::find(handledTitles.begin(), handledTitles.end(), it.first) != handledTitles.end()) {
            continue;
        }
        m_trackOverrides.append(makeEntry(QString::fromStdString(it.first), true, it.second));
    }

    emit settingsChanged();
    emit trackOverridesChanged();
}

void PodcastSettingsModel::apply()
{
    configuration()->setSpeechDetectionSettings(m_settings);
    configuration()->setTightenSettings(m_tightenSettings);

    SpeechDetectionOverrides overrides;
    for (const QVariant& value : m_trackOverrides) {
        const QVariantMap entry = value.toMap();
        if (!entry.value(OVERRIDDEN_KEY).toBool()) {
            continue;
        }

        SpeechDetectionSettings settings = m_settings;
        settings.openDb = entry.value(OPEN_DB_KEY, m_settings.openDb).toDouble();
        settings.closeDb = entry.value(CLOSE_DB_KEY, m_settings.closeDb).toDouble();
        settings.bleedRejection = entry.value(BLEED_REJECTION_KEY, m_settings.bleedRejection).toBool();
        settings.bleedMarginDb = entry.value(BLEED_MARGIN_DB_KEY, m_settings.bleedMarginDb).toDouble();

        overrides.emplace(entry.value(TITLE_KEY).toString().toStdString(), settings);
    }

    configuration()->setSpeechDetectionOverrides(overrides);
}

void PodcastSettingsModel::setTrackOverrideValue(int index, const QString& key, const QVariant& value)
{
    if (index < 0 || index >= m_trackOverrides.size()) {
        return;
    }

    QVariantMap entry = m_trackOverrides.at(index).toMap();
    if (entry.value(key) == value) {
        return;
    }

    entry[key] = value;
    m_trackOverrides[index] = entry;

    emit trackOverridesChanged();
}

double PodcastSettingsModel::frameMs() const { return m_settings.frameMs; }
double PodcastSettingsModel::openDb() const { return m_settings.openDb; }
double PodcastSettingsModel::closeDb() const { return m_settings.closeDb; }
double PodcastSettingsModel::hangoverMs() const { return m_settings.hangoverMs; }
double PodcastSettingsModel::minSpeechMs() const { return m_settings.minSpeechMs; }
double PodcastSettingsModel::minSilenceMs() const { return m_settings.minSilenceMs; }
double PodcastSettingsModel::marginMs() const { return m_settings.marginMs; }
bool PodcastSettingsModel::bleedRejection() const { return m_settings.bleedRejection; }
double PodcastSettingsModel::bleedMarginDb() const { return m_settings.bleedMarginDb; }
double PodcastSettingsModel::minGapMs() const { return m_tightenSettings.minGapMs; }
double PodcastSettingsModel::tightenPercent() const { return m_tightenSettings.percent; }
QVariantList PodcastSettingsModel::trackOverrides() const { return m_trackOverrides; }

void PodcastSettingsModel::setFrameMs(double value)
{
    if (muse::RealIsEqual(m_settings.frameMs, value)) {
        return;
    }
    m_settings.frameMs = value;
    emit settingsChanged();
}

void PodcastSettingsModel::setOpenDb(double value)
{
    if (muse::RealIsEqual(m_settings.openDb, value)) {
        return;
    }
    m_settings.openDb = value;
    emit settingsChanged();
}

void PodcastSettingsModel::setCloseDb(double value)
{
    if (muse::RealIsEqual(m_settings.closeDb, value)) {
        return;
    }
    m_settings.closeDb = value;
    emit settingsChanged();
}

void PodcastSettingsModel::setHangoverMs(double value)
{
    if (muse::RealIsEqual(m_settings.hangoverMs, value)) {
        return;
    }
    m_settings.hangoverMs = value;
    emit settingsChanged();
}

void PodcastSettingsModel::setMinSpeechMs(double value)
{
    if (muse::RealIsEqual(m_settings.minSpeechMs, value)) {
        return;
    }
    m_settings.minSpeechMs = value;
    emit settingsChanged();
}

void PodcastSettingsModel::setMinSilenceMs(double value)
{
    if (muse::RealIsEqual(m_settings.minSilenceMs, value)) {
        return;
    }
    m_settings.minSilenceMs = value;
    emit settingsChanged();
}

void PodcastSettingsModel::setMarginMs(double value)
{
    if (muse::RealIsEqual(m_settings.marginMs, value)) {
        return;
    }
    m_settings.marginMs = value;
    emit settingsChanged();
}

void PodcastSettingsModel::setBleedRejection(bool value)
{
    if (m_settings.bleedRejection == value) {
        return;
    }
    m_settings.bleedRejection = value;
    emit settingsChanged();
}

void PodcastSettingsModel::setBleedMarginDb(double value)
{
    if (muse::RealIsEqual(m_settings.bleedMarginDb, value)) {
        return;
    }
    m_settings.bleedMarginDb = value;
    emit settingsChanged();
}

void PodcastSettingsModel::setMinGapMs(double value)
{
    if (muse::RealIsEqual(m_tightenSettings.minGapMs, value)) {
        return;
    }
    m_tightenSettings.minGapMs = value;
    emit settingsChanged();
}

void PodcastSettingsModel::setTightenPercent(double value)
{
    if (muse::RealIsEqual(m_tightenSettings.percent, value)) {
        return;
    }
    m_tightenSettings.percent = value;
    emit settingsChanged();
}

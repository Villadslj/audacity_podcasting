/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <QObject>
#include <QVariantList>

#include "global/async/asyncable.h"
#include "global/modularity/ioc.h"

#include "context/iglobalcontext.h"

#include "../ipodcastconfiguration.h"

namespace au::podcast {
class PodcastSettingsModel : public QObject, public muse::async::Asyncable, public muse::Contextable
{
    Q_OBJECT

    muse::GlobalInject<IPodcastConfiguration> configuration;
    muse::ContextInject<au::context::IGlobalContext> globalContext { this };

    Q_PROPERTY(double frameMs READ frameMs WRITE setFrameMs NOTIFY settingsChanged)
    Q_PROPERTY(double openDb READ openDb WRITE setOpenDb NOTIFY settingsChanged)
    Q_PROPERTY(double closeDb READ closeDb WRITE setCloseDb NOTIFY settingsChanged)
    Q_PROPERTY(double hangoverMs READ hangoverMs WRITE setHangoverMs NOTIFY settingsChanged)
    Q_PROPERTY(double minSpeechMs READ minSpeechMs WRITE setMinSpeechMs NOTIFY settingsChanged)
    Q_PROPERTY(double minSilenceMs READ minSilenceMs WRITE setMinSilenceMs NOTIFY settingsChanged)
    Q_PROPERTY(double marginMs READ marginMs WRITE setMarginMs NOTIFY settingsChanged)
    Q_PROPERTY(bool bleedRejection READ bleedRejection WRITE setBleedRejection NOTIFY settingsChanged)
    Q_PROPERTY(double bleedMarginDb READ bleedMarginDb WRITE setBleedMarginDb NOTIFY settingsChanged)
    Q_PROPERTY(double minGapMs READ minGapMs WRITE setMinGapMs NOTIFY settingsChanged)
    Q_PROPERTY(double tightenPercent READ tightenPercent WRITE setTightenPercent NOTIFY settingsChanged)

    //! NOTE One entry per audio track of the project, plus the overrides that are
    //! configured for titles that the project does not have (any more).
    //! Keys: "title", "overridden", "openDb", "closeDb", "bleedRejection", "bleedMarginDb"
    Q_PROPERTY(QVariantList trackOverrides READ trackOverrides NOTIFY trackOverridesChanged)

public:
    explicit PodcastSettingsModel(QObject* parent = nullptr);

    Q_INVOKABLE void load();
    Q_INVOKABLE void apply();
    Q_INVOKABLE void setTrackOverrideValue(int index, const QString& key, const QVariant& value);

    double frameMs() const;
    double openDb() const;
    double closeDb() const;
    double hangoverMs() const;
    double minSpeechMs() const;
    double minSilenceMs() const;
    double marginMs() const;
    bool bleedRejection() const;
    double bleedMarginDb() const;
    double minGapMs() const;
    double tightenPercent() const;
    QVariantList trackOverrides() const;

    void setFrameMs(double value);
    void setOpenDb(double value);
    void setCloseDb(double value);
    void setHangoverMs(double value);
    void setMinSpeechMs(double value);
    void setMinSilenceMs(double value);
    void setMarginMs(double value);
    void setBleedRejection(bool value);
    void setBleedMarginDb(double value);
    void setMinGapMs(double value);
    void setTightenPercent(double value);

signals:
    void settingsChanged();
    void trackOverridesChanged();

private:
    SpeechDetectionSettings m_settings;
    TightenSettings m_tightenSettings;
    QVariantList m_trackOverrides;
};
}

#ifndef DLTANALYZERINTERFACE_H
#define DLTANALYZERINTERFACE_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QList>
#include <QMap>
#include <QVariantMap>

#define DLT_ANALYZER_INTERFACE_VERSION "1.0.0"

class DltAnalyzerInterface
{
public:
    struct LogEntry
    {
        int index = -1;
        QString time;
        QString timestamp;
        QString ecu;
        QString apid;
        QString ctid;
        QString level;
        QString payload;
    };

    struct QueryResult
    {
        QString responseHtml;
        QList<int> indices;
        QStringList snippets;
        bool success = false;
        QString errorMessage;
        qint64 processingTimeMs = 0;
        bool usedAi = false;
    };

    virtual ~DltAnalyzerInterface() = default;

    virtual QString name() const = 0;
    virtual QString version() const = 0;
    virtual QString interfaceVersion() const = 0;

    virtual bool isAvailable() const = 0;
    virtual bool supportsStreaming() const = 0;

    virtual QueryResult analyzeQuery(const QString &query,
                                     const QVector<LogEntry> &entries) = 0;

    virtual QString configurationInfo() const = 0;
    virtual QStringList supportedLanguages() const = 0;

    virtual bool configure(const QVariantMap &config) = 0;
    virtual QVariantMap currentConfiguration() const = 0;

    static QString interfaceId() { return "org.genivi.DLT.DltAnalyzerInterface"; }
};

#define DltAnalyzerInterface_iid "org.genivi.DLT.DltAnalyzerInterface"

Q_DECLARE_INTERFACE(DltAnalyzerInterface, DltAnalyzerInterface_iid)

class DltRuleBasedAnalyzer : public DltAnalyzerInterface
{
public:
    DltRuleBasedAnalyzer();
    ~DltRuleBasedAnalyzer() override = default;

    QString name() const override { return "Rule-Based Analyzer"; }
    QString version() const override { return "1.0.0"; }
    QString interfaceVersion() const override { return DLT_ANALYZER_INTERFACE_VERSION; }

    bool isAvailable() const override { return true; }
    bool supportsStreaming() const override { return false; }

    QueryResult analyzeQuery(const QString &query,
                            const QVector<LogEntry> &entries) override;

    QString configurationInfo() const override;
    QStringList supportedLanguages() const override;

    bool configure(const QVariantMap &config) override;
    QVariantMap currentConfiguration() const override;

    static QString simplifyPayload(const QString &payload);

    static QString formatEntryLine(const LogEntry &entry);

private:
    QueryResult analyzeInternal(const QString &query,
                                const QVector<LogEntry> &entries) const;
    QString buildSummaryHtml(const QVector<LogEntry> &entries) const;

    QStringList supportedLanguagesList;
};

#endif

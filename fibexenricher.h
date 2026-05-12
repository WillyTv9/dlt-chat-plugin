#ifndef FIBEXENRICHER_H
#define FIBEXENRICHER_H

#include "dltanalyzerinterface.h"
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

class FibexEnricher
{
public:
    struct EnrichmentEntry
    {
        QString functionName;
        QString description;
        QStringList signalNames;
        bool valid = false;
    };

    FibexEnricher();

    bool loadFile(const QString &filePath, QString *errorOut = nullptr);
    void clear();
    bool isLoaded() const;
    QStringList loadedFiles() const;

    QString enrich(const QString &apid, const QString &ctid) const;
    void enrichEntry(DltAnalyzerInterface::LogEntry &entry) const;
    void enrichAll(QVector<DltAnalyzerInterface::LogEntry> &entries) const;

    int mappingCount() const { return m_enrichmentMap.size(); }

private:
    void parseFibexFile(const QString &path);
    void parseGenericXmlFile(const QString &path, QString *errorOut);

    QHash<QString, EnrichmentEntry> m_enrichmentMap;
    QStringList m_loadedFiles;
    QStringList m_failedFiles;
};

#endif

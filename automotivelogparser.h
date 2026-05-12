#ifndef AUTOMOTIVELOGPARSER_H
#define AUTOMOTIVELOGPARSER_H

#include "dltanalyzerinterface.h"
#include <QHash>
#include <QPair>
#include <QRegularExpression>
#include <QStringList>
#include <QVector>

class AutomotiveLogParser
{
public:
    static void classify(DltAnalyzerInterface::LogEntry &entry);

    static QVector<DltAnalyzerInterface::LogEntry>
    filterByPreset(const QVector<DltAnalyzerInterface::LogEntry> &entries,
                   const QString &presetName,
                   int maxResults = 200);

    static QStringList availablePresets();

    static QPair<int, int> domainStats(
        const QVector<DltAnalyzerInterface::LogEntry> &entries);

private:
    using PresetRules = QHash<QString, QVector<QPair<QString, QRegularExpression>>>;
    static const PresetRules &presetRules();
};

#endif

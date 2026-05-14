#ifndef DLTCHAT_AUTOMOTIVE_LOG_PARSER_H
#define DLTCHAT_AUTOMOTIVE_LOG_PARSER_H

#include "analyzer_interface.h"
#include <QHash>
#include <QPair>
#include <QRegularExpression>
#include <QStringList>
#include <QVector>

namespace dltchat {

class AutomotiveLogParser
{
public:
    static void classify(DltAnalyzerInterface::LogEntry &entry);
    static QVector<DltAnalyzerInterface::LogEntry> filterByPreset(
        const QVector<DltAnalyzerInterface::LogEntry> &entries,
        const QString &presetName);
    static QPair<int, int> domainStats(const QVector<DltAnalyzerInterface::LogEntry> &entries);
    static QHash<QString, QStringList> availablePresets();
};

} // namespace dltchat

#endif

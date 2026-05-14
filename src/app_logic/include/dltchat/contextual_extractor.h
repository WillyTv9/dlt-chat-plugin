#ifndef DLTCHAT_CONTEXTUAL_EXTRACTOR_H
#define DLTCHAT_CONTEXTUAL_EXTRACTOR_H

#include "analyzer_interface.h"
#include <QHash>
#include <QSet>
#include <QString>
#include <QVector>

namespace dltchat {

class ContextualExtractor
{
public:
    struct ContextConfig {
        int windowBefore;
        int windowAfter;
        int maxEntries;
        bool preferDomain;

        ContextConfig()
            : windowBefore(5)
            , windowAfter(3)
            , maxEntries(100)
            , preferDomain(true)
        {}
    };

    QVector<DltAnalyzerInterface::LogEntry> extractContext(
        const QString &query,
        const QVector<DltAnalyzerInterface::LogEntry> &entries,
        const QHash<QString, QSet<int>> &invertedIndex,
        const QList<int> &selectedIndices = {},
        const ContextConfig &config = ContextConfig()) const;

private:
    QList<int> findRelevantIndices(
        const QString &query,
        const QVector<DltAnalyzerInterface::LogEntry> &entries,
        const QHash<QString, QSet<int>> &invertedIndex,
        const ContextConfig &config) const;
};

} // namespace dltchat

#endif

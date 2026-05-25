#ifndef DLTCHAT_ENHANCED_RETRIEVER_H
#define DLTCHAT_ENHANCED_RETRIEVER_H

#include "analyzer_interface.h"

#include <QHash>
#include <QList>
#include <QSet>
#include <QString>
#include <QVector>

namespace dltchat {

/**
 * Retrieves the most relevant subset of log entries for an AI query.
 * Uses four cheap, in-memory signals — no embeddings, no external deps:
 *
 *   score = wTfIdf      * tf-idf(query, entry)
 *         + wRecency    * recency_decay(entry)
 *         + wCorrelation* temporal_proximity_to_other_hits(entry)
 *         - wDiversityPenalty * already_seen_category_or_apid(entry)
 *
 * IDF uses the inverted index (per-term df) already maintained by
 * LogStore. MMR-style diversity penalty prevents the picked set from
 * being dominated by repetitive payloads (heartbeats, periodic
 * notifications) — common in automotive logs.
 *
 * The extracted set is post-expanded with a ±window of contiguous
 * neighbours around each hit to preserve causal context (same idea
 * as ContextualExtractor, just on a smarter seed set).
 *
 * Drop-in replacement for ContextualExtractor with a superset config.
 */
class EnhancedRetriever
{
public:
    struct Config
    {
        int windowBefore = 5;
        int windowAfter = 5;
        int maxEntries = 200;
        int maxSeedHits = 10000;
        int numStrata = 10;
        int maxSeedsPerStratum = 500;
        double wTfIdf = 1.0;
        double wRecency = 0.2;
        double wCorrelation = 0.3;
        double wDiversityPenalty = 0.4;
        bool preferDomain = true;
        qint64 correlationWindowMs = 200;
    };

    struct Result
    {
        QVector<DltAnalyzerInterface::LogEntry> entries;
        QList<int> seedIndices;
        QString diagnostic;
    };

    // GCC rejects `= Config()` as a default arg here because Config is a
    // nested struct of the same enclosing class and its NSDMIs aren't yet
    // visible when the default arg is parsed. Use a static factory instead;
    // it's only instantiated when the default arg fires, which sidesteps
    // the completeness check.
    static const Config &defaultConfig();

    Result extract(const QString &query,
                   const QVector<DltAnalyzerInterface::LogEntry> &entries,
                   const QHash<QString, QSet<int>> &invertedIndex,
                   const QList<int> &selectedIndices = {},
                   const Config &config = defaultConfig()) const;
};

} // namespace dltchat

#endif

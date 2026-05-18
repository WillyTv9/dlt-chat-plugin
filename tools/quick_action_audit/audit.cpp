#include "audit.h"
#include <QElapsedTimer>
#include <QDebug>
#include <QMap>
#include <algorithm>
#include "dltchat/category_registry.h"
#include "dltchat/analyzer_interface.h"

using namespace dltchat;

static QString kindStr(ResolvedQuery::Kind k)
{
    switch (k) {
    case ResolvedQuery::Kind::Category:        return QStringLiteral("Category");
    case ResolvedQuery::Kind::CategoryById:    return QStringLiteral("CategoryById");
    case ResolvedQuery::Kind::CombinedFilter:  return QStringLiteral("CombinedFilter");
    case ResolvedQuery::Kind::ProjectionEvent: return QStringLiteral("ProjectionEvent");
    case ResolvedQuery::Kind::SpecialCommand:  return QStringLiteral("SpecialCommand");
    case ResolvedQuery::Kind::Unknown:         return QStringLiteral("UNKNOWN");
    }
    return QStringLiteral("?");
}

static QString resolvedTarget(const ResolvedQuery &r)
{
    switch (r.kind) {
    case ResolvedQuery::Kind::Category:
    case ResolvedQuery::Kind::CategoryById:    return r.categoryId;
    case ResolvedQuery::Kind::CombinedFilter:  return r.combinedId;
    case ResolvedQuery::Kind::ProjectionEvent: return r.projectionEventId;
    case ResolvedQuery::Kind::SpecialCommand:  return r.specialCommand;
    default: return QStringLiteral("(none)");
    }
}

QVector<AuditRow> runQuickActions(
    const QVector<DltAnalyzerInterface::LogEntry> &entries)
{
    const CategoryRegistry &reg = CategoryRegistry::instance();
    DltRuleBasedAnalyzer ruleAnalyzer;
    QVector<AuditRow> rows;
    rows.reserve(reg.quickActions().size());

    for (const auto &qa : reg.quickActions()) {
        AuditRow row;
        row.label = qa.label;
        row.query = qa.query;

        const ResolvedQuery resolved = reg.resolveQuery(qa.query.toLower());
        row.kind   = kindStr(resolved.kind);
        row.target = resolvedTarget(resolved);

        QElapsedTimer t;
        t.start();

        switch (resolved.kind) {
        case ResolvedQuery::Kind::Unknown:
            row.isError = true;
            row.note    = QStringLiteral("Query did not resolve — button is broken");
            break;

        case ResolvedQuery::Kind::SpecialCommand:
            if (resolved.specialCommand == QLatin1String("timeline")) {
                row.matchCount = static_cast<int>(entries.size());
                for (int k = 0; k < std::min(3, static_cast<int>(entries.size())); ++k)
                    row.samples.append(entries[k].payload.left(80));
            } else {
                auto result    = ruleAnalyzer.analyzeQuery(qa.query, entries);
                row.matchCount = result.indices.size();
                row.samples    = result.snippets.mid(0, 3);
                if (!result.success && !result.errorMessage.isEmpty())
                    row.note = result.errorMessage;
            }
            break;

        default: {
            auto matched   = reg.filterEntries(entries, resolved);
            row.matchCount = static_cast<int>(matched.size());
            for (int k = 0; k < std::min(3, static_cast<int>(matched.size())); ++k)
                row.samples.append(matched[k].payload.left(80));
            break;
        }
        }

        row.timeMs     = t.elapsed();
        row.pctOfTotal = entries.isEmpty() ? 0.0
                       : 100.0 * row.matchCount / static_cast<double>(entries.size());

        qInfo().noquote()
            << QString("  [%1] %2 -> %3 %4 | %5 matches (%6%) | %7 ms")
                   .arg(row.label, -14)
                   .arg(row.query, -20)
                   .arg(row.kind, -15)
                   .arg(row.target, -30)
                   .arg(row.matchCount, 7)
                   .arg(QString::number(row.pctOfTotal, 'f', 2), 6)
                   .arg(row.timeMs, 5);

        rows.append(row);
    }
    return rows;
}

QVector<Collision> findAliasCollisions()
{
    const CategoryRegistry &reg = CategoryRegistry::instance();
    QMap<QString, QStringList> aliasMap;
    for (const auto &cat : reg.categories()) {
        for (const auto &a : cat.aliases) {
            const QString key = a.toLower();
            if (!aliasMap[key].contains(cat.id))
                aliasMap[key].append(cat.id);
        }
    }
    QVector<Collision> result;
    for (auto it = aliasMap.constBegin(); it != aliasMap.constEnd(); ++it) {
        if (it.value().size() < 2)
            continue;
        Collision c;
        c.alias   = it.key();
        c.allCats = it.value();
        c.winner  = resolvedTarget(reg.resolveQuery(it.key()));
        result.append(c);
    }
    return result;
}

static bool isAmbiguousToken(const QString &f)
{
    if (f.length() <= 2)
        return true;
    static const QStringList common = {
        "can", "bus", "ip", "ui", "net", "mac", "lan", "fm", "bt", "ux",
        "x11", "vpn", "a2dp", "hfp", "rds", "obd", "sid", "dtc", "did",
        "dlt", "asr", "the", "and", "not", "are", "was", "for", "ocp"
    };
    return common.contains(f.toLower());
}

QVector<ShortFilter> findAmbiguousFilters()
{
    QVector<ShortFilter> result;
    for (const auto &cat : CategoryRegistry::instance().categories()) {
        for (const auto &f : cat.filters) {
            if (isAmbiguousToken(f))
                result.append({cat.id, f});
        }
    }
    return result;
}

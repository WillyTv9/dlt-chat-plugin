#include "dltchat/automotive_log_parser.h"
#include "dltchat/category_registry.h"

#include <algorithm>

namespace dltchat {

void AutomotiveLogParser::classify(DltAnalyzerInterface::LogEntry &entry)
{
    CategoryRegistry::instance().classifyEntry(entry);
    if (entry.domain.isEmpty())
        entry.domain = QStringLiteral("generic");
}

QVector<DltAnalyzerInterface::LogEntry> AutomotiveLogParser::filterByPreset(
    const QVector<DltAnalyzerInterface::LogEntry> &entries,
    const QString &presetName)
{
    auto resolved = CategoryRegistry::instance().resolveQuery(presetName);
    if (resolved.kind == ResolvedQuery::Kind::Unknown) {
        resolved.kind = ResolvedQuery::Kind::Category;
        resolved.categoryId = presetName.toUpper();
        resolved.categoryIds = QStringList{resolved.categoryId};
    }
    return CategoryRegistry::instance().filterEntries(entries, resolved);
}

QPair<int, int> AutomotiveLogParser::domainStats(const QVector<DltAnalyzerInterface::LogEntry> &entries)
{
    int carplay = 0, androidauto = 0;
    for (const auto &e : entries)
    {
        if (e.domain == QLatin1String("carplay")) carplay++;
        else if (e.domain == QLatin1String("androidauto")) androidauto++;
    }
    return {carplay, androidauto};
}

QHash<QString, QStringList> AutomotiveLogParser::availablePresets()
{
    QHash<QString, QStringList> presets;
    const auto &reg = CategoryRegistry::instance();
    for (const auto &cat : reg.categories()) {
        if (!cat.domain.isEmpty() || cat.hasQuickAction)
            presets[cat.quickAction.query.isEmpty() ? cat.id.toLower() : cat.quickAction.query]
                = cat.aliases;
    }
    for (const auto &ev : reg.projectionEvents()) {
        presets[ev.query] = QStringList{ev.id};
    }
    for (const auto &cf : reg.combinedFilters()) {
        const QString key = cf.quickAction.query.isEmpty()
            ? cf.aliases.value(0, cf.id)
            : cf.quickAction.query;
        presets[key] = cf.aliases;
    }
    return presets;
}

} // namespace dltchat

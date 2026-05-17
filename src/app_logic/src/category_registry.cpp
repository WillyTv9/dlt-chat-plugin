#include "dltchat/category_registry.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
#include <QtConcurrent>

namespace dltchat {

namespace {

static QStringList jsonStringList(const QJsonValue &v)
{
    QStringList out;
    if (!v.isArray())
        return out;
    for (const auto &item : v.toArray())
        if (item.isString())
            out.append(item.toString());
    return out;
}

static void parseQuickAction(const QJsonObject &obj, QuickActionDef &qa, bool &has)
{
    if (!obj.contains("quickAction"))
        return;
    const auto qaObj = obj.value("quickAction").toObject();
    qa.label = qaObj.value("label").toString();
    qa.query = qaObj.value("query").toString();
    qa.row = qaObj.value("row").toInt(0);
    qa.col = qaObj.value("col").toInt(0);
    has = !qa.label.isEmpty() && !qa.query.isEmpty();
}

} // namespace

CategoryRegistry &CategoryRegistry::instance()
{
    static CategoryRegistry reg;
    if (!reg.m_loaded) {
        reg.loadFromResource();
    }
    return reg;
}

CategoryRegistry::CategoryRegistry() = default;

bool CategoryRegistry::loadFromResource(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    return parseJson(f.readAll());
}

bool CategoryRegistry::loadFromFile(const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    return parseJson(f.readAll());
}

bool CategoryRegistry::parseJson(const QByteArray &data)
{
    QJsonParseError err;
    const auto doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return false;

    m_categories.clear();
    m_projectionEvents.clear();
    m_combinedFilters.clear();
    m_quickActions.clear();
    m_aliasToCategory.clear();
    m_aliasToCombined.clear();
    m_aliasToProjection.clear();
    m_specialCommands.clear();
    m_categoryById.clear();

    const auto root = doc.object();

    for (const auto &v : root.value("categories").toArray()) {
        if (!v.isObject())
            continue;
        const auto o = v.toObject();
        CategoryDef cat;
        cat.id = o.value("id").toString();
        cat.aliases = jsonStringList(o.value("aliases"));
        cat.filters = jsonStringList(o.value("filters"));
        cat.priority = o.value("priority").toInt(100);
        cat.domain = o.value("domain").toString();
        cat.levelOnly = o.value("levelOnly").toBool(false);
        parseQuickAction(o, cat.quickAction, cat.hasQuickAction);
        if (cat.id.isEmpty() || cat.filters.isEmpty())
            continue;
        m_categories.append(cat);
        for (const auto &a : cat.aliases)
            m_aliasToCategory.insert(a.toLower(), cat.id);
        m_aliasToCategory.insert(cat.id.toLower(), cat.id);
        if (cat.hasQuickAction)
            m_quickActions.append(cat.quickAction);
    }

    std::sort(m_categories.begin(), m_categories.end(),
              [](const CategoryDef &a, const CategoryDef &b) {
                  return a.priority < b.priority;
              });
    m_categoryById.clear();
    for (int i = 0; i < m_categories.size(); ++i)
        m_categoryById.insert(m_categories[i].id, &m_categories[i]);

    for (const auto &v : root.value("projectionEvents").toArray()) {
        if (!v.isObject())
            continue;
        const auto o = v.toObject();
        ProjectionEventDef ev;
        ev.id = o.value("id").toString();
        ev.query = o.value("query").toString(ev.id);
        ev.domain = o.value("domain").toString();
        ev.event = o.value("event").toString();
        ev.filters = jsonStringList(o.value("filters"));
        ev.matchMode = o.value("matchMode").toString("any");
        ev.requireLevel = o.value("requireLevel").toString();
        parseQuickAction(o, ev.quickAction, ev.hasQuickAction);
        if (ev.id.isEmpty())
            continue;
        m_projectionEvents.append(ev);
        m_aliasToProjection.insert(ev.query.toLower(), ev.id);
        m_aliasToProjection.insert(ev.id.toLower(), ev.id);
        if (ev.hasQuickAction)
            m_quickActions.append(ev.quickAction);
    }

    for (const auto &v : root.value("combinedFilters").toArray()) {
        if (!v.isObject())
            continue;
        const auto o = v.toObject();
        CombinedFilterDef cf;
        cf.id = o.value("id").toString();
        cf.aliases = jsonStringList(o.value("aliases"));
        cf.levels = jsonStringList(o.value("levels"));
        cf.categoryIds = jsonStringList(o.value("categories"));
        cf.extraFilters = jsonStringList(o.value("extraFilters"));
        parseQuickAction(o, cf.quickAction, cf.hasQuickAction);
        if (cf.id.isEmpty())
            continue;
        m_combinedFilters.append(cf);
        for (const auto &a : cf.aliases)
            m_aliasToCombined.insert(a.toLower(), cf.id);
        m_aliasToCombined.insert(cf.id.toLower(), cf.id);
        if (cf.hasQuickAction)
            m_quickActions.append(cf.quickAction);
    }

    for (const auto &v : root.value("specialCommands").toArray()) {
        if (!v.isObject())
            continue;
        const auto o = v.toObject();
        const QString cmd = o.value("query").toString();
        const auto aliases = jsonStringList(o.value("aliases"));
        QuickActionDef qa;
        bool hasQa = false;
        parseQuickAction(o, qa, hasQa);
        for (const auto &a : aliases)
            m_specialCommands.insert(a.toLower(), cmd);
        m_specialCommands.insert(cmd.toLower(), cmd);
        if (hasQa)
            m_quickActions.append(qa);
    }

    std::sort(m_quickActions.begin(), m_quickActions.end(),
              [](const QuickActionDef &a, const QuickActionDef &b) {
                  if (a.row != b.row)
                      return a.row < b.row;
                  return a.col < b.col;
              });

    m_loaded = true;
    return true;
}

bool CategoryRegistry::levelMatchesFilter(const QString &level, const QString &filter) const
{
    const QString lv = level.toLower();
    const QString fl = filter.toLower();
    if (lv == fl)
        return true;
    if (fl == "error" && (lv == "error" || lv == "fatal"))
        return true;
    if (fl == "fatal" && lv == "fatal")
        return true;
    if (fl == "warn" && lv == "warn")
        return true;
    if (fl == "err" && (lv == "error" || lv == "fatal"))
        return true;
    if (fl == "lerr" && (lv == "error" || lv == "fatal"))
        return true;
    if (fl == "lwarn" && lv == "warn")
        return true;
    if (fl == "linf" && lv == "info")
        return true;
    if (fl == "ldebug" && lv == "debug")
        return true;
    if (fl == "dbg" && lv == "debug")
        return true;
    return false;
}

bool CategoryRegistry::fieldMatchesFilter(const DltAnalyzerInterface::LogEntry &entry,
                                          const QString &filter) const
{
    if (filter.isEmpty())
        return false;
    if (levelMatchesFilter(entry.level, filter))
        return true;
    return entry.apid.contains(filter, Qt::CaseInsensitive)
        || entry.ctid.contains(filter, Qt::CaseInsensitive)
        || entry.payload.contains(filter, Qt::CaseInsensitive)
        || entry.ecu.contains(filter, Qt::CaseInsensitive)
        || entry.domain.contains(filter, Qt::CaseInsensitive)
        || entry.event.contains(filter, Qt::CaseInsensitive)
        || entry.category.contains(filter, Qt::CaseInsensitive);
}

bool CategoryRegistry::entryMatchesAnyFilter(const DltAnalyzerInterface::LogEntry &entry,
                                             const QStringList &filters) const
{
    for (const auto &f : filters) {
        if (fieldMatchesFilter(entry, f))
            return true;
    }
    return false;
}

bool CategoryRegistry::entryMatchesAllFilters(const DltAnalyzerInterface::LogEntry &entry,
                                              const QStringList &filters) const
{
    if (filters.isEmpty())
        return true;
    for (const auto &f : filters) {
        if (!fieldMatchesFilter(entry, f))
            return false;
    }
    return true;
}

bool CategoryRegistry::entryMatchesCategory(const DltAnalyzerInterface::LogEntry &entry,
                                            const CategoryDef &cat) const
{
    if (cat.id == QLatin1String("SMARTPHONE_PROJECTION")) {
        if (entry.domain == QLatin1String("carplay") || entry.domain == QLatin1String("androidauto"))
            return true;
    }

    if (!cat.domain.isEmpty() && entry.domain == cat.domain)
        return true;

    if (!entry.category.isEmpty() && entry.category == cat.id)
        return true;

    if (cat.levelOnly) {
        for (const auto &f : cat.filters) {
            if (levelMatchesFilter(entry.level, f))
                return true;
            if (entry.payload.contains(f, Qt::CaseInsensitive))
                return true;
        }
        return false;
    }

    return entryMatchesAnyFilter(entry, cat.filters);
}

bool CategoryRegistry::entryMatchesProjectionEvent(const DltAnalyzerInterface::LogEntry &entry,
                                                   const ProjectionEventDef &ev) const
{
    if (!ev.requireLevel.isEmpty()) {
        if (!levelMatchesFilter(entry.level, ev.requireLevel)
            && entry.level.compare(ev.requireLevel, Qt::CaseInsensitive) != 0)
            return false;
    }

    if (!ev.domain.isEmpty() && entry.domain != ev.domain)
        return false;

    if (!ev.event.isEmpty() && entry.event == ev.event)
        return true;

    if (ev.id == QLatin1String("session")) {
        return entry.event == QLatin1String("session_start")
            || entry.event == QLatin1String("session_stop");
    }

    if (ev.filters.isEmpty())
        return false;

    if (ev.matchMode == QLatin1String("all"))
        return entryMatchesAllFilters(entry, ev.filters);
    return entryMatchesAnyFilter(entry, ev.filters);
}

const CategoryDef *CategoryRegistry::bestCategoryForEntry(
    const DltAnalyzerInterface::LogEntry &entry) const
{
    const CategoryDef *best = nullptr;
    for (const auto &cat : m_categories) {
        if (!entryMatchesCategory(entry, cat))
            continue;
        if (!best || cat.priority < best->priority)
            best = &cat;
    }
    return best;
}

QString CategoryRegistry::classifyEntry(DltAnalyzerInterface::LogEntry &entry) const
{
    const CategoryDef *best = bestCategoryForEntry(entry);
    if (best) {
        entry.category = best->id;
        if (!best->domain.isEmpty())
            entry.domain = best->domain;
    } else {
        entry.category.clear();
    }
    assignProjectionEvent(entry);
    return entry.category;
}

void CategoryRegistry::assignProjectionEvent(DltAnalyzerInterface::LogEntry &entry) const
{
    const auto haystack = QStringList{entry.payload, entry.apid, entry.ctid}.join(QLatin1Char(' '));
    if (haystack.contains(QLatin1String("session"), Qt::CaseInsensitive)) {
        if (haystack.contains(QLatin1String("stop"), Qt::CaseInsensitive)
            || haystack.contains(QLatin1String("end"), Qt::CaseInsensitive)
            || haystack.contains(QLatin1String("disconnect"), Qt::CaseInsensitive))
            entry.event = QStringLiteral("session_stop");
        else if (haystack.contains(QLatin1String("start"), Qt::CaseInsensitive)
                 || haystack.contains(QLatin1String("begin"), Qt::CaseInsensitive)
                 || haystack.contains(QLatin1String("connect"), Qt::CaseInsensitive))
            entry.event = QStringLiteral("session_start");
    }

    for (const auto &ev : m_projectionEvents) {
        if (!entryMatchesProjectionEvent(entry, ev))
            continue;
        if (!ev.event.isEmpty())
            entry.event = ev.event;
        if (!ev.domain.isEmpty())
            entry.domain = ev.domain;
        return;
    }
}

ResolvedQuery CategoryRegistry::resolveQuery(const QString &query) const
{
    ResolvedQuery r;
    const QString lq = query.trimmed().toLower();
    if (lq.isEmpty())
        return r;

    if (lq.startsWith(QLatin1String("category:"))) {
        r.kind = ResolvedQuery::Kind::CategoryById;
        r.categoryId = lq.mid(9).trimmed().toUpper();
        r.queryKey = lq;
        return r;
    }

    if (m_specialCommands.contains(lq)) {
        r.kind = ResolvedQuery::Kind::SpecialCommand;
        r.specialCommand = m_specialCommands.value(lq);
        r.queryKey = lq;
        return r;
    }

    if (m_aliasToCombined.contains(lq)) {
        r.kind = ResolvedQuery::Kind::CombinedFilter;
        r.combinedId = m_aliasToCombined.value(lq);
        r.queryKey = lq;
        for (const auto &cf : m_combinedFilters) {
            if (cf.id != r.combinedId)
                continue;
            r.levelFilter = cf.levels;
            r.categoryIds = cf.categoryIds;
            r.extraFilters = cf.extraFilters;
            break;
        }
        return r;
    }

    if (m_aliasToProjection.contains(lq)) {
        r.kind = ResolvedQuery::Kind::ProjectionEvent;
        r.projectionEventId = m_aliasToProjection.value(lq);
        r.queryKey = lq;
        return r;
    }

    if (m_aliasToCategory.contains(lq)) {
        r.kind = ResolvedQuery::Kind::Category;
        r.categoryId = m_aliasToCategory.value(lq);
        r.categoryIds = QStringList{r.categoryId};
        r.queryKey = lq;
        return r;
    }

    return r;
}

bool CategoryRegistry::isSpecialCommand(const QString &query) const
{
    const QString lq = query.trimmed().toLower();
    if (m_specialCommands.contains(lq))
        return true;
    for (auto it = m_specialCommands.constBegin(); it != m_specialCommands.constEnd(); ++it) {
        if (lq.startsWith(it.key() + QLatin1Char(' ')))
            return true;
    }
    return false;
}


QVector<DltAnalyzerInterface::LogEntry> CategoryRegistry::filterEntries(
    const QVector<DltAnalyzerInterface::LogEntry> &entries,
    const ResolvedQuery &resolved) const
{
    if (entries.isEmpty())
        return {};

    auto levelOk = [&](const DltAnalyzerInterface::LogEntry &e) {
        if (resolved.levelFilter.isEmpty())
            return true;
        for (const auto &lv : resolved.levelFilter) {
            if (levelMatchesFilter(e.level, lv))
                return true;
        }
        return false;
    };

    auto extraOk = [&](const DltAnalyzerInterface::LogEntry &e) {
        if (resolved.extraFilters.isEmpty())
            return true;
        return entryMatchesAnyFilter(e, resolved.extraFilters);
    };

    std::function<bool(const DltAnalyzerInterface::LogEntry&)> filterFn;

    switch (resolved.kind) {
    case ResolvedQuery::Kind::ProjectionEvent: {
        ProjectionEventDef ev;
        bool found = false;
        for (const auto &e : m_projectionEvents) {
            if (e.id == resolved.projectionEventId) {
                ev = e;
                found = true;
                break;
            }
        }
        if (!found) return {};
        filterFn = [this, ev](const DltAnalyzerInterface::LogEntry &e) {
            return entryMatchesProjectionEvent(e, ev);
        };
        break;
    }
    case ResolvedQuery::Kind::Category:
    case ResolvedQuery::Kind::CategoryById: {
        const CategoryDef *cat = categoryById(resolved.categoryId);
        if (!cat) return {};
        filterFn = [this, cat, levelOk, extraOk](const DltAnalyzerInterface::LogEntry &e) {
            return levelOk(e) && extraOk(e) && entryMatchesCategory(e, *cat);
        };
        break;
    }
    case ResolvedQuery::Kind::CombinedFilter: {
        filterFn = [this, resolved, levelOk, extraOk](const DltAnalyzerInterface::LogEntry &e) {
            if (!levelOk(e) || !extraOk(e))
                return false;
            if (resolved.categoryIds.isEmpty())
                return true;
            for (const auto &cid : resolved.categoryIds) {
                const CategoryDef *cat = categoryById(cid);
                if (cat && entryMatchesCategory(e, *cat))
                    return true;
            }
            return false;
        };
        break;
    }
    default:
        return {};
    }

    QVector<DltAnalyzerInterface::LogEntry> results = QtConcurrent::blockingFiltered(entries, filterFn);
    
    std::sort(results.begin(), results.end(),
              [](const auto &a, const auto &b) { return a.index < b.index; });
    return results;
}

QStringList CategoryRegistry::allCompletionStrings() const
{
    QStringList items;
    for (const auto &cat : m_categories) {
        items.append(cat.aliases);
        items.append(QStringLiteral("category:%1").arg(cat.id));
    }
    for (const auto &cf : m_combinedFilters)
        items.append(cf.aliases);
    for (const auto &ev : m_projectionEvents) {
        items.append(ev.query);
        items.append(ev.id);
    }
    for (auto it = m_specialCommands.constBegin(); it != m_specialCommands.constEnd(); ++it)
        items.append(it.key());
    items.removeDuplicates();
    items.sort(Qt::CaseInsensitive);
    return items;
}

QString CategoryRegistry::buildCategoriesHelpHtml() const
{
    QString html = QStringLiteral("<b>Categorie disponibili:</b><br>");
    for (const auto &cat : m_categories) {
        if (cat.levelOnly)
            continue;
        html += QStringLiteral("<b>%1</b>: alias %2<br>")
                    .arg(cat.id, cat.aliases.join(QStringLiteral(", ")));
    }
    html += QStringLiteral("<br><b>Eventi projection:</b><br>");
    for (const auto &ev : m_projectionEvents) {
        html += QStringLiteral("<b>%1</b> (%2)<br>").arg(ev.id, ev.query);
    }
    html += QStringLiteral("<br><b>Filtri combinati:</b> ");
    QStringList comboNames;
    for (const auto &cf : m_combinedFilters)
        comboNames.append(cf.aliases.value(0, cf.id));
    html += comboNames.join(QStringLiteral(", "));
    html += QStringLiteral("<br><i>Usa category:NOME oppure alias (es. gps_errors, carplay)</i>");
    return html;
}

const CategoryDef *CategoryRegistry::categoryById(const QString &id) const
{
    return m_categoryById.value(id.toUpper(), nullptr);
}

} // namespace dltchat

#include "dltchat/user_filter_manager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QRegularExpression>

namespace dltchat {

UserFilterManager::UserFilterManager(QObject *parent)
    : QObject(parent)
{
}

bool UserFilterManager::loadFromFile(const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        if (error) *error = file.errorString();
        return false;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError)
    {
        if (error) *error = parseError.errorString();
        return false;
    }

    QJsonObject root = doc.object();
    if (!root.contains("version"))
    {
        if (error) *error = QStringLiteral("Missing required 'version' field");
        return false;
    }

    QJsonArray arr = root["filters"].toArray();
    m_filters = parseFilters(arr);
    emit filtersChanged();
    return true;
}

bool UserFilterManager::saveToFile(const QString &path, QString *error) const
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (error) *error = file.errorString();
        return false;
    }

    QJsonObject root;
    root["version"] = "1.0.0";
    root["filters"] = serializeFilters();

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

QHash<int, QColor> UserFilterManager::applyToEntries(
    const QVector<DltAnalyzerInterface::LogEntry> &entries) const
{
    QHash<int, QColor> highlights;

    for (const auto &filter : m_filters)
    {
        if (!filter.enabled || !filter.isValid) continue;

        for (const auto &entry : entries)
        {
            bool match = false;
            for (const QString &field : filter.fields)
            {
                QString value;
                if (field == "payload") value = entry.payload;
                else if (field == "apid") value = entry.apid;
                else if (field == "ctid") value = entry.ctid;
                else if (field == "ecu") value = entry.ecu;

                if (value.contains(filter.regex))
                {
                    match = true;
                    break;
                }
            }

            if (match)
            {
                if (!filter.levels.isEmpty() && !filter.levels.contains(entry.level))
                    continue;
                if (!filter.domain.isEmpty() && filter.domain != entry.domain)
                    continue;
                highlights[entry.index] = filter.color;
            }
        }
    }

    return highlights;
}

void UserFilterManager::addFilter(const UserFilterRule &rule)
{
    m_filters.append(rule);
    emit filtersChanged();
}

bool UserFilterManager::removeFilter(const QString &label)
{
    for (int i = 0; i < m_filters.size(); ++i)
    {
        if (m_filters[i].label == label)
        {
            m_filters.remove(i);
            emit filtersChanged();
            return true;
        }
    }
    return false;
}

void UserFilterManager::clearFilters()
{
    m_filters.clear();
    emit filtersChanged();
}

int UserFilterManager::activeFilterCount() const
{
    int count = 0;
    for (const auto &f : m_filters)
    {
        if (f.enabled && f.isValid)
            count++;
    }
    return count;
}

QVector<UserFilterRule> UserFilterManager::filters() const
{
    return m_filters;
}

QVector<UserFilterRule> UserFilterManager::parseFilters(const QJsonArray &arr) const
{
    QVector<UserFilterRule> rules;
    for (const auto &val : arr)
    {
        QJsonObject obj = val.toObject();
        UserFilterRule rule;
        rule.label = obj["label"].toString();
        rule.pattern = obj["pattern"].toString();
        rule.enabled = obj["enabled"].toBool(true);

        QJsonArray fields = obj["fields"].toArray();
        for (const auto &f : fields)
            rule.fields.append(f.toString());
        if (rule.fields.isEmpty())
            rule.fields << "payload";

        QJsonArray levels = obj["level"].toArray();
        for (const auto &l : levels)
            rule.levels.append(l.toString());

        rule.domain = obj["domain"].toString();

        rule.color = QColor(obj["color"].toString("#FF0000"));

        rule.regex = QRegularExpression(rule.pattern,
            QRegularExpression::CaseInsensitiveOption);
        rule.isValid = rule.regex.isValid();

        rules.append(rule);
    }
    return rules;
}

QJsonArray UserFilterManager::serializeFilters() const
{
    QJsonArray arr;
    for (const auto &rule : m_filters)
    {
        QJsonObject obj;
        obj["label"] = rule.label;
        obj["pattern"] = rule.pattern;
        obj["enabled"] = rule.enabled;
        obj["color"] = rule.color.name();

        QJsonArray fields;
        for (const auto &f : rule.fields)
            fields.append(f);
        obj["fields"] = fields;

        QJsonArray levels;
        for (const auto &l : rule.levels)
            levels.append(l);
        obj["level"] = levels;

        if (!rule.domain.isEmpty())
            obj["domain"] = rule.domain;

        arr.append(obj);
    }
    return arr;
}

} // namespace dltchat

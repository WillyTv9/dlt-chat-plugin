#ifndef DLTCHAT_USER_FILTER_MANAGER_H
#define DLTCHAT_USER_FILTER_MANAGER_H

#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>

#include "analyzer_interface.h"

namespace dltchat {

struct UserFilterRule {
    QString label;
    QString pattern;
    QStringList fields;
    QString colorHex;
    QStringList levels;
    QString domain;
    bool enabled = true;
    bool isValid = false;
    QRegularExpression regex;
};

class UserFilterManager : public QObject
{
    Q_OBJECT
public:
    explicit UserFilterManager(QObject *parent = nullptr);

    bool loadFromFile(const QString &path, QString *error = nullptr);
    bool saveToFile(const QString &path, QString *error = nullptr) const;
    QHash<int, QString> applyToEntries(const QVector<DltAnalyzerInterface::LogEntry> &entries) const;

    void addFilter(const UserFilterRule &rule);
    bool removeFilter(const QString &label);
    void clearFilters();
    int activeFilterCount() const;
    QVector<UserFilterRule> filters() const;

signals:
    void filtersChanged();

private:
    QVector<UserFilterRule> m_filters;
    QVector<UserFilterRule> parseFilters(const QJsonArray &arr) const;
    QJsonArray serializeFilters() const;
};

} // namespace dltchat

#endif

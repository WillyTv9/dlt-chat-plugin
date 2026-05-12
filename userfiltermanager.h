#ifndef USERFILTERMANAGER_H
#define USERFILTERMANAGER_H

#include <QColor>
#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QRegularExpression>
#include <QStringList>
#include <QVector>

#include "dltanalyzerinterface.h"

struct UserFilter {
    QString             label;
    QRegularExpression  regex;
    QStringList         fields;
    QColor              color;
    QStringList         levels;
    QString             domain;
    bool                enabled = true;
    bool                isValid = false;
};

class UserFilterManager : public QObject
{
    Q_OBJECT
public:
    explicit UserFilterManager(QObject *parent = nullptr);

    bool loadFromFile(const QString &path, QString *error = nullptr);
    void clear();
    const QVector<UserFilter> &filters() const;
    int activeFilterCount() const;

    QHash<int, QColor> applyToEntries(
        const QVector<DltAnalyzerInterface::LogEntry> &entries) const;

signals:
    void filtersChanged();

private:
    QVector<UserFilter> parseFilterArray(const QJsonArray &arr, QString *error);
    UserFilter parseSingleFilter(const QJsonObject &obj, int idx, QString *error);
    bool matchesEntry(const UserFilter &f,
                      const DltAnalyzerInterface::LogEntry &e) const;

    QVector<UserFilter> m_filters;
};

#endif

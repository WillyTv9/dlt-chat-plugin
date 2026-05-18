#pragma once
#include <QString>
#include <QStringList>
#include <QVector>

struct AuditRow {
    QString     label, query, kind, target;
    int         matchCount  = 0;
    double      pctOfTotal  = 0.0;
    qint64      timeMs      = 0;
    QStringList samples;
    bool        isError     = false;
    QString     note;
};

struct Collision {
    QString     alias, winner;
    QStringList allCats;
};

struct ShortFilter {
    QString catId, token;
};

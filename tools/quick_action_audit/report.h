#pragma once
#include <QMap>
#include <QVector>
#include "audit_types.h"
#include "dltchat/analyzer_interface.h"

void writeReport(
    const QString &outPath,
    const QString &dltPath,
    int total,
    const QVector<dltchat::DltAnalyzerInterface::LogEntry> &entries,
    const QMap<QString, int> &levelCounts,
    const QVector<AuditRow> &rows,
    const QVector<Collision> &collisions,
    const QVector<ShortFilter> &shortFilters);

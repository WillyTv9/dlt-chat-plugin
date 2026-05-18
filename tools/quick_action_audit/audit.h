#pragma once
#include <QVector>
#include "audit_types.h"
#include "dltchat/analyzer_interface.h"

QVector<AuditRow> runQuickActions(
    const QVector<dltchat::DltAnalyzerInterface::LogEntry> &entries);

QVector<Collision>   findAliasCollisions();
QVector<ShortFilter> findAmbiguousFilters();

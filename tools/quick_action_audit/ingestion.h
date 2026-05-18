#pragma once
#include <QMap>
#include <QVector>
#include "dltchat/analyzer_interface.h"
class QDltFile;

QVector<dltchat::DltAnalyzerInterface::LogEntry> ingestFile(
    QDltFile &dltFile, int ingestCount, QMap<QString, int> &levelCounts);

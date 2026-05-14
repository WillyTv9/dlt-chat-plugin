#ifndef DLTCHAT_EXPORT_ENGINE_H
#define DLTCHAT_EXPORT_ENGINE_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QVector>

#include "analyzer_interface.h"

namespace dltchat {

class DltExport
{
public:
    static bool exportToCsv(const QString &filePath,
                            const QList<int> &indices,
                            const QStringList &snippets,
                            const QString &query,
                            const QVector<DltAnalyzerInterface::LogEntry> &entries);
    static bool exportAllEntries(const QString &filePath,
                                 const QVector<DltAnalyzerInterface::LogEntry> &entries);

private:
    static QString escapeCsvField(const QString &field);
};

} // namespace dltchat

#endif

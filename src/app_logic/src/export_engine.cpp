#include "dltchat/export_engine.h"

#include <QFile>
#include <QTextStream>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QTextCodec>
#endif

namespace dltchat {

QString DltExport::escapeCsvField(const QString &field)
{
    if (field.contains(',') || field.contains('"') || field.contains('\n') || field.contains('\r'))
    {
        QString escaped = field;
        escaped.replace('"', "\"\"");
        return '"' + escaped + '"';
    }
    return field;
}

bool DltExport::exportToCsv(const QString &filePath,
                            const QList<int> &indices,
                            const QStringList &snippets,
                            const QString &query,
                            const QVector<DltAnalyzerInterface::LogEntry> &entries)
{
    if (indices.isEmpty())
        return false;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec(QTextCodec::codecForName("UTF-8"));
#endif

    out << "Index,Time,Timestamp,Level,ECU,APID,CTID,Domain,Payload,Snippet,Query\n";

    for (int i = 0; i < indices.size(); ++i)
    {
        int idx = indices[i];
        QString snippet = (i < snippets.size()) ? snippets[i] : QString();

        for (const auto &entry : entries)
        {
            if (entry.index == idx)
            {
                out << entry.index << ","
                    << escapeCsvField(entry.time) << ","
                    << escapeCsvField(entry.timestamp) << ","
                    << escapeCsvField(entry.level) << ","
                    << escapeCsvField(entry.ecu) << ","
                    << escapeCsvField(entry.apid) << ","
                    << escapeCsvField(entry.ctid) << ","
                    << escapeCsvField(entry.domain) << ","
                    << escapeCsvField(entry.payload) << ","
                    << escapeCsvField(snippet) << ","
                    << escapeCsvField(query) << "\n";
                break;
            }
        }
    }

    file.close();
    return true;
}

bool DltExport::exportAllEntries(const QString &filePath,
                                 const QVector<DltAnalyzerInterface::LogEntry> &entries)
{
    if (entries.isEmpty())
        return false;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec(QTextCodec::codecForName("UTF-8"));
#endif

    out << "Index,Time,Timestamp,Level,ECU,APID,CTID,Domain,Payload\n";

    for (const auto &entry : entries)
    {
        out << entry.index << ","
            << escapeCsvField(entry.time) << ","
            << escapeCsvField(entry.timestamp) << ","
            << escapeCsvField(entry.level) << ","
            << escapeCsvField(entry.ecu) << ","
            << escapeCsvField(entry.apid) << ","
            << escapeCsvField(entry.ctid) << ","
            << escapeCsvField(entry.domain) << ","
            << escapeCsvField(entry.payload) << "\n";
    }

    file.close();
    return true;
}

} // namespace dltchat

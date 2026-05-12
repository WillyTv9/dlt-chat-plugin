#include "dltexport.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

QString DltExport::generateCsvRow(const QStringList &fields)
{
    QStringList escaped;
    escaped.reserve(fields.size());
    for (const QString &field : fields)
    {
        QString value = field;
        value.replace("\"", "\"\"");
        if (value.contains(',') || value.contains('"') || value.contains('\n') || value.contains('\r'))
            value = QString("\"%1\"").arg(value);
        escaped.append(value);
    }
    return escaped.join(',');
}

QStringList DltExport::sanitizeFields(const QStringList &fields)
{
    QStringList sanitized;
    sanitized.reserve(fields.size());
    for (const QString &field : fields)
        sanitized.append(field.trimmed());
    return sanitized;
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    out.setEncoding(QStringConverter::Encoding::System);
#else
    out.setCodec("UTF-8");
#endif

    out << generateCsvRow({"#", "Index", "Time", "Timestamp", "Level", "ECU", "APID", "CTID", "Domain", "Payload", "Source Query"}) << "\n";

    QHash<int, DltAnalyzerInterface::LogEntry> entryMap;
    for (const auto &e : entries)
        entryMap.insert(e.index, e);

    const int count = qMin(indices.size(), snippets.size());
    for (int i = 0; i < count; ++i)
    {
        int idx = indices[i];
        auto it = entryMap.constFind(idx);
        QStringList fields = {
            QString::number(i + 1),
            QString::number(idx),
            it != entryMap.constEnd() ? it->time : QString(),
            it != entryMap.constEnd() ? it->timestamp : QString(),
            it != entryMap.constEnd() ? it->level : QString(),
            it != entryMap.constEnd() ? it->ecu : QString(),
            it != entryMap.constEnd() ? it->apid : QString(),
            it != entryMap.constEnd() ? it->ctid : QString(),
            it != entryMap.constEnd() ? it->domain : QString(),
            snippets[i],
            query
        };
        out << generateCsvRow(sanitizeFields(fields)) << "\n";
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    out.setEncoding(QStringConverter::Encoding::System);
#else
    out.setCodec("UTF-8");
#endif

    out << generateCsvRow({"Index", "Time", "Timestamp", "Level", "ECU", "APID", "CTID", "Domain", "Payload"}) << "\n";

    for (const DltAnalyzerInterface::LogEntry &entry : entries)
    {
        out << generateCsvRow(sanitizeFields({
            QString::number(entry.index),
            entry.time,
            entry.timestamp,
            entry.level,
            entry.ecu,
            entry.apid,
            entry.ctid,
            entry.domain,
            entry.payload
        })) << "\n";
    }

    file.close();
    return true;
}

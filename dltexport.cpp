/*
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at http://mozilla.org/MPL/2.0/.
 *
 * SPDX-License-Identifier: MPL-2.0
 */

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
        {
            value = QString("\"%1\"").arg(value);
        }
        escaped.append(value);
    }
    return escaped.join(',');
}

QStringList DltExport::sanitizeFields(const QStringList &fields)
{
    QStringList sanitized;
    sanitized.reserve(fields.size());
    for (const QString &field : fields)
    {
        sanitized.append(field.trimmed());
    }
    return sanitized;
}

bool DltExport::exportToCsv(const QString &filePath,
                          const QList<int> &indices,
                          const QStringList &snippets,
                          const QString &query)
{
    if (indices.isEmpty())
    {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }

    QTextStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    out.setEncoding(QStringConverter::Encoding::System);
#else
    out.setCodec("UTF-8");
#endif

    // Write header with all relevant fields
    out << generateCsvRow({"#", "Index", "Time", "Timestamp", "Level", "ECU", "APID", "CTID", "Payload", "Source Query"}) << "\n";

    const int count = qMin(indices.size(), snippets.size());
    for (int i = 0; i < count; ++i)
    {
        // Note: We only have index and snippet from the result
        // Full entry data would require reference to entries vector
        QStringList fields = {
            QString::number(i + 1),           // Row number
            QString::number(indices[i]),      // Index
            QString(),                        // Time (not available in export)
            QString(),                        // Timestamp (not available)
            QString(),                        // Level (not available)
            QString(),                        // ECU (not available)
            QString(),                        // APID (not available)
            QString(),                        // CTID (not available)
            snippets[i],                      // Payload
            query                             // Source query
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
    {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }

    QTextStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    out.setEncoding(QStringConverter::Encoding::System);
#else
    out.setCodec("UTF-8");
#endif

    out << generateCsvRow({"Index", "Time", "Timestamp", "Level", "ECU", "APID", "CTID", "Payload"}) << "\n";

    for (const DltAnalyzerInterface::LogEntry &entry : entries)
    {
        QStringList fields = {
            QString::number(entry.index),
            entry.time,
            entry.timestamp,
            entry.level,
            entry.ecu,
            entry.apid,
            entry.ctid,
            entry.payload
        };
        out << generateCsvRow(sanitizeFields(fields)) << "\n";
    }

    file.close();
    return true;
}

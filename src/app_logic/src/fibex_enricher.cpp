#include "dltchat/fibex_enricher.h"

#include <QDomDocument>
#include <QFile>
#include <QXmlStreamReader>

namespace dltchat {

bool FibexEnricher::loadFile(const QString &filePath, QString *errorOut)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        if (errorOut) *errorOut = file.errorString();
        return false;
    }

    bool ok = parseXmlFile(filePath, errorOut);
    file.close();

    if (ok)
    {
        m_loaded = true;
    }

    return ok;
}

void FibexEnricher::enrichAll(QVector<DltAnalyzerInterface::LogEntry> &entries) const
{
    if (!m_loaded) return;

    for (auto &entry : entries)
    {
        QString enriched = enrich(entry.payload);
        if (enriched != entry.payload)
            entry.payload = enriched;
    }
}

QString FibexEnricher::enrich(const QString &rawId) const
{
    auto it = m_mappings.constFind(rawId);
    if (it != m_mappings.constEnd())
        return rawId + QString(" /* %1 */").arg(it.value());
    return rawId;
}

void FibexEnricher::clear()
{
    m_mappings.clear();
    m_loaded = false;
}

bool FibexEnricher::parseXmlFile(const QString &filePath, QString *errorOut)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        if (errorOut) *errorOut = file.errorString();
        return false;
    }

    QXmlStreamReader xml(&file);
    QString currentId;
    QString currentName;

    while (!xml.atEnd() && !xml.hasError())
    {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement)
        {
            if (xml.name().toString() == "FUNCTION" || xml.name().toString() == "function")
            {
                currentId = xml.attributes().value("id").toString();
                if (currentId.isEmpty())
                    currentId = xml.attributes().value("ID").toString();
            }
            else if (xml.name().toString() == "SHORT-NAME" || xml.name().toString() == "short-name" ||
                     xml.name().toString() == "SHORT_NAME")
            {
                currentName = xml.readElementText();
            }
        }
        else if (token == QXmlStreamReader::EndElement)
        {
            if ((xml.name().toString() == "FUNCTION" || xml.name().toString() == "function") &&
                !currentId.isEmpty() && !currentName.isEmpty())
            {
                m_mappings[currentId] = currentName;
                currentId.clear();
                currentName.clear();
            }
        }
    }

    file.close();

    if (xml.hasError())
    {
        if (errorOut)
            *errorOut = xml.errorString();
        return false;
    }

    return true;
}

} // namespace dltchat

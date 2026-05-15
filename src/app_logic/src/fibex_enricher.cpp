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
        auto it = m_mappings.constFind(entry.apid + "_" + entry.ctid);
        if (it == m_mappings.constEnd())
            it = m_mappings.constFind(entry.apid);
        if (it != m_mappings.constEnd() && !entry.payload.contains(it.value()))
            entry.payload += QString(" /* %1 */").arg(it.value());
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
    QString shortName;
    QString functionName;
    QString longName;
    bool inBlock = false;

    while (!xml.atEnd() && !xml.hasError())
    {
        QXmlStreamReader::TokenType token = xml.readNext();
        const QString name = xml.name().toString().toUpper();

        if (token == QXmlStreamReader::StartElement)
        {
            if (name == "APPLICATION" || name == "FUNCTION")
            {
                inBlock = true;
                shortName.clear();
                functionName.clear();
                longName.clear();
            }
            else if (inBlock && (name == "SHORT-NAME" || name == "SHORT_NAME"))
            {
                shortName = xml.readElementText().trimmed();
            }
            else if (inBlock && name == "FUNCTION-NAME")
            {
                functionName = xml.readElementText().trimmed();
            }
            else if (inBlock && name == "LONG-NAME")
            {
                longName = xml.readElementText().trimmed();
            }
        }
        else if (token == QXmlStreamReader::EndElement)
        {
            if (name == "APPLICATION" || name == "FUNCTION")
            {
                if (!shortName.isEmpty())
                {
                    QString value = functionName;
                    if (value.isEmpty())
                        value = longName.isEmpty() ? shortName : longName;
                    m_mappings[shortName] = value;
                }
                inBlock = false;
                shortName.clear();
                functionName.clear();
                longName.clear();
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

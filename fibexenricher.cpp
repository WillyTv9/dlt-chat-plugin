#include "fibexenricher.h"

#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

FibexEnricher::FibexEnricher()
{
}

bool FibexEnricher::loadFile(const QString &filePath, QString *errorOut)
{
    QFileInfo fi(filePath);
    if (!fi.exists()) {
        if (errorOut) *errorOut = QString("File not found: %1").arg(filePath);
        return false;
    }

    QString ext = fi.suffix().toLower();
    if (ext == "fibex") {
        parseFibexFile(filePath);
    } else if (ext == "xml") {
        parseGenericXmlFile(filePath, errorOut);
    } else {
        // Try as XML anyway
        parseGenericXmlFile(filePath, errorOut);
    }

    // Check if we got any data
    if (m_enrichmentMap.isEmpty()) {
        m_failedFiles.append(filePath);
        if (errorOut)
            errorOut->append(" (parsed but no enrichment data found)");
        return false;
    }

    if (!m_loadedFiles.contains(filePath))
        m_loadedFiles.append(filePath);
    return true;
}

void FibexEnricher::clear()
{
    m_enrichmentMap.clear();
    m_loadedFiles.clear();
    m_failedFiles.clear();
}

bool FibexEnricher::isLoaded() const
{
    return !m_enrichmentMap.isEmpty();
}

QStringList FibexEnricher::loadedFiles() const
{
    return m_loadedFiles;
}

QString FibexEnricher::enrich(const QString &apid, const QString &ctid) const
{
    QString key = QString("%1_%2").arg(apid.toUpper(), ctid.toUpper());
    auto it = m_enrichmentMap.constFind(key);
    if (it != m_enrichmentMap.constEnd() && it->valid)
        return it->functionName;

    // Try case-insensitive fallback
    for (auto it2 = m_enrichmentMap.constBegin(); it2 != m_enrichmentMap.constEnd(); ++it2) {
        if (it2.key().compare(key, Qt::CaseInsensitive) == 0 && it2->valid)
            return it2->functionName;
    }

    return QString();
}

void FibexEnricher::enrichEntry(DltAnalyzerInterface::LogEntry &entry) const
{
    QString name = enrich(entry.apid, entry.ctid);
    if (!name.isEmpty()) {
        // Append human-readable name to payload for AI context
        entry.payload = QString("%1 {%2}").arg(entry.payload, name);
    }
}

void FibexEnricher::enrichAll(QVector<DltAnalyzerInterface::LogEntry> &entries) const
{
    for (auto &e : entries)
        enrichEntry(e);
}

void FibexEnricher::parseFibexFile(const QString &path)
{
    QDomDocument doc;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;

    if (!doc.setContent(&file)) {
        file.close();
        return;
    }
    file.close();

    // AUTOSAR Fibex: look for <SHORT-NAME> and <FUNCTION-NAME> or <APPLICATION>
    QDomElement root = doc.documentElement();

    // Find all APPLICATION nodes (common in Fibex)
    QDomNodeList apps = root.elementsByTagName("APPLICATION");
    for (int i = 0; i < apps.size(); ++i) {
        QDomElement app = apps.at(i).toElement();
        if (app.isNull()) continue;

        QString shortName;
        QString longName;
        QStringList sigList;

        QDomNodeList children = app.childNodes();
        for (int j = 0; j < children.size(); ++j) {
            QDomElement child = children.at(j).toElement();
            if (child.isNull()) continue;
            if (child.tagName() == "SHORT-NAME")
                shortName = child.text().trimmed();
            else if (child.tagName() == "LONG-NAME" || child.tagName() == "DESC" || child.tagName() == "FUNCTION-NAME")
                longName = child.text().trimmed();
            else if (child.tagName() == "SIGNAL")
                sigList.append(child.text().trimmed());
        }

        if (!shortName.isEmpty()) {
            QString normalized = shortName.toUpper();
            QString apid, ctid;

            static QRegularExpression sepRe("[/_]");
            QStringList parts = normalized.split(sepRe);
            if (parts.size() >= 2) {
                apid = parts[0];
                ctid = parts.mid(1).join("_");
            } else {
                apid = normalized;
                ctid = "MAIN";
            }

            QString key = QString("%1_%2").arg(apid, ctid);
            EnrichmentEntry entry;
            entry.functionName = longName.isEmpty() ? shortName : longName;
            entry.signalNames = sigList;
            entry.valid = true;
            m_enrichmentMap.insert(key, entry);
        }

        if (!shortName.isEmpty()) {
            // Try to extract APID_CTID from short name or use as-is
            // Format could be "APP_CTX" or "APID/CTX" or just "AppName"
            QString normalized = shortName.toUpper();
            QString apid, ctid;

            static QRegularExpression sepRe("[/_]");
            QStringList parts = normalized.split(sepRe);
            if (parts.size() >= 2) {
                apid = parts[0];
                ctid = parts.mid(1).join("_");
            } else {
                apid = normalized;
                ctid = "MAIN";
            }

            QString key = QString("%1_%2").arg(apid, ctid);
            EnrichmentEntry entry;
            entry.functionName = longName.isEmpty() ? shortName : longName;
            entry.signalNames = sigList;
            entry.valid = true;
            m_enrichmentMap.insert(key, entry);
        }
    }

    // Also look for FUNCTION nodes
    QDomNodeList funcs = root.elementsByTagName("FUNCTION");
    for (int i = 0; i < funcs.size(); ++i) {
        QDomElement func = funcs.at(i).toElement();
        if (func.isNull()) continue;

        QString name;
        QDomNodeList cn = func.childNodes();
        for (int j = 0; j < cn.size(); ++j) {
            QDomElement c = cn.at(j).toElement();
            if (c.isNull()) continue;
            if (c.tagName() == "SHORT-NAME")
                name = c.text().trimmed();
        }

        if (!name.isEmpty()) {
            QStringList parts = name.toUpper().split(QRegularExpression("[/_]"));
            QString key = parts.size() >= 2
                ? QString("%1_%2").arg(parts[0], parts.mid(1).join("_"))
                : QString("%1_MAIN").arg(parts[0]);

            if (!m_enrichmentMap.contains(key)) {
                EnrichmentEntry entry;
                entry.functionName = name;
                entry.valid = true;
                m_enrichmentMap.insert(key, entry);
            }
        }
    }
}

static void parseElementForMapping(const QDomElement &el, QHash<QString, FibexEnricher::EnrichmentEntry> &map)
{
    QString tag = el.tagName().toUpper();
    QString apid, ctid, name;

    // Check direct attributes for APID/CTID
    if (el.hasAttribute("APID")) apid = el.attribute("APID");
    else if (el.hasAttribute("AppId")) apid = el.attribute("AppId");
    if (el.hasAttribute("CTID")) ctid = el.attribute("CTID");
    else if (el.hasAttribute("CtxId")) ctid = el.attribute("CtxId");

    // Check for Name
    if (el.hasAttribute("Name")) name = el.attribute("Name");
    else if (el.hasAttribute("name")) name = el.attribute("name");
    else if (el.hasAttribute("SHORT-NAME")) name = el.attribute("SHORT-NAME");

    // If no direct APID/CTID, try to extract from APPLICATION/SW-COMPONENT short-name
    if (apid.isEmpty() && (tag == "APPLICATION" || tag == "SW-COMPONENT" || tag == "ATOMIC-SW-COMPONENT-TYPE")) {
        QString shortName, longName, functionName;
        QDomNodeList cn = el.childNodes();
        for (int j = 0; j < cn.size(); ++j) {
            QDomElement child = cn.at(j).toElement();
            if (child.isNull()) continue;
            QString tagName = child.tagName();
            if (tagName == "SHORT-NAME")
                shortName = child.text().trimmed();
            else if (tagName == "LONG-NAME")
                longName = child.text().trimmed();
            else if (tagName == "FUNCTION-NAME")
                functionName = child.text().trimmed();
        }
        // Prefer FUNCTION-NAME > LONG-NAME > SHORT-NAME for display
        name = functionName.isEmpty() ? (longName.isEmpty() ? shortName : longName) : functionName;
        if (!name.isEmpty()) {
            QStringList parts = shortName.toUpper().split(QRegularExpression("[/_]"));
            if (parts.size() >= 2) {
                apid = parts[0];
                ctid = parts.mid(1).join("_");
            }
        }
    }

    if (!apid.isEmpty() && !name.isEmpty()) {
        if (ctid.isEmpty()) ctid = "MAIN";
        QString key = QString("%1_%2").arg(apid.toUpper(), ctid.toUpper());
        if (!map.contains(key)) {
            FibexEnricher::EnrichmentEntry entry;
            entry.functionName = name;
            entry.valid = true;
            map.insert(key, entry);
        }
    }

    // Recursively process children
    QDomNodeList children = el.childNodes();
    for (int i = 0; i < children.size(); ++i) {
        QDomElement childEl = children.at(i).toElement();
        if (!childEl.isNull())
            parseElementForMapping(childEl, map);
    }
}

void FibexEnricher::parseGenericXmlFile(const QString &path, QString *errorOut)
{
    QDomDocument doc("fibex");
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorOut) *errorOut = QString("Cannot open: %1").arg(file.fileName());
        return;
    }

    if (!doc.setContent(&file)) {
        if (errorOut) *errorOut = "Failed to parse XML file";
        file.close();
        return;
    }
    file.close();

    QDomElement root = doc.documentElement();
    parseElementForMapping(root, m_enrichmentMap);
}

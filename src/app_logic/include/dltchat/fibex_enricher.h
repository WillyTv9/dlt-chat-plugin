#ifndef DLTCHAT_FIBEX_ENRICHER_H
#define DLTCHAT_FIBEX_ENRICHER_H

#include "analyzer_interface.h"
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

namespace dltchat {

class FibexEnricher
{
public:
    FibexEnricher() = default;

    bool loadFile(const QString &filePath, QString *errorOut = nullptr);
    bool isLoaded() const { return m_loaded; }
    int mappingCount() const { return m_mappings.size(); }

    void enrichAll(QVector<DltAnalyzerInterface::LogEntry> &entries) const;
    QString enrich(const QString &rawId) const;

    void clear();

private:
    QHash<QString, QString> m_mappings;
    bool m_loaded = false;
    bool parseXmlFile(const QString &filePath, QString *errorOut);
};

} // namespace dltchat

#endif

#ifndef DLTCHAT_LOG_INDEX_H
#define DLTCHAT_LOG_INDEX_H

#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>

namespace dltchat {

class LogIndex
{
public:
    LogIndex() = default;

    void add(const QString &keyword, int entryIndex);
    QSet<int> lookup(const QString &keyword) const;
    QSet<int> intersect(const QStringList &keywords) const;
    bool contains(const QString &keyword) const;
    void clear();
    int size() const;

    static QSet<QString> defaultStopwords();
    bool isStopword(const QString &word) const;
    void setStopwords(const QSet<QString> &sw) { m_stopwords = sw; }

private:
    QHash<QString, QSet<int>> m_index;
    QSet<QString> m_stopwords;
};

} // namespace dltchat

#endif

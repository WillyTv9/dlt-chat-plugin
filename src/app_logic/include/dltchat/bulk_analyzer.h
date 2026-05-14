#ifndef DLTCHAT_BULK_ANALYZER_H
#define DLTCHAT_BULK_ANALYZER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QVector>
#include <QHash>
#include <QSet>
#include <QJsonObject>
#include <QElapsedTimer>

#include "analyzer_interface.h"

namespace dltchat {

class DltLlmAnalyzerInterface;
struct BulkAnalysisResult {
    int logIndex = -1;
    QString category;
    QString summary;
    QSet<QString> tags;
};

class DltBulkAnalyzerWorker;

class DltBulkAnalyzer : public QObject
{
    Q_OBJECT
public:
    explicit DltBulkAnalyzer(QObject *parent = nullptr);
    ~DltBulkAnalyzer() override;

    void setAnalyzer(DltLlmAnalyzerInterface *analyzer);
    void startBulkAnalysis(const QVector<DltAnalyzerInterface::LogEntry> &entries,
                           int chunkSize = 100);
    void pauseAnalysis();
    void resumeAnalysis();
    void cancelAnalysis();
    bool isRunning() const;
    bool hasCompleted() const;

    void searchByTag(const QString &tag, QList<int> &results) const;
    void searchByCategory(const QString &category, QList<int> &results) const;
    QString getCategoryForIndex(int logIndex) const;

signals:
    void progressUpdated(double progress, int processed, int total);
    void analysisFinished(bool success);
    void errorOccurred(const QString &error);

private:
    DltBulkAnalyzerWorker *m_worker;
    QThread *m_workerThread;
    QVector<DltAnalyzerInterface::LogEntry> m_entries;
    bool m_running;
    mutable QMutex m_mutex;
};

} // namespace dltchat

#endif

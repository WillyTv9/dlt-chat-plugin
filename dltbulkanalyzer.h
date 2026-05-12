#ifndef DLTBULKANALYZER_H
#define DLTBULKANALYZER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QVector>
#include <QHash>
#include <QSet>
#include <QJsonObject>
#include <QElapsedTimer>

#include "dltanalyzerinterface.h"

struct BulkAnalysisResult {
    int logIndex = -1;
    QString category;
    QString summary;
    QSet<QString> tags;
    bool processed = false;
};

class DltBulkAnalyzerWorker : public QThread
{
    Q_OBJECT

public:
    DltBulkAnalyzerWorker(QObject *parent = nullptr);
    ~DltBulkAnalyzerWorker() override;

    void configure(DltAnalyzerInterface *analyzer,
                   int chunkSize = 100,
                   int maxRetries = 3);

    void enqueueLogs(const QVector<DltAnalyzerInterface::LogEntry> &logs);
    void processAsync();

    void pause() { m_paused = true; }
    void resume() { m_paused = false; m_cv.wakeAll(); }
    void cancel() { m_cancelled = true; m_cv.wakeAll(); }

    int totalLogs() const { return m_totalLogs; }
    int processedLogs() const { return m_processedLogs; }
    double progress() const;

    const QHash<int, BulkAnalysisResult> &results() const { return m_results; }

signals:
    void progressChanged(double progress, int processed, int total);
    void chunkProcessed(int chunkIndex, int entriesProcessed);
    void analysisComplete();
    void errorOccurred(const QString &error);

protected:
    void run() override;

private:
    bool processChunk(int chunkIndex,
                     const QVector<DltAnalyzerInterface::LogEntry> &chunk,
                     int maxRetries);

    bool waitIfPaused();

    DltAnalyzerInterface *m_analyzer = nullptr;
    int m_chunkSize = 100;
    int m_maxRetries = 3;

    QVector<DltAnalyzerInterface::LogEntry> m_logs;
    QVector<QVector<DltAnalyzerInterface::LogEntry>> m_chunks;

    QHash<int, BulkAnalysisResult> m_results;
    mutable QMutex m_mutex;
    QWaitCondition m_cv;
    bool m_paused = false;
    bool m_cancelled = false;
    bool m_started = false;

    int m_totalLogs = 0;
    int m_processedLogs = 0;
    QElapsedTimer m_timer;
};

class DltBulkAnalyzer : public QObject
{
    Q_OBJECT

public:
    explicit DltBulkAnalyzer(QObject *parent = nullptr);
    ~DltBulkAnalyzer();

    void setAnalyzer(DltAnalyzerInterface *analyzer);

    void startBulkAnalysis(const QVector<DltAnalyzerInterface::LogEntry> &logs,
                           int chunkSize = 100);

    void pauseAnalysis();
    void resumeAnalysis();
    void cancelAnalysis();

    bool isRunning() const;
    bool hasCompleted() const;

    double progress() const;
    const QHash<int, BulkAnalysisResult> &getResults() const;

    bool searchByTag(const QString &tag, QList<int> &outIndices) const;
    bool searchByCategory(const QString &category, QList<int> &outIndices) const;
    QString getSummaryForIndex(int index) const;
    QString getCategoryForIndex(int index) const;
    QSet<QString> getAllTags() const;
    QSet<QString> getAllCategories() const;

    void clearCache();

signals:
    void progressUpdated(double progress, int processed, int total);
    void analysisFinished(bool success);
    void errorOccurred(const QString &error);

private slots:
    void onWorkerProgress(double progress, int processed, int total);
    void onWorkerFinished();
    void onWorkerError(const QString &error);

private:
    DltBulkAnalyzerWorker *m_worker = nullptr;
    bool m_hasCompleted = false;
};

#endif
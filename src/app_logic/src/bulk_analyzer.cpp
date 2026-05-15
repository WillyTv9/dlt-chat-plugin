#include "dltchat/bulk_analyzer.h"
#include "dltchat/llm_analyzer_interface.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace dltchat {

class DltBulkAnalyzerWorker : public QObject
{
    Q_OBJECT
public:
    DltBulkAnalyzerWorker(DltLlmAnalyzerInterface *analyzer,
                          QVector<DltAnalyzerInterface::LogEntry> entries,
                          int chunkSize)
        : m_analyzer(analyzer), m_entries(std::move(entries)), m_chunkSize(chunkSize) {}

public slots:
    void process()
    {
        int total = m_entries.size();
        int processed = 0;

        while (processed < total && !m_cancelled)
        {
            if (m_paused)
            {
                QThread::msleep(100);
                continue;
            }

            int end = qMin(processed + m_chunkSize, total);
            QVector<DltAnalyzerInterface::LogEntry> chunk(
                m_entries.begin() + processed,
                m_entries.begin() + end);

            for (const auto &entry : chunk)
            {
                if (m_cancelled) break;
                QJsonObject result;
                result["index"] = entry.index;
                result["category"] = "unknown";
                result["summary"] = entry.payload.left(200);
                result["tags"] = QJsonArray();

                {
                    QMutexLocker lock(&m_resultsMutex);
                    m_results.append(result);
                }
            }

            processed = end;
            double progress = static_cast<double>(processed) / total;
            emit progressUpdated(progress, processed, total);
        }

        if (!m_cancelled)
            emit analysisFinished(true);
        else
            emit analysisFinished(false);
    }

    void pause() { m_paused = true; }
    void resume() { m_paused = false; }
    void cancel() { m_cancelled = true; }

    QVector<QJsonObject> results() const
    {
        QMutexLocker lock(&m_resultsMutex);
        return m_results;
    }

signals:
    void progressUpdated(double progress, int processed, int total);
    void analysisFinished(bool success);

private:
    DltLlmAnalyzerInterface *m_analyzer;
    QVector<DltAnalyzerInterface::LogEntry> m_entries;
    int m_chunkSize;
    volatile bool m_paused = false;
    volatile bool m_cancelled = false;
    QVector<QJsonObject> m_results;
    mutable QMutex m_resultsMutex;
};

DltBulkAnalyzer::DltBulkAnalyzer(QObject *parent)
    : QObject(parent)
    , m_worker(nullptr)
    , m_workerThread(nullptr)
    , m_running(false)
{
}

DltBulkAnalyzer::~DltBulkAnalyzer()
{
    cancelAnalysis();
}

void DltBulkAnalyzer::setAnalyzer(DltLlmAnalyzerInterface *analyzer)
{
    Q_UNUSED(analyzer)
}

void DltBulkAnalyzer::startBulkAnalysis(const QVector<DltAnalyzerInterface::LogEntry> &entries,
                                         int chunkSize)
{
    Q_UNUSED(chunkSize)
    QMutexLocker lock(&m_mutex);
    if (m_running) return;

    m_entries = entries;
    m_running = true;
}

void DltBulkAnalyzer::pauseAnalysis()
{
    if (m_worker) m_worker->pause();
}

void DltBulkAnalyzer::resumeAnalysis()
{
    if (m_worker) m_worker->resume();
}

void DltBulkAnalyzer::cancelAnalysis()
{
    QMutexLocker lock(&m_mutex);
    if (m_worker)
    {
        m_worker->cancel();
        if (m_workerThread)
        {
            m_workerThread->quit();
            m_workerThread->wait(5000);
        }
    }
    m_running = false;
}

bool DltBulkAnalyzer::isRunning() const
{
    QMutexLocker lock(&m_mutex);
    return m_running;
}

bool DltBulkAnalyzer::hasCompleted() const
{
    return !m_running && !m_entries.isEmpty();
}

void DltBulkAnalyzer::searchByTag(const QString &tag, QList<int> &results) const
{
    Q_UNUSED(tag)
    Q_UNUSED(results)
}

void DltBulkAnalyzer::searchByCategory(const QString &category, QList<int> &results) const
{
    Q_UNUSED(category)
    Q_UNUSED(results)
}

QString DltBulkAnalyzer::getCategoryForIndex(int logIndex) const
{
    Q_UNUSED(logIndex)
    return QString();
}

} // namespace dltchat

#include "bulk_analyzer.moc"

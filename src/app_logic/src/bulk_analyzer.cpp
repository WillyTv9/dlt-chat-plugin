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
        : m_analyzer(analyzer)
        , m_entries(std::move(entries))
        , m_chunkSize(chunkSize)
    {
        m_chunks.clear();
        for (int i = 0; i < m_entries.size(); i += m_chunkSize)
        {
            int end = qMin(i + m_chunkSize, m_entries.size());
            m_chunks.append(m_entries.mid(i, end - i));
        }
    }

    void setEntries(QVector<DltAnalyzerInterface::LogEntry> entries)
    {
        m_entries = std::move(entries);
        m_chunks.clear();
        for (int i = 0; i < m_entries.size(); i += m_chunkSize)
        {
            int end = qMin(i + m_chunkSize, m_entries.size());
            m_chunks.append(m_entries.mid(i, end - i));
        }
    }

    QHash<int, BulkAnalysisResult> results() const
    {
        QMutexLocker lock(&m_mutex);
        return m_results;
    }

    bool isPaused() const { return m_paused; }
    bool isCancelled() const { return m_cancelled; }

public slots:
    void process()
    {
        if (!m_analyzer)
        {
            emit errorOccurred("Bulk analyzer not configured");
            return;
        }

        int total = static_cast<int>(m_entries.size());
        if (total == 0)
        {
            emit analysisComplete();
            return;
        }

        int processed = 0;

        for (int ci = 0; ci < m_chunks.size(); ++ci)
        {
            if (m_cancelled) break;

            {
                QMutexLocker lock(&m_mutex);
                while (m_paused && !m_cancelled)
                    m_cv.wait(&m_mutex, 100);
            }
            if (m_cancelled) break;

            const auto &chunk = m_chunks[ci];
            if (chunk.isEmpty()) continue;

            QString query = "Analyze this log chunk. For each log entry, provide:\n"
                           "1. A brief category (error, warning, info, debug, etc.)\n"
                           "2. A one-line summary\n"
                           "3. Key tags (comma-separated)\n"
                           "Format per entry: [INDEX]|CATEGORY|SUMMARY|TAGS\n\n"
                           "Example: [42]|error|Memory allocation failed|memory,allocation,failure\n\n"
                           "Return the analysis for all entries.";

            DltAnalyzerInterface::QueryResult result = m_analyzer->analyzeQuery(query, chunk);

            if (result.success)
            {
                QString response = result.responseHtml;
                QStringList lines = response.split('\n', Qt::SkipEmptyParts);

                for (const QString &line : lines)
                {
                    QString trimmed = line.trimmed();
                    if (trimmed.isEmpty()) continue;

                    QStringList parts = trimmed.split('|');
                    if (parts.size() >= 4)
                    {
                        bool ok;
                        int idx = parts[0].remove('[').remove(']').toInt(&ok);
                        if (ok)
                        {
                            BulkAnalysisResult ar;
                            ar.logIndex = idx;
                            ar.category = parts[1].trimmed();
                            ar.summary = parts[2].trimmed();
                            for (const QString &tag : parts[3].split(',', Qt::SkipEmptyParts))
                                ar.tags.insert(tag.trimmed().toLower());

                            {
                                QMutexLocker lock(&m_mutex);
                                m_results.insert(idx, ar);
                            }
                        }
                    }
                }

                for (const auto &entry : chunk)
                {
                    QMutexLocker lock(&m_mutex);
                    if (!m_results.contains(entry.index))
                    {
                        BulkAnalysisResult ar;
                        ar.logIndex = entry.index;
                        ar.category = "unknown";
                        ar.summary = result.responseHtml.left(100);
                        m_results.insert(entry.index, ar);
                    }
                }
            }

            processed += chunk.size();
            double progress = total > 0 ? static_cast<double>(processed) / total : 0.0;
            emit progressChanged(progress, processed, total);
        }

        if (m_cancelled)
            emit errorOccurred("Analysis cancelled");
        else
            emit analysisComplete();
    }

    void pause()
    {
        QMutexLocker lock(&m_mutex);
        m_paused = true;
    }

    void resume()
    {
        QMutexLocker lock(&m_mutex);
        m_paused = false;
        m_cv.wakeAll();
    }

    void cancel()
    {
        QMutexLocker lock(&m_mutex);
        m_cancelled = true;
        m_cv.wakeAll();
    }

signals:
    void progressChanged(double progress, int processed, int total);
    void chunkProcessed(int chunkIndex, int entriesProcessed);
    void analysisComplete();
    void errorOccurred(const QString &error);

private:
    DltLlmAnalyzerInterface *m_analyzer;
    QVector<DltAnalyzerInterface::LogEntry> m_entries;
    QVector<QVector<DltAnalyzerInterface::LogEntry>> m_chunks;
    int m_chunkSize;
    QHash<int, BulkAnalysisResult> m_results;
    mutable QMutex m_mutex;
    QWaitCondition m_cv;
    volatile bool m_paused = false;
    volatile bool m_cancelled = false;
};

DltBulkAnalyzer::DltBulkAnalyzer(QObject *parent)
    : QObject(parent)
    , m_worker(nullptr)
    , m_workerThread(nullptr)
    , m_running(false)
    , m_hasCompleted(false)
    , m_llmAnalyzer(nullptr)
{
}

DltBulkAnalyzer::~DltBulkAnalyzer()
{
    cancelAnalysis();
}

void DltBulkAnalyzer::setAnalyzer(DltLlmAnalyzerInterface *analyzer)
{
    m_llmAnalyzer = analyzer;
}

void DltBulkAnalyzer::startBulkAnalysis(
    const QVector<DltAnalyzerInterface::LogEntry> &entries, int chunkSize)
{
    QMutexLocker lock(&m_mutex);
    if (m_running) return;

    m_hasCompleted = false;
    m_resultsCache.clear();

    if (!m_llmAnalyzer)
    {
        emit errorOccurred("Bulk analyzer: LLM analyzer not configured");
        return;
    }

    if (m_workerThread)
    {
        m_workerThread->quit();
        m_workerThread->wait(3000);
        delete m_workerThread;
        m_workerThread = nullptr;
    }

    m_worker = new DltBulkAnalyzerWorker(m_llmAnalyzer, entries, chunkSize);
    m_workerThread = new QThread(this);

    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started, m_worker, &DltBulkAnalyzerWorker::process);
    connect(m_worker, &DltBulkAnalyzerWorker::progressChanged,
            this, &DltBulkAnalyzer::onWorkerProgress);
    connect(m_worker, &DltBulkAnalyzerWorker::analysisComplete,
            this, &DltBulkAnalyzer::onWorkerFinished);
    connect(m_worker, &DltBulkAnalyzerWorker::errorOccurred,
            this, &DltBulkAnalyzer::onWorkerError);
    connect(m_worker, &DltBulkAnalyzerWorker::analysisComplete,
            m_workerThread, &QThread::quit);
    connect(m_worker, &DltBulkAnalyzerWorker::errorOccurred,
            m_workerThread, &QThread::quit);
    connect(m_workerThread, &QThread::finished,
            m_worker, &QObject::deleteLater);
    connect(m_workerThread, &QThread::finished,
            m_workerThread, &QObject::deleteLater);

    m_running = true;
    m_workerThread->start();
}

void DltBulkAnalyzer::pauseAnalysis()
{
    QMutexLocker lock(&m_mutex);
    if (m_worker)
        m_worker->pause();
}

void DltBulkAnalyzer::resumeAnalysis()
{
    QMutexLocker lock(&m_mutex);
    if (m_worker)
        m_worker->resume();
}

void DltBulkAnalyzer::cancelAnalysis()
{
    QMutexLocker lock(&m_mutex);
    if (m_worker)
        m_worker->cancel();

    if (m_workerThread)
    {
        m_workerThread->quit();
        if (!m_workerThread->wait(5000))
        {
            m_workerThread->terminate();
            m_workerThread->wait();
        }
    }

    m_worker = nullptr;
    m_workerThread = nullptr;
    m_running = false;
    m_resultsCache.clear();
}

bool DltBulkAnalyzer::isRunning() const
{
    QMutexLocker lock(&m_mutex);
    return m_running;
}

bool DltBulkAnalyzer::hasCompleted() const
{
    QMutexLocker lock(&m_mutex);
    return m_hasCompleted;
}

void DltBulkAnalyzer::searchByTag(const QString &tag, QList<int> &results) const
{
    QMutexLocker lock(&m_mutex);
    QString lowerTag = tag.toLower();
    for (auto it = m_resultsCache.constBegin(); it != m_resultsCache.constEnd(); ++it)
    {
        if (it.value().tags.contains(lowerTag))
            results.append(it.key());
    }
}

void DltBulkAnalyzer::searchByCategory(const QString &category, QList<int> &results) const
{
    QMutexLocker lock(&m_mutex);
    QString lowerCat = category.toLower();
    for (auto it = m_resultsCache.constBegin(); it != m_resultsCache.constEnd(); ++it)
    {
        if (it.value().category.toLower().contains(lowerCat))
            results.append(it.key());
    }
}

QString DltBulkAnalyzer::getCategoryForIndex(int logIndex) const
{
    QMutexLocker lock(&m_mutex);
    auto it = m_resultsCache.constFind(logIndex);
    if (it != m_resultsCache.constEnd())
        return it.value().category;
    return QString();
}

void DltBulkAnalyzer::onWorkerProgress(double progress, int processed, int total)
{
    emit progressUpdated(progress, processed, total);
}

void DltBulkAnalyzer::onWorkerFinished()
{
    QMutexLocker lock(&m_mutex);
    if (m_worker)
    {
        m_resultsCache = m_worker->results();
        m_hasCompleted = true;
    }
    m_running = false;
    m_worker = nullptr;
    m_workerThread = nullptr;
    emit analysisFinished(true);
}

void DltBulkAnalyzer::onWorkerError(const QString &error)
{
    QMutexLocker lock(&m_mutex);
    m_resultsCache.clear();
    m_hasCompleted = false;
    m_running = false;
    m_worker = nullptr;
    m_workerThread = nullptr;
    emit errorOccurred(error);
    emit analysisFinished(false);
}

} // namespace dltchat

#include "bulk_analyzer.moc"

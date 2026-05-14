#include "dltbulkanalyzer.h"

#include <QRandomGenerator>
#include <QDebug>

DltBulkAnalyzerWorker::DltBulkAnalyzerWorker(QObject *parent)
    : QThread(parent)
{
}

DltBulkAnalyzerWorker::~DltBulkAnalyzerWorker()
{
    cancel();
    wait();
}

void DltBulkAnalyzerWorker::configure(DltAnalyzerInterface *analyzer,
                                      int chunkSize,
                                      int maxRetries)
{
    QMutexLocker lock(&m_mutex);
    m_analyzer = analyzer;
    m_chunkSize = chunkSize;
    m_maxRetries = maxRetries;
}

void DltBulkAnalyzerWorker::enqueueLogs(const QVector<DltAnalyzerInterface::LogEntry> &logs)
{
    QMutexLocker lock(&m_mutex);
    m_logs = logs;
    m_totalLogs = logs.size();

    m_chunks.clear();
    for (int i = 0; i < logs.size(); i += m_chunkSize)
    {
        int end = qMin(i + m_chunkSize, logs.size());
        m_chunks.append(logs.mid(i, end - i));
    }
}

void DltBulkAnalyzerWorker::processAsync()
{
    if (m_started) return;
    m_started = true;
    m_timer.start();
    start();
}

double DltBulkAnalyzerWorker::progress() const
{
    QMutexLocker lock(&m_mutex);
    if (m_totalLogs == 0) return 0.0;
    return static_cast<double>(m_processedLogs) / m_totalLogs;
}

bool DltBulkAnalyzerWorker::waitIfPaused()
{
    QMutexLocker lock(&m_mutex);
    while (m_paused && !m_cancelled)
    {
        m_cv.wait(&m_mutex);
    }
    return m_cancelled;
}

void DltBulkAnalyzerWorker::run()
{
    if (!m_analyzer)
    {
        emit errorOccurred("Analyzer not configured");
        return;
    }

    {
        QMutexLocker lock(&m_mutex);
        if (m_chunks.isEmpty())
        {
            emit analysisComplete();
            return;
        }
    }

    for (int i = 0; i < m_chunks.size(); ++i)
    {
        if (m_cancelled) break;
        if (waitIfPaused()) break;

        QVector<DltAnalyzerInterface::LogEntry> chunk;
        {
            QMutexLocker lock(&m_mutex);
            chunk = m_chunks[i];
        }

        bool success = processChunk(i, chunk, m_maxRetries);

        if (!success && !m_cancelled)
        {
            qWarning() << "Failed to process chunk" << i;
        }

        {
            QMutexLocker lock(&m_mutex);
            m_processedLogs += chunk.size();
            double prog = m_totalLogs > 0 ? static_cast<double>(m_processedLogs) / m_totalLogs : 0.0;
            emit progressChanged(prog, m_processedLogs, m_totalLogs);
        }
    }

    if (m_cancelled)
    {
        emit errorOccurred("Analysis cancelled");
    }
    else
    {
        emit analysisComplete();
    }
}

bool DltBulkAnalyzerWorker::processChunk(int chunkIndex,
                                          const QVector<DltAnalyzerInterface::LogEntry> &chunk,
                                          int maxRetries)
{
    if (chunk.isEmpty()) return true;

    QString query = "Analyze this log chunk. For each log entry, provide:\n"
                   "1. A brief category (error, warning, info, debug, etc.)\n"
                   "2. A one-line summary\n"
                   "3. Key tags (comma-separated)\n"
                   "Format per entry: [INDEX]|CATEGORY|SUMMARY|TAGS\n\n"
                   "Example: [42]|error|Memory allocation failed|memory,allocation,failure\n\n"
                   "Return the analysis for all entries.";

    int attempt = 0;
    while (attempt <= maxRetries)
    {
        if (m_cancelled) return false;

        DltAnalyzerInterface::QueryResult result = m_analyzer->analyzeQuery(query, chunk);

        if (result.success)
        {
            QMutexLocker lock(&m_mutex);

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
                    int index = parts[0].replace("[", "").replace("]", "").toInt(&ok);
                    if (ok)
                    {
                        BulkAnalysisResult ar;
                        ar.logIndex = index;
                        ar.category = parts[1].trimmed();
                        ar.summary = parts[2].trimmed();

                        QStringList tagList = parts[3].split(',', Qt::SkipEmptyParts);
                        for (const QString &tag : tagList)
                        {
                            ar.tags.insert(tag.trimmed().toLower());
                        }
                        ar.processed = true;

                        m_results[index] = ar;
                    }
                }
            }

            for (const auto &entry : chunk)
            {
                if (!m_results.contains(entry.index))
                {
                    BulkAnalysisResult ar;
                    ar.logIndex = entry.index;
                    ar.category = "unknown";
                    ar.summary = result.responseHtml.left(100);
                    ar.processed = true;
                    m_results[entry.index] = ar;
                }
            }

            return true;
        }

        attempt++;

        if (attempt <= maxRetries)
        {
            int baseDelay = 1000 * (1 << (attempt - 1));
            int jitter = QRandomGenerator::global()->bounded(0, baseDelay / 2);
            QThread::sleep((baseDelay + jitter) / 1000);
        }
    }

    return false;
}

DltBulkAnalyzer::DltBulkAnalyzer(QObject *parent)
    : QObject(parent)
    , m_worker(new DltBulkAnalyzerWorker(nullptr))
{
    connect(m_worker, &DltBulkAnalyzerWorker::progressChanged,
            this, &DltBulkAnalyzer::onWorkerProgress);
    connect(m_worker, &DltBulkAnalyzerWorker::analysisComplete,
            this, &DltBulkAnalyzer::onWorkerFinished);
    connect(m_worker, &DltBulkAnalyzerWorker::errorOccurred,
            this, &DltBulkAnalyzer::onWorkerError);
}

DltBulkAnalyzer::~DltBulkAnalyzer()
{
    cancelAnalysis();
    delete m_worker;
    m_worker = nullptr;
}

void DltBulkAnalyzer::setAnalyzer(DltAnalyzerInterface *analyzer)
{
    if (m_worker)
        m_worker->configure(analyzer);
}

void DltBulkAnalyzer::startBulkAnalysis(const QVector<DltAnalyzerInterface::LogEntry> &logs,
                                         int chunkSize)
{
    if (m_worker->isRunning())
    {
        qWarning() << "Bulk analysis already in progress";
        return;
    }

    m_hasCompleted = false;
    m_worker->enqueueLogs(logs);
    m_worker->processAsync();
}

void DltBulkAnalyzer::pauseAnalysis()
{
    m_worker->pause();
}

void DltBulkAnalyzer::resumeAnalysis()
{
    m_worker->resume();
}

void DltBulkAnalyzer::cancelAnalysis()
{
    if (!m_worker) return;
    m_worker->cancel();
    m_worker->wait(5000);
}

bool DltBulkAnalyzer::isRunning() const
{
    return m_worker->isRunning();
}

bool DltBulkAnalyzer::hasCompleted() const
{
    return m_hasCompleted;
}

double DltBulkAnalyzer::progress() const
{
    return m_worker->progress();
}

const QHash<int, BulkAnalysisResult> &DltBulkAnalyzer::getResults() const
{
    return m_worker->results();
}

bool DltBulkAnalyzer::searchByTag(const QString &tag, QList<int> &outIndices) const
{
    const QHash<int, BulkAnalysisResult> &results = m_worker->results();
    QString lowerTag = tag.toLower();

    for (auto it = results.constBegin(); it != results.constEnd(); ++it)
    {
        if (it.value().tags.contains(lowerTag))
        {
            outIndices.append(it.key());
        }
    }

    return !outIndices.isEmpty();
}

bool DltBulkAnalyzer::searchByCategory(const QString &category, QList<int> &outIndices) const
{
    const QHash<int, BulkAnalysisResult> &results = m_worker->results();
    QString lowerCat = category.toLower();

    for (auto it = results.constBegin(); it != results.constEnd(); ++it)
    {
        if (it.value().category.toLower().contains(lowerCat))
        {
            outIndices.append(it.key());
        }
    }

    return !outIndices.isEmpty();
}

QString DltBulkAnalyzer::getSummaryForIndex(int index) const
{
    const QHash<int, BulkAnalysisResult> &results = m_worker->results();
    if (results.contains(index))
    {
        return results[index].summary;
    }
    return QString();
}

QString DltBulkAnalyzer::getCategoryForIndex(int index) const
{
    const QHash<int, BulkAnalysisResult> &results = m_worker->results();
    if (results.contains(index))
    {
        return results[index].category;
    }
    return QString();
}

QSet<QString> DltBulkAnalyzer::getAllTags() const
{
    const QHash<int, BulkAnalysisResult> &results = m_worker->results();
    QSet<QString> allTags;

    for (auto it = results.constBegin(); it != results.constEnd(); ++it)
    {
        allTags.unite(it.value().tags);
    }

    return allTags;
}

QSet<QString> DltBulkAnalyzer::getAllCategories() const
{
    const QHash<int, BulkAnalysisResult> &results = m_worker->results();
    QSet<QString> allCategories;

    for (auto it = results.constBegin(); it != results.constEnd(); ++it)
    {
        QString cat = it.value().category;
        if (!cat.isEmpty() && cat != "unknown")
        {
            allCategories.insert(cat);
        }
    }

    return allCategories;
}

void DltBulkAnalyzer::onWorkerProgress(double progress, int processed, int total)
{
    emit progressUpdated(progress, processed, total);
}

void DltBulkAnalyzer::onWorkerFinished()
{
    m_hasCompleted = true;
    emit analysisFinished(true);
}

void DltBulkAnalyzer::onWorkerError(const QString &error)
{
    emit errorOccurred(error);
    emit analysisFinished(false);
}
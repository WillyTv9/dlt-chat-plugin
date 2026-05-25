#ifndef TEST_ENHANCED_RETRIEVER_H
#define TEST_ENHANCED_RETRIEVER_H

#include <QObject>
#include <QString>

#include "dltchat/analyzer_interface.h"

class TestEnhancedRetriever : public QObject
{
    Q_OBJECT

private:
    dltchat::DltAnalyzerInterface::LogEntry makeEntry(int idx,
                                                      const QString &payload,
                                                      const QString &cat = QString(),
                                                      const QString &apid = QString()) const;

private slots:
    void init();
    void cleanup();

    void testEmptyEntries();
    void testTokenMatchesRetrieved();
    void testDiversityPenalty();
    void testIdfWeighting();
};

#endif

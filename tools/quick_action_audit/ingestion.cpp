#include "ingestion.h"
#include <QDebug>
#include <qdltfile.h>
#include <qdltmsg.h>
#include "dltchat/automotive_log_parser.h"
#include "dltchat/analyzer_interface.h"

using namespace dltchat;

static DltAnalyzerInterface::LogEntry msgToEntry(int index, QDltMsg &msg)
{
    DltAnalyzerInterface::LogEntry e;
    e.index     = index;
    e.time      = QString("%1.%2").arg(msg.getTimeString())
                                  .arg(msg.getMicroseconds(), 6, 10, QLatin1Char('0'));
    e.timestamp = QString("%1.%2").arg(msg.getTimestamp() / 10000)
                                  .arg(msg.getTimestamp() % 10000, 4, 10, QLatin1Char('0'));
    e.ecu     = msg.getEcuid();
    e.apid    = msg.getApid();
    e.ctid    = msg.getCtid();
    e.level   = msg.getSubtypeString().toLower();
    e.payload = DltRuleBasedAnalyzer::simplifyPayload(msg.toStringPayload());
    AutomotiveLogParser::classify(e);
    return e;
}

QVector<DltAnalyzerInterface::LogEntry> ingestFile(
    QDltFile &dltFile, int ingestCount, QMap<QString, int> &levelCounts)
{
    QVector<DltAnalyzerInterface::LogEntry> entries;
    entries.reserve(ingestCount);
    for (int i = 0; i < ingestCount; ++i) {
        QDltMsg msg;
        if (!dltFile.getMsg(i, msg))
            continue;
        auto e = msgToEntry(i, msg);
        levelCounts[e.level]++;
        entries.append(e);
        if (i > 0 && i % 100000 == 0)
            qInfo() << "  ingested" << i << "/" << ingestCount;
    }
    return entries;
}

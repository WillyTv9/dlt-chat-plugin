#include <QCoreApplication>
#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QDebug>

#include <qdltfile.h>

#include "dltchat/category_registry.h"
#include "ingestion.h"
#include "audit.h"
#include "report.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("quick_action_audit");

    QCommandLineParser cli;
    cli.addHelpOption();
    cli.addOption({"dlt",      "DLT file to audit",                                            "file", "logs_decoded.dlt"});
    cli.addOption({"registry", "Path to category_registry.json (uses Qt resource if omitted)", "file"});
    cli.addOption({"out",      "Output report path",                                            "file", "quick_action_audit_report.md"});
    cli.addOption({"limit",    "Max messages to ingest (0 = all)",                              "N",    "0"});
    cli.process(app);

    const QString dltPath = cli.value("dlt");
    const QString regPath = cli.value("registry");
    const QString outPath = cli.value("out");
    const int     limit   = cli.value("limit").toInt();

    auto &reg = dltchat::CategoryRegistry::instance();
    if (!regPath.isEmpty() && !reg.loadFromFile(regPath)) {
        qCritical() << "Failed to load registry from:" << regPath;
        return 1;
    }
    if (reg.quickActions().isEmpty()) {
        qCritical() << "Registry empty. Pass --registry path/to/category_registry.json";
        return 1;
    }
    qInfo() << "Registry loaded:" << reg.quickActions().size() << "quick actions";

    QDltFile dltFile;
    if (!dltFile.open(dltPath)) {
        qCritical() << "Cannot open DLT file:" << dltPath;
        return 1;
    }
    if (!dltFile.createIndex()) {
        qCritical() << "Failed to index DLT file:" << dltPath;
        return 1;
    }
    const int total       = dltFile.size();
    const int ingestCount = (limit > 0 && limit < total) ? limit : total;
    qInfo() << "DLT file:" << dltPath << "| total:" << total << "| ingesting:" << ingestCount;

    QMap<QString, int> levelCounts;
    QElapsedTimer ingestTimer;
    ingestTimer.start();
    const auto entries = ingestFile(dltFile, ingestCount, levelCounts);
    qInfo() << "Ingestion done in" << ingestTimer.elapsed() << "ms |" << entries.size() << "entries";

    qInfo() << "\nRunning" << reg.quickActions().size() << "quick actions...";
    const auto rows         = runQuickActions(entries);
    const auto collisions   = findAliasCollisions();
    const auto shortFilters = findAmbiguousFilters();

    writeReport(outPath, dltPath, total, entries, levelCounts, rows, collisions, shortFilters);
    return 0;
}

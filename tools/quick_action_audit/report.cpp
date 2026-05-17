#include "report.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>

using namespace dltchat;

void writeReport(
    const QString &outPath,
    const QString &dltPath,
    int total,
    const QVector<DltAnalyzerInterface::LogEntry> &entries,
    const QMap<QString, int> &levelCounts,
    const QVector<AuditRow> &rows,
    const QVector<Collision> &collisions,
    const QVector<ShortFilter> &shortFilters)
{
    QFile outFile(outPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Cannot write report to" << outPath;
        return;
    }
    QTextStream out(&outFile);

    out << "# Quick Action Audit Report\n\n";
    out << "**DLT file:** " << dltPath << "  \n";
    out << "**Messages ingested:** " << entries.size() << " / " << total << "  \n";
    out << "**Quick actions tested:** " << rows.size() << "  \n\n";

    out << "## Level Breakdown\n\n";
    for (auto it = levelCounts.constBegin(); it != levelCounts.constEnd(); ++it)
        out << "- **" << it.key() << "**: " << it.value() << "\n";
    out << "\n";

    out << "## Quick Action Results\n\n";
    out << "| Label | Query | Kind | Target ID | Matches | % total | ms | Notes / Samples |\n";
    out << "|-------|-------|------|-----------|---------|---------|----|-----------------|\n";
    for (const auto &r : rows) {
        QString sampleStr = r.samples.join("; ").left(120);
        sampleStr.replace("|", "\\|").replace("\n", " ").replace("\r", "");
        QString notes = r.isError      ? ("⚠️ " + r.note)
                      : r.matchCount == 0 ? "⚪ zero results"
                      : !r.note.isEmpty()  ? r.note
                      : sampleStr;
        out << "| " << r.label
            << " | `" << r.query << "`"
            << " | " << r.kind
            << " | " << r.target
            << " | " << r.matchCount
            << " | " << QString::number(r.pctOfTotal, 'f', 2) << "%"
            << " | " << r.timeMs
            << " | " << notes
            << " |\n";
    }
    out << "\n> ⚠️ = broken (resolves to Unknown)  |  ⚪ = zero results on this log file\n\n";

    out << "## Static Audit: Alias Collisions\n\n";
    out << "Aliases shared by multiple categories — `resolveQuery` returns last-inserted (QHash last-write-wins).\n\n";
    if (collisions.isEmpty()) {
        out << "_No alias collisions found._\n\n";
    } else {
        out << "| Alias | All categories | resolveQuery winner |\n";
        out << "|-------|---------------|---------------------|\n";
        for (const auto &c : collisions)
            out << "| `" << c.alias << "` | " << c.allCats.join(", ")
                << " | **" << c.winner << "** |\n";
        out << "\n";
    }

    out << "## Static Audit: Ambiguous Filter Tokens\n\n";
    out << "`fieldMatchesFilter` uses substring `contains()` — tokens ≤2 chars or common words cause false positives.\n\n";
    if (shortFilters.isEmpty()) {
        out << "_No ambiguous filter tokens found._\n\n";
    } else {
        out << "| Category | Token | Length |\n";
        out << "|----------|-------|--------|\n";
        for (const auto &sf : shortFilters)
            out << "| " << sf.catId << " | `" << sf.token << "` | " << sf.token.length() << " |\n";
        out << "\n";
    }

    out << "## Zero-Match Quick Actions\n\n";
    bool anyZero = false;
    for (const auto &r : rows) {
        if (!r.isError && r.matchCount == 0) {
            out << "- **" << r.label << "** (`" << r.query << "`) → "
                << r.kind << " / " << r.target
                << " — 0 results (no matching entries in this log)\n";
            anyZero = true;
        }
    }
    if (!anyZero)
        out << "_All quick actions returned at least one result._\n";
    out << "\n";

    qInfo() << "Report written to:" << outPath;
}

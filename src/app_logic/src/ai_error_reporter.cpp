#include "dltchat/ai_error_reporter.h"

#include <QLocale>
#include <QString>
#include <QStringLiteral>

namespace dltchat {

namespace {

QString formatThousands(int value)
{
    return QLocale::c().toString(value);
}

} // namespace

QString AiErrorReporter::deriveHint(const QString &stage, const QString &cause)
{
    Q_UNUSED(stage);
    const QString c = cause.toLower();

    if (c.contains(QStringLiteral("401")) || c.contains(QStringLiteral("unauthorized"))) {
        return QStringLiteral("Re-run Copilot device-flow login from Options dialog");
    }
    if (c.contains(QStringLiteral("timeout"))) {
        return QStringLiteral("Increase llmTimeout in dlt_chat_plugin.ini (current default 120000ms)");
    }
    if (c.contains(QStringLiteral("rate"))) {
        return QStringLiteral("Reduce concurrency or wait; provider rate-limit hit");
    }
    if (c.contains(QStringLiteral("bad_alloc")) || c.contains(QStringLiteral("memory"))) {
        return QStringLiteral("Reduce blockSize in dlt_chat_plugin.ini (current default 5000) or close other apps");
    }
    if (c.contains(QStringLiteral("cancelled"))) {
        return QStringLiteral("Pipeline cancelled \xE2\x80\x94 start a new query");
    }
    if (c.contains(QStringLiteral("circuit")) || c.contains(QStringLiteral("breaker"))) {
        return QStringLiteral("Circuit breaker open \xE2\x80\x94 wait 60s or fix root cause then retry");
    }
    if (c.contains(QStringLiteral("empty")) || c.contains(QStringLiteral("null"))) {
        return QStringLiteral("Verify a log file is loaded and indexing pipeline has completed");
    }
    return QStringLiteral("Check qInfo logs (filter category 'dltchat.*') for full trace");
}

QString AiErrorReporter::formatHtml(const QString &stage, const QString &cause, const Context &ctx)
{
    QString html;
    html.reserve(512);
    html += QStringLiteral("<div style=\"color:#a00;font-family:monospace;border-left:3px solid #a00;padding-left:8px\">");

    const QString stageEsc = stage.toHtmlEscaped();
    const QString causeEsc = cause.toHtmlEscaped();
    html += QStringLiteral("<b>[") + stageEsc + QStringLiteral("] ") + causeEsc + QStringLiteral("</b>");

    if (!ctx.provider.isEmpty()) {
        html += QStringLiteral("<br>provider: ") + ctx.provider.toHtmlEscaped();
    }
    if (!ctx.model.isEmpty()) {
        html += QStringLiteral("<br>model: ") + ctx.model.toHtmlEscaped();
    }
    if (ctx.budgetChars > 0) {
        html += QStringLiteral("<br>budgetChars: ") + formatThousands(ctx.budgetChars);
    }
    if (ctx.entryCount > 0) {
        html += QStringLiteral("<br>entryCount: ") + formatThousands(ctx.entryCount);
    }
    if (ctx.shardIdx >= 0 && ctx.shardTotal > 0) {
        html += QStringLiteral("<br>shard: ") + QString::number(ctx.shardIdx + 1)
              + QStringLiteral("/") + QString::number(ctx.shardTotal);
    }
    if (!ctx.diagnostic.isEmpty()) {
        html += QStringLiteral("<br>diagnostic: <code style=\"word-wrap:break-word;white-space:pre-wrap\">")
              + ctx.diagnostic.toHtmlEscaped()
              + QStringLiteral("</code>");
    }

    const QString hint = ctx.hint.isEmpty() ? deriveHint(stage, cause) : ctx.hint;
    html += QStringLiteral("<br>hint: ") + hint.toHtmlEscaped();

    html += QStringLiteral("</div>");
    return html;
}

} // namespace dltchat

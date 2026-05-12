#include "chatform.h"

#include <QCoreApplication>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMap>
#include <QProgressBar>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QGroupBox>
#include <QFont>
#include <QPalette>
#include <QMessageBox>

namespace DltChat {

static constexpr int kMaxInputLength = 500;

static QString btnColorStyle(bool dark, const QString &bg, const QString &fg,
                             const QString &hoverBg, int borderWidth = 2)
{
    QString darker = QColor(bg).darker(dark ? 130 : 120).name();
    QString pressed = QColor(hoverBg).darker(110).name();
    return QString(
        "QPushButton{padding:2px 6px;border:%6px solid %1;border-radius:3px;"
        "background:%2;color:%3;font-size:11px;font-weight:bold;}"
        "QPushButton:hover{background:%4;}"
        "QPushButton:pressed{background:%5;}"
    ).arg(darker).arg(bg).arg(fg).arg(hoverBg).arg(pressed).arg(borderWidth);
}

static QPushButton *makeBtn(const QString &text, const QString &tip, QWidget *parent)
{
    auto *b = new QPushButton(text, parent);
    b->setToolTip(tip);
    b->setMinimumHeight(24);
    return b;
}

static void applyBtnStyle(QPushButton *b, bool dark, const QString &bg,
                          const QString &fg, const QString &hoverBg)
{
    if (b) b->setStyleSheet(btnColorStyle(dark, bg, fg, hoverBg));
}

// Color definitions per button role
struct BtnColors { QString bg, fg, hover; };

static BtnColors sendColors(bool dark)
{
    return dark
        ? BtnColors{"#1565c0", "#ffffff", "#1976d2"}
        : BtnColors{"#1565c0", "#ffffff", "#0d47a1"};
}

static BtnColors aiSendColors(bool dark)
{
    return dark
        ? BtnColors{"#2e7d32", "#ffffff", "#388e3c"}
        : BtnColors{"#2e7d32", "#ffffff", "#1b5e20"};
}

static BtnColors clearColors(bool dark)
{
    return dark
        ? BtnColors{"#c62828", "#ffffff", "#d32f2f"}
        : BtnColors{"#c62828", "#ffffff", "#b71c1c"};
}

static BtnColors exportColors(bool dark)
{
    return dark
        ? BtnColors{"#e65100", "#ffffff", "#ef6c00"}
        : BtnColors{"#e65100", "#ffffff", "#bf360c"};
}

static BtnColors filterColors(bool dark)
{
    return dark
        ? BtnColors{"#7b1fa2", "#ffffff", "#8e24aa"}
        : BtnColors{"#7b1fa2", "#ffffff", "#6a1b9a"};
}

static BtnColors helpColors(bool)
{
    return BtnColors{"#616161", "#ffffff", "#757575"};
}

static BtnColors levelColors(bool dark)
{
    return dark
        ? BtnColors{"#1e3a5f", "#e3f2fd", "#2a4f82"}
        : BtnColors{"#bbdefb", "#1565c0", "#90caf9"};
}

static BtnColors warnColors(bool dark)
{
    return dark
        ? BtnColors{"#5f3a1e", "#fff3e0", "#824f2a"}
        : BtnColors{"#ffe0b2", "#e65100", "#ffcc80"};
}

static BtnColors errorColors(bool dark)
{
    return dark
        ? BtnColors{"#5f1e1e", "#ffebee", "#822a2a"}
        : BtnColors{"#ffcdd2", "#c62828", "#ef9a9a"};
}

static BtnColors infoColors(bool dark)
{
    return dark
        ? BtnColors{"#1e3a5f", "#e3f2fd", "#2a4f82"}
        : BtnColors{"#bbdefb", "#1565c0", "#90caf9"};
}

static BtnColors debugColors(bool dark)
{
    return dark
        ? BtnColors{"#424242", "#eeeeee", "#616161"}
        : BtnColors{"#e0e0e0", "#424242", "#bdbdbd"};
}

static BtnColors systemColors(bool dark)
{
    return dark
        ? BtnColors{"#1b3b1b", "#e8f5e9", "#2a5e2a"}
        : BtnColors{"#c8e6c9", "#2e7d32", "#a5d6a7"};
}

static BtnColors analysisColors(bool dark)
{
    return dark
        ? BtnColors{"#3a1b4b", "#f3e5f5", "#5e2a82"}
        : BtnColors{"#e1bee7", "#7b1fa2", "#ce93d8"};
}

static BtnColors automotiveColors(bool dark)
{
    return dark
        ? BtnColors{"#1b3a4b", "#e0f7fa", "#2a5e82"}
        : BtnColors{"#b2ebf2", "#00838f", "#80deea"};
}

Form::Form(QWidget *parent)
    : QWidget(parent)
    , statusLabel(new QLabel(this))
    , aiStatusLabel(new QLabel(this))
    , configButton(new QPushButton(QString::fromUtf8("\u2699"), this))
    , history(new QTextBrowser(this))
    , resultsList(new QListWidget(this))
    , input(new QLineEdit(this))
    , sendButton(new QPushButton(tr("Send"), this))
    , aiInput(new QLineEdit(this))
    , aiSendButton(new QPushButton(tr("Ask AI"), this))
    , aiProgress(new QProgressBar(this))
    , clearButton(new QPushButton(tr("Clear"), this))
    , exportCsvButton(new QPushButton(tr("CSV"), this))
    , exportAllButton(new QPushButton(tr("CSV All"), this))
    , filterLoadButton(new QPushButton(tr("Filters"), this))
    , aiDisclaimer(nullptr)
{
    bool dark = palette().color(QPalette::Window).lightness() < 128;
    QColor hl = palette().color(QPalette::Highlight);
    QColor fg = palette().color(QPalette::WindowText);
    QColor bg = palette().color(QPalette::Base);
    QColor mid = palette().color(QPalette::Mid);
    QString bgS = bg.name(), fgS = fg.name(), hlS = hl.name(), midS = mid.name();

    title = new QLabel(tr("Chat Log Assistant"), this);
    QFont tf = title->font(); tf.setBold(true); tf.setPointSize(tf.pointSize() + 1);
    title->setFont(tf);
    title->setStyleSheet(QString("color:%1;padding:0;margin:0;").arg(hlS));

    statusLabel->setWordWrap(true);
    statusLabel->setStyleSheet(QString("font-size:11px;color:%1;padding:0;margin:0;").arg(fgS));
    statusLabel->setText(tr("No log loaded."));

    configButton->setToolTip(tr("Configure AI"));
    configButton->setFixedSize(28, 24);
    configButton->setStyleSheet(QString(
        "QPushButton{border:none;font-size:14px;color:%1;}"
        "QPushButton:hover{color:%2;}"
    ).arg(fgS).arg(hlS));

    aiStatusLabel->setStyleSheet(QString("font-size:11px;color:%1;padding:0;").arg(dark?"#9e9e9e":"#757575"));
    aiStatusLabel->setText(tr("AI: -"));

    history->setReadOnly(true);
    history->setStyleSheet(QString(
        "QTextBrowser{background:%1;color:%2;border:1px solid %3;border-radius:4px;}"
    ).arg(bgS).arg(fgS).arg(midS));

    resultsList->setSelectionMode(QAbstractItemView::SingleSelection);
    resultsList->setStyleSheet(QString(
        "QListWidget{background:%1;color:%2;border:1px solid %3;border-radius:4px;}"
        "QListWidget::item{padding:2px 4px;}"
        "QListWidget::item:selected{background:%4;color:%5;}"
    ).arg(bgS).arg(fgS).arg(midS).arg(hlS).arg(palette().color(QPalette::HighlightedText).name()));
    resultsList->setMaximumHeight(180);

    input->setPlaceholderText(tr("Ask about logs..."));
    input->setMaxLength(kMaxInputLength);
    input->setStyleSheet(QString(
        "QLineEdit{padding:4px;border:1px solid %1;border-radius:4px;background:%2;color:%3;}"
    ).arg(midS).arg(bgS).arg(fgS));
    aiInput->setPlaceholderText(tr("Ask the AI..."));
    aiInput->setMaxLength(kMaxInputLength);
    aiInput->setStyleSheet(input->styleSheet());

    aiProgress->setRange(0, 0);
    aiProgress->setFixedHeight(4);
    aiProgress->hide();

    // Quick actions - 3x7 grid (20 buttons)
    QGroupBox *quickBox = new QGroupBox(tr("Quick Actions"), this);
    QGridLayout *ql = new QGridLayout();
    ql->setSpacing(2);

    struct BtnDef {
        const char *text;
        const char *tip;
        BtnColors (*getColors)(bool);
    };

    BtnDef bd[] = {
        {"Errors",      "Errors and fatals",       errorColors},
        {"Warnings",    "Warning messages",        warnColors},
        {"Info",        "Informational messages",  infoColors},
        {"Debug",       "Debug messages",          debugColors},
        {"CAN",         "CAN bus messages",        systemColors},
        {"Security",    "Auth and security",       systemColors},
        {"Memory",      "Memory issues",           systemColors},
        {"Performance", "Timeouts and delays",     systemColors},
        {"Diagnostic",  "DTC diagnostics",         systemColors},
        {"Pattern",     "Repeated messages",       analysisColors},
        {"Summary",     "Log statistics",          analysisColors},
        {"Timeline",    "Chronological order",     analysisColors},
        {"GPS",         "Navigation and GPS",      systemColors},
        {"Help",        "Show available commands", helpColors},
        {"CarPlay",     "CarPlay session events",  automotiveColors},
        {"AndroidAuto", "Android Auto events",     automotiveColors},
        {"Focus",       "Video Focus Lost",        automotiveColors},
        {"Ducking",     "Audio Ducking events",    automotiveColors},
        {"mDNS",        "mDNS Handshake events",   automotiveColors},
        {"Sensor",      "Vehicle Sensor Data",     automotiveColors},
    };
    int totalButtons = sizeof(bd) / sizeof(bd[0]);
    for (int i = 0; i < totalButtons; ++i)
    {
        auto *b = makeBtn(tr(bd[i].text), tr(bd[i].tip), this);
        auto c = bd[i].getColors(dark);
        applyBtnStyle(b, dark, c.bg, c.fg, c.hover);
        connect(b, &QPushButton::clicked, this, &Form::onQuickActionClicked);
        ql->addWidget(b, i / 7, i % 7);
    }
    quickBox->setLayout(ql);

    // Header: title + AI status + config button
    QHBoxLayout *hh = new QHBoxLayout();
    hh->setContentsMargins(0,0,0,0);
    hh->addWidget(title, 1);
    hh->addWidget(aiStatusLabel);
    hh->addWidget(configButton);

    // Query input row
    QHBoxLayout *il = new QHBoxLayout();
    il->addWidget(input, 1); il->addWidget(sendButton);

    // AI section
    QVBoxLayout *aiL = new QVBoxLayout();
    QHBoxLayout *aiRow = new QHBoxLayout();
    aiRow->addWidget(aiInput, 1); aiRow->addWidget(aiSendButton);
    aiL->addLayout(aiRow);
    aiL->addWidget(aiProgress);

    aiDisclaimer = new QLabel(tr("AI performance depends on the host system hardware and model size."), this);
    aiDisclaimer->setStyleSheet(QString(
        "font-size:8px;color:%1;font-style:italic;padding:0;margin:0;"
    ).arg(midS));
    aiDisclaimer->setWordWrap(true);
    aiL->addWidget(aiDisclaimer);
    QGroupBox *aiBox = new QGroupBox(tr("AI")); aiBox->setLayout(aiL);

    // Bottom buttons
    QHBoxLayout *bl = new QHBoxLayout();
    bl->addWidget(clearButton); bl->addStretch();
    bl->addWidget(filterLoadButton);
    bl->addWidget(exportCsvButton); bl->addWidget(exportAllButton);

    // Main layout - simple vertical
    QVBoxLayout *ml = new QVBoxLayout(this);
    ml->setContentsMargins(4, 2, 4, 4);
    ml->setSpacing(4);
    ml->addLayout(hh);
    ml->addWidget(statusLabel);
    ml->addWidget(quickBox);
    ml->addWidget(history, 1);
    ml->addWidget(resultsList);
    ml->addLayout(il);
    ml->addWidget(aiBox);
    ml->addLayout(bl);

    // --- CONNECTIONS ---
    connect(sendButton, &QPushButton::clicked, this, &Form::onSendClicked);
    connect(input, &QLineEdit::returnPressed, this, &Form::onSendClicked);
    connect(aiSendButton, &QPushButton::clicked, this, &Form::onAiSendClicked);
    connect(aiInput, &QLineEdit::returnPressed, this, &Form::onAiSendClicked);
    connect(configButton, &QPushButton::clicked, this, &Form::configureAiClicked);
    connect(resultsList, &QListWidget::itemActivated, this, &Form::onResultActivated);
    connect(clearButton, &QPushButton::clicked, this, &Form::onClearClicked);
    connect(exportCsvButton, &QPushButton::clicked, this, &Form::onExportCsvClicked);
    connect(exportAllButton, &QPushButton::clicked, this, &Form::onExportAllClicked);
    connect(filterLoadButton, &QPushButton::clicked, this, &Form::onFilterLoadClicked);

    // --- MAIN BUTTON STYLES ---
    {
        auto c = sendColors(dark);
        applyBtnStyle(sendButton, dark, c.bg, c.fg, c.hover);
    }
    {
        auto c = aiSendColors(dark);
        applyBtnStyle(aiSendButton, dark, c.bg, c.fg, c.hover);
    }
    {
        auto c = clearColors(dark);
        applyBtnStyle(clearButton, dark, c.bg, c.fg, c.hover);
    }
    {
        auto c = exportColors(dark);
        applyBtnStyle(exportCsvButton, dark, c.bg, c.fg, c.hover);
        applyBtnStyle(exportAllButton, dark, c.bg, c.fg, c.hover);
    }
    {
        auto c = filterColors(dark);
        applyBtnStyle(filterLoadButton, dark, c.bg, c.fg, c.hover);
    }
}

void Form::appendMessage(const QString &author, const QString &html)
{
    bool dk = palette().color(QPalette::Window).lightness() < 128;
    QString color;
    if (author == "Tu" || author.startsWith("Tu")) color = dk ? "#42a5f5" : "#1565c0";
    else if (author.contains("AI")) color = dk ? "#66bb6a" : "#2e7d32";
    else color = dk ? "#bdbdbd" : "#616161";

    history->append(QString("<p><b style='color:%1'>%2:</b> %3</p>").arg(color, author.toHtmlEscaped(), html));
    auto *sb = history->verticalScrollBar();
    if (sb) sb->setValue(sb->maximum());
}

void Form::setResults(const QList<int> &indices, const QStringList &snippets,
                      const QStringList &levels)
{
    resultsList->clear();
    if (indices.isEmpty()) return;

    resultsList->setUpdatesEnabled(false);
    bool dk = palette().color(QPalette::Window).lightness() < 128;
    int n = qMin(indices.size(), snippets.size());

    resultsList->setMaximumHeight(qMin(n * 20 + 4, 400));

    for (int i = 0; i < n; ++i)
    {
        QString lvl = i < levels.size() ? levels[i] : QString();
        QString col;
        if (lvl == "error" || lvl == "fatal") col = dk ? "#ef5350" : "#d32f2f";
        else if (lvl == "warn") col = dk ? "#ffa726" : "#e65100";
        else col = dk ? "#e0e0e0" : "#424242";

        auto *item = new QListWidgetItem(resultsList);
        item->setText(QString("%1: %2").arg(indices[i]).arg(snippets[i]));
        item->setData(Qt::UserRole, indices[i]);
        if (!lvl.isEmpty()) item->setForeground(QColor(col));
    }
    resultsList->setUpdatesEnabled(true);
}

void Form::setAiStatus(int state, const QString &modelName)
{
    bool dk = palette().color(QPalette::Window).lightness() < 128;
    if (state == 2) {
        aiStatusLabel->setText(QString("AI: %1").arg(modelName));
        aiStatusLabel->setStyleSheet(QString("font-size:11px;color:%1;padding:0;").arg(dk?"#66bb6a":"#2e7d32"));
        aiSendButton->setEnabled(true);
        aiInput->setPlaceholderText(tr("Ask the AI..."));
        if (aiDisclaimer) aiDisclaimer->show();
    } else if (state == 1) {
        aiStatusLabel->setText(tr("AI: offline"));
        aiStatusLabel->setStyleSheet(QString("font-size:11px;color:%1;padding:0;").arg(dk?"#ffa726":"#e65100"));
        aiSendButton->setEnabled(false);
        aiInput->setPlaceholderText(tr("AI offline. Click \u2699 to configure."));
        if (aiDisclaimer) aiDisclaimer->hide();
    } else {
        aiStatusLabel->setText(tr("AI: -"));
        aiStatusLabel->setStyleSheet(QString("font-size:11px;color:%1;padding:0;").arg(dk?"#9e9e9e":"#757575"));
        aiSendButton->setEnabled(false);
        aiInput->setPlaceholderText(tr("Configure AI (\u2699)"));
        if (aiDisclaimer) aiDisclaimer->hide();
    }
}

void Form::setStatusText(const QString &text) { statusLabel->setText(text); }

void Form::setProcessingProgress(bool visible) {
    aiProgress->setVisible(visible);
    aiSendButton->setEnabled(!visible);
    aiInput->setEnabled(!visible);
    if (visible) QCoreApplication::processEvents();
}

void Form::onSendClicked() {
    QString q = input->text().trimmed();
    if (q.isEmpty()) return;
    if (q.length() > kMaxInputLength) {
        q = q.left(kMaxInputLength);
    }
    input->clear();
    emit querySubmitted(q);
}

void Form::onAiSendClicked() {
    QString q = aiInput->text().trimmed();
    if (q.isEmpty()) return;
    if (!aiSendButton->isEnabled()) return;
    if (q.length() > kMaxInputLength) {
        q = q.left(kMaxInputLength);
    }
    aiInput->clear();
    emit aiQuerySubmitted(q);
}

void Form::onResultActivated(QListWidgetItem *item) {
    if (!item) return;
    bool ok = false;
    int idx = item->data(Qt::UserRole).toInt(&ok);
    if (ok) emit indexActivated(idx);
}

void Form::onClearClicked() {
    if (resultsList->count() > 0) {
        emit clearHighlightsRequested();
    }
}

void Form::exportResultsToCsv(const QString &def) {
    if (resultsList->count() == 0) {
        appendMessage("Chat Assistant", tr("No results to export. Run a query first."));
        return;
    }

    QString fp = QFileDialog::getSaveFileName(this,
        tr("Export Results CSV"),
        def.isEmpty() ? "dlt_results.csv" : def,
        tr("CSV Files (*.csv);;All Files (*)"));
    if (fp.isEmpty()) return;

    QList<int> idx; QStringList snip;
    idx.reserve(resultsList->count());
    snip.reserve(resultsList->count());
    for (int i = 0; i < resultsList->count(); ++i) {
        auto *item = resultsList->item(i);
        if (!item) continue;
        bool ok = false; int n = item->data(Qt::UserRole).toInt(&ok);
        if (ok) { idx.append(n); snip.append(item->text()); }
    }
    emit exportRequested(fp, idx, snip, lastQuery);
}

void Form::exportAllToCsv(const QString &def) {
    QString fp = QFileDialog::getSaveFileName(this,
        tr("Export All CSV"),
        def.isEmpty() ? "dlt_all.csv" : def,
        tr("CSV Files (*.csv);;All Files (*)"));
    if (!fp.isEmpty()) emit exportAllRequested(fp);
}

void Form::onExportCsvClicked() { exportResultsToCsv(); }
void Form::onExportAllClicked() { exportAllToCsv(); }

void Form::onFilterLoadClicked()
{
    QString fp = QFileDialog::getOpenFileName(this,
        tr("Load User Filters"),
        QString(),
        tr("Filter Files (*.json);;All Files (*)"));
    if (!fp.isEmpty())
        emit userFilterLoadRequested(fp);
}

void Form::onQuickActionClicked()
{
    auto *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    static QMap<QString, QString> map;
    if (map.isEmpty()) {
        map = {
            {tr("Errors"),"error"},{tr("Warnings"),"warn"},{tr("Info"),"info"},{tr("Debug"),"debug"},
            {tr("CAN"),"can"},{tr("Security"),"security"},{tr("Memory"),"memory"},
            {tr("Performance"),"performance"},{tr("Diagnostic"),"diagnostic"},{tr("Pattern"),"pattern"},
            {tr("Summary"),"summary"},{tr("Timeline"),"timeline"},{tr("GPS"),"gps"},{tr("Help"),"help"},
            {tr("CarPlay"),"carplay"},{tr("AndroidAuto"),"androidauto"},
            {tr("Focus"),"video_focus"},{tr("Ducking"),"audio_ducking"},
            {tr("mDNS"),"mdns"},{tr("Sensor"),"sensor_data"},
        };
    }
    QString q = map.value(btn->text());
    if (!q.isEmpty()) { input->setText(q); onSendClicked(); }
}

} // namespace DltChat

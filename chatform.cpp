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

namespace DltChat {

static QString btnStyle(const QWidget *w)
{
    if (!w) return QString();
    QColor b = w->palette().color(QPalette::Button);
    QColor f = w->palette().color(QPalette::ButtonText);
    QColor h = w->palette().color(QPalette::Highlight);
    return QString(
        "QPushButton{padding:2px 6px;border:1px solid %1;border-radius:3px;background:%2;color:%3;font-size:11px;}"
        "QPushButton:hover{background:%4;border-color:%5;}"
        "QPushButton:pressed{background:%6;}")
        .arg(b.darker(130).name()).arg(b.name()).arg(f.name())
        .arg(h.lighter(160).name()).arg(h.name()).arg(h.lighter(130).name());
}

static QPushButton *makeBtn(const QString &text, const QString &tip, QWidget *parent)
{
    auto *b = new QPushButton(text, parent);
    b->setToolTip(tip);
    b->setMinimumHeight(24);
    b->setStyleSheet(btnStyle(parent));
    return b;
}

Form::Form(QWidget *parent)
    : QWidget(parent)
    , statusLabel(new QLabel(this))
    , aiStatusLabel(new QLabel(this))
    , configButton(new QPushButton(QString::fromUtf8("\u2699"), this))
    , history(new QTextBrowser(this))
    , resultsList(new QListWidget(this))
    , input(new QLineEdit(this))
    , sendButton(new QPushButton(tr("Invia"), this))
    , aiInput(new QLineEdit(this))
    , aiSendButton(new QPushButton(tr("Chiedi"), this))
    , aiProgress(new QProgressBar(this))
    , clearButton(new QPushButton(tr("Pulisci"), this))
    , exportCsvButton(new QPushButton(tr("CSV"), this))
    , exportAllButton(new QPushButton(tr("CSV All"), this))
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
    statusLabel->setText(tr("Nessun log caricato."));

    configButton->setToolTip(tr("Configura AI"));
    configButton->setFixedSize(28, 24);
    configButton->setStyleSheet(QString("QPushButton{border:none;font-size:14px;color:%1;} QPushButton:hover{color:%2;}").arg(fgS).arg(hlS));

    aiStatusLabel->setStyleSheet(QString("font-size:11px;color:%1;padding:0;").arg(dark?"#9e9e9e":"#757575"));
    aiStatusLabel->setText(tr("AI: \u2716"));

    history->setReadOnly(true);
    history->setStyleSheet(QString("QTextBrowser{background:%1;color:%2;border:1px solid %3;border-radius:4px;}").arg(bgS).arg(fgS).arg(midS));

    resultsList->setSelectionMode(QAbstractItemView::SingleSelection);
    resultsList->setStyleSheet(QString(
        "QListWidget{background:%1;color:%2;border:1px solid %3;border-radius:4px;}"
        "QListWidget::item{padding:2px 4px;}"
        "QListWidget::item:selected{background:%4;color:%5;}"
    ).arg(bgS).arg(fgS).arg(midS).arg(hlS).arg(palette().color(QPalette::HighlightedText).name()));
    resultsList->setMaximumHeight(180);

    input->setPlaceholderText(tr("Fai una domanda sui log..."));
    input->setStyleSheet(QString("QLineEdit{padding:4px;border:1px solid %1;border-radius:4px;background:%2;color:%3;}").arg(midS).arg(bgS).arg(fgS));
    aiInput->setPlaceholderText(tr("Fai una domanda all'AI..."));
    aiInput->setStyleSheet(input->styleSheet());

    aiProgress->setRange(0, 0);
    aiProgress->setFixedHeight(4);
    aiProgress->hide();

    // Quick buttons - 2x7 grid
    QGroupBox *quickBox = new QGroupBox(tr("Azioni Rapide"), this);
    QGridLayout *ql = new QGridLayout();
    ql->setSpacing(3);
    struct { QString t; QString tip; } bd[] = {
        {tr("Errori"),tr("Errori/fatali")},{tr("Warnings"),tr("Warning")},
        {tr("Info"),tr("Informativi")},{tr("Debug"),tr("Debug")},
        {tr("CAN"),tr("CAN bus")},{tr("Security"),tr("Sicurezza")},
        {tr("Memoria"),tr("Memoria")},{tr("Performance"),tr("Performance")},
        {tr("Diagnostic"),tr("Diagnostica DTC")},{tr("Pattern"),tr("Ripetizioni")},
        {tr("Summary"),tr("Statistiche")},{tr("Timeline"),tr("Sequenza")},
        {tr("GPS"),tr("Navigazione")},{tr("Help"),tr("Aiuto")},
    };
    for (int i = 0; i < 14; ++i)
    {
        auto *b = makeBtn(bd[i].t, bd[i].tip, this);
        connect(b, &QPushButton::clicked, this, &Form::onQuickActionClicked);
        ql->addWidget(b, i / 7, i % 7);
    }
    quickBox->setLayout(ql);

    // Header: title + AI status + config
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
    aiL->addLayout(aiRow); aiL->addWidget(aiProgress);
    QGroupBox *aiBox = new QGroupBox(tr("AI")); aiBox->setLayout(aiL);

    // Bottom buttons
    QHBoxLayout *bl = new QHBoxLayout();
    bl->addWidget(clearButton); bl->addStretch();
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

    connect(sendButton, &QPushButton::clicked, this, &Form::onSendClicked);
    connect(input, &QLineEdit::returnPressed, this, &Form::onSendClicked);
    connect(aiSendButton, &QPushButton::clicked, this, &Form::onAiSendClicked);
    connect(aiInput, &QLineEdit::returnPressed, this, &Form::onAiSendClicked);
    connect(configButton, &QPushButton::clicked, this, &Form::configureAiClicked);
    connect(resultsList, &QListWidget::itemActivated, this, &Form::onResultActivated);
    connect(clearButton, &QPushButton::clicked, this, &Form::onClearClicked);
    connect(exportCsvButton, &QPushButton::clicked, this, &Form::onExportCsvClicked);
    connect(exportAllButton, &QPushButton::clicked, this, &Form::onExportAllClicked);

    QString sb = QString("QPushButton{padding:2px 8px;border:1px solid %1;border-radius:3px;background:%2;color:%3;}"
        "QPushButton:hover{background:%4;}").arg(midS).arg(dark?"#353535":"#f5f5f5").arg(fgS).arg(hl.lighter(170).name());
    sendButton->setStyleSheet(sb);
    aiSendButton->setStyleSheet(sb);
    clearButton->setStyleSheet(sb);
    exportCsvButton->setStyleSheet(sb);
    exportAllButton->setStyleSheet(sb);
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
    bool dk = palette().color(QPalette::Window).lightness() < 128;
    int n = qMin(indices.size(), snippets.size());
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
}

void Form::setAiStatus(int state, const QString &modelName)
{
    bool dk = palette().color(QPalette::Window).lightness() < 128;
    if (state == 2) {
        aiStatusLabel->setText(QString("AI: %1").arg(modelName));
        aiStatusLabel->setStyleSheet(QString("font-size:11px;color:%1;padding:0;").arg(dk?"#66bb6a":"#2e7d32"));
        aiSendButton->setEnabled(true);
        aiInput->setPlaceholderText(tr("Chiedi all'AI..."));
    } else if (state == 1) {
        aiStatusLabel->setText(tr("AI: offline"));
        aiStatusLabel->setStyleSheet(QString("font-size:11px;color:%1;padding:0;").arg(dk?"#ffa726":"#e65100"));
        aiSendButton->setEnabled(false);
        aiInput->setPlaceholderText(tr("AI offline. Clicca \u2699 per configurare."));
    } else {
        aiStatusLabel->setText(tr("AI: -"));
        aiStatusLabel->setStyleSheet(QString("font-size:11px;color:%1;padding:0;").arg(dk?"#9e9e9e":"#757575"));
        aiSendButton->setEnabled(false);
        aiInput->setPlaceholderText(tr("Configura AI (\u2699)"));
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
    input->clear(); emit querySubmitted(q);
}

void Form::onAiSendClicked() {
    QString q = aiInput->text().trimmed();
    if (q.isEmpty()) return;
    aiInput->clear(); emit aiQuerySubmitted(q);
}

void Form::onResultActivated(QListWidgetItem *item) {
    if (!item) return;
    bool ok = false;
    int idx = item->data(Qt::UserRole).toInt(&ok);
    if (ok) emit indexActivated(idx);
}

void Form::onClearClicked() { emit clearHighlightsRequested(); }

void Form::exportResultsToCsv(const QString &def) {
    QString fp = QFileDialog::getSaveFileName(this, tr("Esporta CSV"),
        def.isEmpty() ? "dlt_results.csv" : def, tr("CSV (*.csv);;All (*)"));
    if (fp.isEmpty()) return;
    QList<int> idx; QStringList snip;
    for (int i = 0; i < resultsList->count(); ++i) {
        auto *item = resultsList->item(i);
        if (!item) continue;
        bool ok = false; int n = item->data(Qt::UserRole).toInt(&ok);
        if (ok) { idx.append(n); snip.append(item->text()); }
    }
    emit exportRequested(fp, idx, snip, lastQuery);
}

void Form::exportAllToCsv(const QString &def) {
    QString fp = QFileDialog::getSaveFileName(this, tr("Esporta Tutto"),
        def.isEmpty() ? "dlt_all.csv" : def, tr("CSV (*.csv);;All (*)"));
    if (!fp.isEmpty()) emit exportAllRequested(fp);
}

void Form::onExportCsvClicked() { exportResultsToCsv(); }
void Form::onExportAllClicked() { exportAllToCsv(); }

void Form::onQuickActionClicked()
{
    auto *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    static const QMap<QString, QString> map = {
        {tr("Errori"),"error"},{tr("Warnings"),"warn"},{tr("Info"),"info"},{tr("Debug"),"debug"},
        {tr("CAN"),"can"},{tr("Security"),"security"},{tr("Memoria"),"memory"},
        {tr("Performance"),"performance"},{tr("Diagnostic"),"diagnostic"},{tr("Pattern"),"pattern"},
        {tr("Summary"),"summary"},{tr("Timeline"),"timeline"},{tr("GPS"),"gps"},{tr("Help"),"help"},
    };
    QString q = map.value(btn->text());
    if (!q.isEmpty()) { input->setText(q); onSendClicked(); }
}

} // namespace DltChat

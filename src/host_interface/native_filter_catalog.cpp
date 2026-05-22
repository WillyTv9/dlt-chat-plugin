#include "native_filter_catalog.h"

#include <QFile>
#include <QXmlStreamReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>

#include "qdltfile.h"
#include "qdltmsg.h"

namespace DltChat {

namespace {
constexpr char kEmbeddedDlp[]    = ":/dltchat/default_filters.dlp";
constexpr char kGroupingJson[]   = ":/dltchat/native_filter_groups.json";
constexpr char kLeftoverGroupId[]    = "other";
constexpr char kLeftoverGroupLabel[] = "Altri";
} // namespace

bool NativeFilterCatalog::load(const QString &dlpPath)
{
    m_lastError.clear();
    m_actions.clear();
    m_actionIndex.clear();
    m_groups.clear();

    const QString path = dlpPath.isEmpty() ? QString::fromLatin1(kEmbeddedDlp) : dlpPath;
    if (!parseDlp(path))
        return false;

    buildGroups();
    return isLoaded();
}

const NativeFilterAction *NativeFilterCatalog::action(const QString &name) const
{
    const int idx = m_actionIndex.value(name, -1);
    return (idx >= 0) ? &m_actions.at(idx) : nullptr;
}

bool NativeFilterCatalog::parseDlp(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = QStringLiteral("Impossibile aprire il file filtri: %1").arg(path);
        return false;
    }
    m_sourcePath = path;

    // Parse every <pfilter> element via the SDK's own loader so matching stays
    // byte-for-byte faithful to DLT-Viewer. Positives become actions; negatives
    // are stashed by name and later attached to their positive via 'exclusions'.
    QList<QDltFilter> positives;
    QHash<QString, QDltFilter> negatives;   // name -> filter (type == 1)

    QXmlStreamReader xml(&file);
    while (!xml.atEnd() && !xml.hasError()) {
        const auto tok = xml.readNext();
        if (tok != QXmlStreamReader::StartElement || xml.name() != QLatin1String("pfilter"))
            continue;

        QDltFilter f;
        // Consume the element's children, feeding each to LoadFilterItem (which
        // handles one tag per call), until the matching </pfilter>.
        while (!(xml.tokenType() == QXmlStreamReader::EndElement &&
                 xml.name() == QLatin1String("pfilter"))) {
            if (xml.tokenType() == QXmlStreamReader::StartElement)
                f.LoadFilterItem(xml);
            if (xml.atEnd() || xml.hasError()) break;
            xml.readNext();
        }
        f.compileRegexps();   // build header/payload/appid/context QRegularExpressions

        if (f.type == QDltFilter::negative)
            negatives.insert(f.name, f);
        else
            positives.append(f);   // positive + marker both act as "show" actions
    }

    if (xml.hasError()) {
        m_lastError = QStringLiteral("Errore di parsing .dlp: %1").arg(xml.errorString());
        return false;
    }

    // Read the positive->negative pairing map from the grouping resource.
    QHash<QString, QString> exclusions;   // positiveName -> negativeName
    {
        QFile gf(QString::fromLatin1(kGroupingJson));
        if (gf.open(QIODevice::ReadOnly)) {
            const QJsonObject root = QJsonDocument::fromJson(gf.readAll()).object();
            const QJsonObject ex = root.value(QStringLiteral("exclusions")).toObject();
            for (auto it = ex.begin(); it != ex.end(); ++it)
                exclusions.insert(it.key(), it.value().toString());
        }
    }

    for (const QDltFilter &p : positives) {
        NativeFilterAction a;
        a.name     = p.name;
        a.colour   = QColor(p.filterColour);
        a.positive = p;
        const QString negName = exclusions.value(p.name);
        if (!negName.isEmpty() && negatives.contains(negName))
            a.excludes.append(negatives.value(negName));

        m_actionIndex.insert(a.name, m_actions.size());
        m_actions.append(a);
    }

    if (m_actions.isEmpty())
        m_lastError = QStringLiteral("Nessun filtro positivo trovato in %1").arg(path);
    return !m_actions.isEmpty();
}

void NativeFilterCatalog::buildGroups()
{
    QFile gf(QString::fromLatin1(kGroupingJson));
    QSet<QString> assigned;

    if (gf.open(QIODevice::ReadOnly)) {
        const QJsonObject root = QJsonDocument::fromJson(gf.readAll()).object();
        const QJsonArray groups = root.value(QStringLiteral("groups")).toArray();
        for (const QJsonValue &gv : groups) {
            const QJsonObject go = gv.toObject();
            NativeFilterGroup g;
            g.id    = go.value(QStringLiteral("id")).toString();
            g.label = go.value(QStringLiteral("label")).toString();
            for (const QJsonValue &fv : go.value(QStringLiteral("filters")).toArray()) {
                const QString fn = fv.toString();
                if (m_actionIndex.contains(fn)) {   // only keep names that exist in the .dlp
                    g.actionNames.append(fn);
                    assigned.insert(fn);
                }
            }
            if (!g.actionNames.isEmpty())
                m_groups.append(g);
        }
    }

    // Any positive filter not referenced by the grouping JSON lands in "Altri",
    // so a Quick Action is never silently dropped when the .dlp gains new filters.
    NativeFilterGroup leftover;
    leftover.id    = QString::fromLatin1(kLeftoverGroupId);
    leftover.label = QString::fromLatin1(kLeftoverGroupLabel);
    for (const NativeFilterAction &a : m_actions)
        if (!assigned.contains(a.name))
            leftover.actionNames.append(a.name);
    if (!leftover.actionNames.isEmpty())
        m_groups.append(leftover);
}

QList<int> NativeFilterCatalog::match(const NativeFilterAction &action, QDltFile *file) const
{
    QList<int> out;
    if (!file) return out;

    const int count = file->size();
    out.reserve(count / 8);
    for (int i = 0; i < count; ++i) {
        QDltMsg msg;
        if (!file->getMsg(i, msg))
            continue;
        if (!action.positive.match(msg))
            continue;

        bool excluded = false;
        for (const QDltFilter &neg : action.excludes) {
            if (neg.match(msg)) { excluded = true; break; }
        }
        if (!excluded)
            out.append(i);
    }
    return out;
}

} // namespace DltChat

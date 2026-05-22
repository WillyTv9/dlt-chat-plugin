#ifndef DLTCHAT_NATIVE_FILTER_CATALOG_H
#define DLTCHAT_NATIVE_FILTER_CATALOG_H

#include <QString>
#include <QList>
#include <QHash>
#include <QColor>

#include "qdltfilter.h"

class QDltFile;

namespace DltChat {

//! In-memory representation of one Quick Action backed by native DLT filters.
/*!
    Each action wraps exactly one POSITIVE (or marker) QDltFilter parsed from the
    project .dlp, plus zero or more NEGATIVE filters whose matches are subtracted
    from the result — reproducing DLT-Viewer's "positive = show / negative = hide"
    semantics. All matching is delegated to QDltFilter::match(), so regex and
    case-sensitivity behave exactly as configured in the .dlp.
*/
struct NativeFilterAction
{
    QString name;                 //!< <name> of the positive filter (unique key, also the menu label)
    QColor colour;                //!< <filterColour> used for row highlighting
    QDltFilter positive;          //!< the show filter
    QList<QDltFilter> excludes;   //!< paired negative filters (subtracted)
    bool hasExcludes() const { return !excludes.isEmpty(); }
};

//! A drop-down menu grouping several NativeFilterAction together.
struct NativeFilterGroup
{
    QString id;
    QString label;                //!< menu caption shown in the UI
    QList<QString> actionNames;   //!< keys into NativeFilterCatalog::action()
};

//! Loads, groups and applies the native .dlp filters that drive the Quick Actions.
/*!
    Loading order (see load()):
      1. an explicit .dlp path (typically from dlt_chat_plugin.ini) when non-empty;
      2. otherwise the embedded ":/dltchat/default_filters.dlp" resource.
    The macro-category layout and the positive/negative pairing come from the
    embedded ":/dltchat/native_filter_groups.json".
*/
class NativeFilterCatalog
{
public:
    NativeFilterCatalog() = default;

    //! Load filters + grouping. \a dlpPath empty => use embedded default. Returns false on parse error.
    bool load(const QString &dlpPath = QString());

    bool isLoaded() const { return !m_actions.isEmpty(); }
    QString lastError() const { return m_lastError; }
    QString sourcePath() const { return m_sourcePath; }   //!< ":/dltchat/default_filters.dlp" or the .ini path

    const QList<NativeFilterGroup> &groups() const { return m_groups; }
    const NativeFilterAction *action(const QString &name) const;

    //! Run \a action over \a file natively and return the matched message indices (ascending).
    /*!
        For every message: included when the positive filter matches AND no paired
        negative filter matches. Decoding is performed on demand via QDltFile::getMsg().
    */
    QList<int> match(const NativeFilterAction &action, QDltFile *file) const;

private:
    bool parseDlp(const QString &path);
    void buildGroups();   //!< reads native_filter_groups.json + assigns leftovers to "Altri"

    QString m_lastError;
    QString m_sourcePath;
    QList<NativeFilterAction> m_actions;
    QHash<QString, int>       m_actionIndex;  //!< name -> index into m_actions
    QList<NativeFilterGroup>  m_groups;
};

} // namespace DltChat

#endif // DLTCHAT_NATIVE_FILTER_CATALOG_H

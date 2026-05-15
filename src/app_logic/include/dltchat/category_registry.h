#ifndef DLTCHAT_CATEGORY_REGISTRY_H
#define DLTCHAT_CATEGORY_REGISTRY_H

#include "analyzer_interface.h"

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

namespace dltchat {

struct QuickActionDef {
    QString label;
    QString query;
    int row = 0;
    int col = 0;
};

struct CategoryDef {
    QString id;
    QStringList aliases;
    QStringList filters;
    int priority = 100;
    QString domain;
    bool levelOnly = false;
    QuickActionDef quickAction;
    bool hasQuickAction = false;
};

struct ProjectionEventDef {
    QString id;
    QString query;
    QString domain;
    QString event;
    QStringList filters;
    QString matchMode = "any";
    QString requireLevel;
    QuickActionDef quickAction;
    bool hasQuickAction = false;
};

struct CombinedFilterDef {
    QString id;
    QStringList aliases;
    QStringList levels;
    QStringList categoryIds;
    QStringList extraFilters;
    QuickActionDef quickAction;
    bool hasQuickAction = false;
};

struct ResolvedQuery {
    enum class Kind {
        Unknown,
        SpecialCommand,
        CombinedFilter,
        Category,
        ProjectionEvent,
        CategoryById
    };

    Kind kind = Kind::Unknown;
    QString queryKey;
    QString specialCommand;
    QString combinedId;
    QString categoryId;
    QString projectionEventId;
    QStringList levelFilter;
    QStringList categoryIds;
    QStringList extraFilters;
};

class CategoryRegistry
{
public:
    static CategoryRegistry &instance();

    bool loadFromResource(const QString &path = QStringLiteral(":/dltchat/category_registry.json"));
    bool loadFromFile(const QString &filePath);

    const QVector<CategoryDef> &categories() const { return m_categories; }
    const QVector<ProjectionEventDef> &projectionEvents() const { return m_projectionEvents; }
    const QVector<CombinedFilterDef> &combinedFilters() const { return m_combinedFilters; }
    const QVector<QuickActionDef> &quickActions() const { return m_quickActions; }

    ResolvedQuery resolveQuery(const QString &query) const;
    bool isSpecialCommand(const QString &query) const;

    QString classifyEntry(DltAnalyzerInterface::LogEntry &entry) const;
    void assignProjectionEvent(DltAnalyzerInterface::LogEntry &entry) const;

    bool entryMatchesCategory(const DltAnalyzerInterface::LogEntry &entry,
                              const CategoryDef &cat) const;
    bool entryMatchesProjectionEvent(const DltAnalyzerInterface::LogEntry &entry,
                                     const ProjectionEventDef &ev) const;

    QVector<DltAnalyzerInterface::LogEntry> filterEntries(
        const QVector<DltAnalyzerInterface::LogEntry> &entries,
        const ResolvedQuery &resolved) const;

    QStringList allCompletionStrings() const;
    QString buildCategoriesHelpHtml() const;
    const CategoryDef *categoryById(const QString &id) const;

private:
    CategoryRegistry();
    bool parseJson(const QByteArray &data);
    bool fieldMatchesFilter(const DltAnalyzerInterface::LogEntry &entry,
                            const QString &filter) const;
    bool levelMatchesFilter(const QString &level, const QString &filter) const;
    bool entryMatchesAnyFilter(const DltAnalyzerInterface::LogEntry &entry,
                               const QStringList &filters) const;
    bool entryMatchesAllFilters(const DltAnalyzerInterface::LogEntry &entry,
                                const QStringList &filters) const;
    const CategoryDef *bestCategoryForEntry(const DltAnalyzerInterface::LogEntry &entry) const;

    QVector<CategoryDef> m_categories;
    QVector<ProjectionEventDef> m_projectionEvents;
    QVector<CombinedFilterDef> m_combinedFilters;
    QVector<QuickActionDef> m_quickActions;
    QHash<QString, QString> m_aliasToCategory;
    QHash<QString, QString> m_aliasToCombined;
    QHash<QString, QString> m_aliasToProjection;
    QHash<QString, QString> m_specialCommands;
    QHash<QString, const CategoryDef *> m_categoryById;
    bool m_loaded = false;
};

} // namespace dltchat

#endif

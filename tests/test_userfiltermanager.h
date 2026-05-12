#ifndef TEST_USERFILTERMANAGER_H
#define TEST_USERFILTERMANAGER_H

#include <QObject>
#include <QTemporaryDir>

class TestUserFilterManager : public QObject
{
    Q_OBJECT

private:
    QString createSampleFilterFile(const QTemporaryDir &dir) const;

private slots:
    void testLoadValidFilters();
    void testLoadInvalidFile();
    void testLoadMissingVersion();
    void testActiveFilterCount();
    void testApplyToEntries();
    void testClear();
    void testMatchesEntryByDomain();
    void testMatchesEntryByLevel();
};

#endif

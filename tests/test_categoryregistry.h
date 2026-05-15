#ifndef TEST_CATEGORYREGISTRY_H
#define TEST_CATEGORYREGISTRY_H

#include <QObject>

class TestCategoryRegistry : public QObject
{
    Q_OBJECT

private slots:
    void testRegistryLoads();
    void testResolveGpsAlias();
    void testResolveCombinedGpsErrors();
    void testClassifyCarPlay();
    void testClassifyAndroidAuto();
    void testFilterUsesAllKeywords();
    void testLevelOnlyError();
    void testProjectionAuthErrors();
};

#endif

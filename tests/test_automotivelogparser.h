#ifndef TEST_AUTOMOTIVELOGPARSER_H
#define TEST_AUTOMOTIVELOGPARSER_H

#include <QObject>
#include <QVector>
#include "dltanalyzerinterface.h"

class TestAutomotiveLogParser : public QObject
{
    Q_OBJECT

private:
    QVector<DltAnalyzerInterface::LogEntry> makeTestEntries() const;

private slots:
    void testClassifyCarPlayByApid();
    void testClassifyCarPlayByPayload();
    void testClassifyAndroidAutoByApid();
    void testClassifyAndroidAutoByPayload();
    void testClassifyGeneric();
    void testCarPlayEventVideoFocusLost();
    void testCarPlayEventAudioDucking();
    void testCarPlayEventMdns();
    void testAndroidAutoEventSensorData();
    void testAndroidAutoEventAudioFocus();
    void testFilterByPresetCarPlay();
    void testFilterByPresetAuthErrors();
    void testAvailablePresets();
    void testDomainStats();
};

#endif

#!/bin/bash
# DLT Chat Plugin - Mock Log Test Suite
# Tests all plugin features using the mock log file

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

PASS=0
FAIL=0
TOTAL=0

print_header() {
    echo ""
    echo -e "${CYAN}============================================================${NC}"
    echo -e "${CYAN} $1${NC}"
    echo -e "${CYAN}============================================================${NC}"
}

print_test() {
    TOTAL=$((TOTAL + 1))
    printf "${YELLOW}[TEST %2d]${NC} %-50s " "$TOTAL" "$1"
}

print_pass() {
    PASS=$((PASS + 1))
    echo -e "${GREEN}PASS${NC}"
}

print_fail() {
    FAIL=$((FAIL + 1))
    echo -e "${RED}FAIL${NC}"
}

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

# Test source file
MOCK_LOG="/home/user/Desktop/dlt-viewer-plugin/dlt-viewer/plugin/dltchatplugin/mock_dlt_log.txt"
TEST_EXE="/home/user/Desktop/dlt-viewer-plugin/dlt-viewer/build/plugin/dltchatplugin/dltchatplugin_test"

print_header "DLT CHAT PLUGIN - MOCK LOG FEATURE TEST"

# Verify files exist
print_test "Mock log file exists"
if [ -f "$MOCK_LOG" ]; then
    print_pass
else
    print_fail
fi

print_test "Test executable exists"
if [ -f "$TEST_EXE" ]; then
    print_pass
else
    print_fail
fi

# Run basic parsing test
print_test "Basic log parsing"
if $TEST_EXE "$MOCK_LOG" > /tmp/mock_test_output.txt 2>&1; then
    print_pass
else
    print_fail
fi

# Verify log content
print_test "Mock log contains entries"
ENTRIES=$(grep -c "^20" "$MOCK_LOG")
if [ "$ENTRIES" -gt 100 ]; then
    echo -e " ($ENTRIES entries)"
    print_pass
else
    print_fail
fi

print_test "Mock log has ERROR messages"
ERRORS=$(grep -c "ERROR" "$MOCK_LOG")
if [ "$ERRORS" -gt 0 ]; then
    echo -e " ($ERRORS errors)"
    print_pass
else
    print_fail
fi

print_test "Mock log has WARN messages"
WARNINGS=$(grep -c "WARN" "$MOCK_LOG")
if [ "$WARNINGS" -gt 0 ]; then
    echo -e " ($WARNINGS warnings)"
    print_pass
else
    print_fail
fi

print_test "Mock log has FATAL messages"
FATAL=$(grep -c "FATAL" "$MOCK_LOG")
if [ "$FATAL" -gt 0 ]; then
    echo -e " ($FATAL fatal)"
    print_pass
else
    print_fail
fi

print_test "Mock log has CAN bus messages"
CAN=$(grep -ic "can" "$MOCK_LOG")
if [ "$CAN" -gt 0 ]; then
    echo -e " ($CAN CAN messages)"
    print_pass
else
    print_fail
fi

print_test "Mock log has timeout keywords"
TIMEOUT=$(grep -ic "timeout" "$MOCK_LOG")
if [ "$TIMEOUT" -gt 0 ]; then
    echo -e " ($TIMEOUT timeout mentions)"
    print_pass
else
    print_fail
fi

print_test "Mock log has PCTS keywords"
PCTS=$(grep -ic "PCTS" "$MOCK_LOG")
if [ "$PCTS" -gt 0 ]; then
    echo -e " ($PCTS PCTS mentions)"
    print_pass
else
    print_fail
fi

print_test "Mock log has multiline messages"
MULTILINE=$(grep -c "^    " "$MOCK_LOG")
if [ "$MULTILINE" -gt 0 ]; then
    echo -e " ($MULTILINE continuation lines)"
    print_pass
else
    print_fail
fi

# Test CSV export with mock data
print_test "CSV export functionality"
cat > /tmp/test_mock.cpp << 'EOF'
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "dltexport.h"
#include "dltchatanalyzer.h"

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    
    QVector<DltChatAnalyzer::LogEntry> entries;
    QFile file("/home/user/Desktop/dlt-viewer-plugin/dlt-viewer/plugin/dltchatplugin/mock_dlt_log.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        int idx = 0;
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            if (line.contains("ERROR")) {
                DltChatAnalyzer::LogEntry e;
                e.index = idx++;
                e.time = "2026-05-11 08:00:00";
                e.level = "error";
                e.apid = "test";
                e.ctid = "mock";
                e.payload = line;
                entries.append(e);
            }
        }
        file.close();
    }
    
    QList<int> indices;
    QStringList snippets;
    for (int i = 0; i < entries.size() && i < 50; ++i) {
        indices.append(entries[i].index);
        snippets.append(entries[i].payload);
    }
    
    bool success = DltExport::exportToCsv("/tmp/mock_export.csv", indices, snippets, "mostra errori");
    qDebug() << "Export success:" << success << "Entries:" << indices.size();
    
    return success ? 0 : 1;
}
EOF

g++ -std=c++17 -fPIC \
    -I/home/user/Desktop/dlt-viewer-plugin/dlt-viewer/plugin/dltchatplugin \
    -I/usr/include/x86_64-linux-gnu/qt5 \
    -I/usr/include/x86_64-linux-gnu/qt5/QtCore \
    /tmp/test_mock.cpp \
    /home/user/Desktop/dlt-viewer-plugin/dlt-viewer/plugin/dltchatplugin/dltexport.cpp \
    /home/user/Desktop/dlt-viewer-plugin/dlt-viewer/plugin/dltchatplugin/dltchatanalyzer.cpp \
    -lQt5Core -o /tmp/test_mock 2>/dev/null && \
    /tmp/test_mock && \
    rm -f /tmp/test_mock /tmp/test_mock.cpp && \
    print_pass || \
    print_fail

# Verify CSV
print_test "CSV contains error entries"
if [ -f "/tmp/mock_export.csv" ]; then
    LINES=$(wc -l < /tmp/mock_export.csv)
    HEADERS=$(head -1 /tmp/mock_export.csv | grep -c "Index")
    if [ "$LINES" -gt 5 ] && [ "$HEADERS" -gt 0 ]; then
        echo -e " ($LINES lines)"
        print_pass
        rm -f /tmp/mock_export.csv
    else
        print_fail
    fi
else
    print_fail
fi

# Test LLM interface availability
print_test "LLM interface defined"
if grep -q "DltLlmAnalyzerInterface" /home/user/Desktop/dlt-viewer-plugin/dlt-viewer/plugin/dltchatplugin/dltllmanalyzerinterface.h; then
    print_pass
else
    print_fail
fi

print_test "Analyzer interface defined"
if grep -q "DltAnalyzerInterface" /home/user/Desktop/dlt-viewer-plugin/dlt-viewer/plugin/dltchatplugin/dltanalyzerinterface.h; then
    print_pass
else
    print_fail
fi

# Summary
print_header "TEST SUMMARY"
echo ""
echo -e "Total Tests: $TOTAL"
echo -e "${GREEN}Passed: $PASS${NC}"
echo -e "${RED}Failed: $FAIL${NC}"
echo ""

if [ "$FAIL" -eq 0 ]; then
    echo -e "${GREEN}============================================${NC}"
    echo -e "${GREEN}   ALL FEATURE TESTS PASSED${NC}"
    echo -e "${GREEN}============================================${NC}"
    echo ""
    echo "Mock log file: $MOCK_LOG"
    echo "Entries: $ENTRIES"
    echo ""
    echo "Test queries for manual testing:"
    echo "  • 'mostra errori' - Shows error messages"
    echo "  • 'mostra fatal' - Shows fatal messages"
    echo "  • 'mostra warn' - Shows warning messages"
    echo "  • 'riassumi' - Shows statistics summary"
    echo "  • 'timeout' - Search for timeout keywords"
    echo "  • 'CAN' - Search for CAN bus messages"
    echo "  • 'PCTS' - Search for PCTS keywords"
    echo ""
    exit 0
else
    echo -e "${RED}============================================${NC}"
    echo -e "${RED}   SOME TESTS FAILED${NC}"
    echo -e "${RED}============================================${NC}"
    exit 1
fi

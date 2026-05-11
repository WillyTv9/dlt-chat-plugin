#!/bin/bash
# DLT Chat Plugin - Comprehensive Test & Verification Script
# Version: 0.2.1

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PASS_COUNT=0
FAIL_COUNT=0
TOTAL_TESTS=0

print_header() {
    echo ""
    echo -e "${BLUE}============================================================${NC}"
    echo -e "${BLUE} $1${NC}"
    echo -e "${BLUE}============================================================${NC}"
}

print_test() {
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    echo -e "${YELLOW}[TEST $TOTAL_TESTS]${NC} $1"
}

print_pass() {
    PASS_COUNT=$((PASS_COUNT + 1))
    echo -e "${GREEN}[PASS]${NC} $1"
}

print_fail() {
    FAIL_COUNT=$((FAIL_COUNT + 1))
    echo -e "${RED}[FAIL]${NC} $1"
}

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

# Configuration
BUILD_DIR="/home/user/Desktop/dlt-viewer-plugin/dlt-viewer/build"
LOG_FILE="/home/user/Desktop/dlt-viewer-plugin/logs.txt"
PLUGIN_DIR="$BUILD_DIR/bin/plugins"

cd "$BUILD_DIR"

print_header "DLT CHAT PLUGIN v0.2.1 - TEST & VERIFICATION SUITE"

# Test 1: Check plugin binary exists
print_test "Plugin library exists"
if [ -f "$PLUGIN_DIR/libdltchatplugin.so" ]; then
    print_pass "Plugin library found"
else
    print_fail "Plugin library NOT found"
fi

# Test 2: Check plugin binary size
print_test "Plugin library size > 100KB"
PLUGIN_SIZE=$(stat -c%s "$PLUGIN_DIR/libdltchatplugin.so" 2>/dev/null || echo 0)
if [ "$PLUGIN_SIZE" -gt 100000 ]; then
    print_pass "Plugin size: $((PLUGIN_SIZE / 1024)) KB"
else
    print_fail "Plugin size too small: $PLUGIN_SIZE bytes"
fi

# Test 3: Check dependencies
print_test "Plugin dependencies resolved"
MISSING_DEPS=$(ldd "$PLUGIN_DIR/libdltchatplugin.so" 2>&1 | grep "not found" || true)
if [ -z "$MISSING_DEPS" ]; then
    print_pass "All dependencies resolved"
else
    print_fail "Missing dependencies: $MISSING_DEPS"
fi

# Test 4: Check DLT Viewer binary
print_test "DLT Viewer binary exists"
if [ -f "$BUILD_DIR/bin/dlt-viewer" ]; then
    print_pass "DLT Viewer found"
else
    print_fail "DLT Viewer NOT found"
fi

# Test 5: Check libqdlt
print_test "libqdlt dependency"
if [ -f "$BUILD_DIR/bin/libqdlt.so" ]; then
    print_pass "libqdlt.so found"
else
    print_fail "libqdlt.so NOT found"
fi

# Test 6: Run CLI test
print_test "CLI test suite execution"
cd "$BUILD_DIR/plugin/dltchatplugin"
if ./dltchatplugin_test "$LOG_FILE" > /tmp/test_output.txt 2>&1; then
    print_pass "CLI tests passed"
    cat /tmp/test_output.txt
else
    print_fail "CLI tests failed"
    cat /tmp/test_output.txt
fi

# Test 7: Check CSV export (compile and run)
print_test "CSV export functionality"
cat > /tmp/csv_test.cpp << 'EOF'
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "dltexport.h"
#include "dltchatanalyzer.h"

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    
    QVector<DltChatAnalyzer::LogEntry> entries;
    QFile file("/home/user/Desktop/dlt-viewer-plugin/logs.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        int idx = 0;
        while (!stream.atEnd() && idx < 100) {
            QString line = stream.readLine();
            if (!line.isEmpty()) {
                DltChatAnalyzer::LogEntry e;
                e.index = idx++;
                e.time = "2026-03-03 08:53:38";
                e.level = line.contains("ERROR") ? "error" : "debug";
                e.apid = "test";
                e.payload = line;
                entries.append(e);
            }
        }
        file.close();
    }
    
    QList<int> indices;
    QStringList snippets;
    for (int i = 0; i < entries.size(); ++i) {
        indices.append(i);
        snippets.append(entries[i].payload);
    }
    
    bool success = DltExport::exportToCsv("/tmp/test_export.csv", indices, snippets, "test");
    qDebug() << "Export result:" << success;
    
    return success ? 0 : 1;
}
EOF

g++ -std=c++17 -fPIC \
    -I/home/user/Desktop/dlt-viewer-plugin/dlt-viewer/plugin/dltchatplugin \
    -I/usr/include/x86_64-linux-gnu/qt5 \
    -I/usr/include/x86_64-linux-gnu/qt5/QtCore \
    /tmp/csv_test.cpp \
    /home/user/Desktop/dlt-viewer-plugin/dlt-viewer/plugin/dltchatplugin/dltexport.cpp \
    /home/user/Desktop/dlt-viewer-plugin/dlt-viewer/plugin/dltchatplugin/dltchatanalyzer.cpp \
    -lQt5Core -o /tmp/csv_test 2>/dev/null && \
    /tmp/csv_test && \
    rm -f /tmp/csv_test /tmp/csv_test.cpp && \
    print_pass "CSV export working" || \
    print_fail "CSV export failed"

# Test 8: Verify exported CSV content
print_test "CSV export content verification"
if [ -f "/tmp/test_export.csv" ]; then
    LINES=$(wc -l < /tmp/test_export.csv)
    HEADERS=$(head -1 /tmp/test_export.csv)
    if [ "$LINES" -gt 1 ] && [ -n "$HEADERS" ]; then
        print_pass "CSV content valid ($LINES lines, headers: $HEADERS)"
        rm -f /tmp/test_export.csv
    else
        print_fail "CSV content invalid"
    fi
else
    print_fail "CSV file not created"
fi

# Test 9: Plugin version check
print_test "Plugin version in binary"
if strings "$PLUGIN_DIR/libdltchatplugin.so" | grep -q "0.2.1"; then
    print_pass "Version 0.2.1 detected in binary"
else
    print_fail "Version 0.2.1 NOT found in binary"
fi

# Test 10: DLT Viewer startup test (background)
print_test "DLT Viewer startup with plugin"
cat > /tmp/viewer_test.sh << 'VIEWEREOF'
#!/bin/bash
cd /home/user/Desktop/dlt-viewer-plugin/dlt-viewer/build/bin
export LD_LIBRARY_PATH=.
timeout 5 ./dlt-viewer 2>&1 | grep -E "(DLT Log Assistant|Load plugin|ERROR)" | head -10
VIEWEREOF
chmod +x /tmp/viewer_test.sh
VIEWER_OUTPUT=$(/tmp/viewer_test.sh 2>&1 || true)
rm -f /tmp/viewer_test.sh

if echo "$VIEWER_OUTPUT" | grep -q "DLT Log Assistant"; then
    print_pass "Plugin loaded by DLT Viewer"
    echo "$VIEWER_OUTPUT" | grep "DLT Log Assistant"
else
    print_fail "Plugin NOT loaded by DLT Viewer"
    echo "$VIEWER_OUTPUT"
fi

# Summary
print_header "TEST SUMMARY"
echo ""
echo -e "Total Tests: $TOTAL_TESTS"
echo -e "${GREEN}Passed: $PASS_COUNT${NC}"
echo -e "${RED}Failed: $FAIL_COUNT${NC}"
echo ""

if [ "$FAIL_COUNT" -eq 0 ]; then
    echo -e "${GREEN}============================================${NC}"
    echo -e "${GREEN}   ALL TESTS PASSED - READY FOR USE${NC}"
    echo -e "${GREEN}============================================${NC}"
    echo ""
    echo "Plugin: $PLUGIN_DIR/libdltchatplugin.so"
    echo "Version: 0.2.1"
    echo ""
    echo "To use the plugin:"
    echo "  1. cd $BUILD_DIR/bin"
    echo "  2. LD_LIBRARY_PATH=. ./dlt-viewer"
    echo "  3. Enable 'DLT Log Assistant' in plugin settings"
    echo ""
    exit 0
else
    echo -e "${RED}============================================${NC}"
    echo -e "${RED}   SOME TESTS FAILED - REVIEW OUTPUT${NC}"
    echo -e "${RED}============================================${NC}"
    exit 1
fi

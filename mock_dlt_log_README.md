# DLT Chat Plugin - Mock Log File Documentation

**File:** `mock_dlt_log.dlt`
**Size:** ~8 KB
**Entries:** 138 log messages
**Purpose:** Testing of all plugin features

---

## Log Contents Overview

| Category | Count | Description |
|----------|-------|-------------|
| **Total Entries** | 138 | All log messages |
| **FATAL** | 3 | Critical failures |
| **ERROR** | 11 | Error conditions |
| **WARN** | 14 | Warning conditions |
| **INFO** | 37 | Informational messages |
| **DEBUG** | 65 | Debug messages |
| **VERBOSE** | 8 | Detailed trace messages |

---

## Test Scenarios

### 1. Log Level Detection
- `mostra errori` → Should find 11 ERROR entries
- `mostra fatal` → Should find 3 FATAL entries
- `mostra warn` → Should find 14 WARN entries

### 2. Keyword Search
- `timeout` → Multiple matches (timeout-related messages)
- `CAN` → Multiple matches (CAN bus messages)
- `connection` → Multiple connection messages

### 3. Summary Statistics
- `riassumi` → Shows log level distribution

### 4. Index Lookup
- `indice 50` → Shows context around index 50
- `indice 100` → Shows context around index 100

---

## Usage in DLT Viewer

1. Copy `mock_dlt_log.dlt` to a convenient location
2. In DLT Viewer: `File → Open DLT File`
3. Select `mock_dlt_log.dlt`
4. Open `DLT Log Assistant` panel
5. Test the queries listed above

---

## Expected Plugin Behavior

| Action | Expected Result |
|--------|-----------------|
| Load file | 138 entries parsed |
| Query "mostra errori" | 11 results, indices shown |
| Query "riassumi" | Summary with all level counts |
| Query "timeout" | Multiple matching entries |
| Query "indice 100" | Context around index 100 |
| Click result | Jump to corresponding row |
| Click "Esporta CSV" | CSV file created |

---

**Note:** The DLT file is in standard binary format compatible with COVESA DLT Viewer.

**Generated:** 2026-05-11
**Version:** 2.0
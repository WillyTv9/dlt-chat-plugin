#!/usr/bin/env python3
"""
DLT Log Generator - Create proper DLT binary log files
Generates a valid DLT log file for plugin testing
Based on the COVESA DLT file format specification
"""

import struct
from datetime import datetime, timedelta

DLT_SERIAL_HEADER = b'DLS\x01'

DLT_TYPE_LOG = 0x00
DLT_LOG_FATAL = 1
DLT_LOG_ERROR = 2
DLT_LOG_WARN = 3
DLT_LOG_INFO = 4
DLT_LOG_DEBUG = 5
DLT_LOG_VERBOSE = 6

DLT_HTYP_UEH = 0x01
DLT_HTYP_WEID = 0x04
DLT_HTYP_WSID = 0x08
DLT_HTYP_WTMS = 0x10

def create_dlt_message(timestamp, log_level, apid, ctid, payload, ecu_id="ECU1", session_id=1, timestamp_value=0):
    apid_bytes = apid.encode('ascii', errors='replace').ljust(4, b'\x00')[:4]
    ctid_bytes = ctid.encode('ascii', errors='replace').ljust(4, b'\x00')[:4]
    ecu_bytes = ecu_id.encode('ascii', errors='replace').ljust(4, b'\x00')[:4]

    msin = (DLT_TYPE_LOG << 1) | 0x00
    noar = 1

    extended_header = struct.pack('<BB', msin, noar) + apid_bytes + ctid_bytes

    payload_bytes = payload.encode('utf-8', errors='replace')

    message_content = ecu_bytes + struct.pack('<I', session_id) + struct.pack('<I', timestamp_value)
    message_content += extended_header
    message_content += payload_bytes

    htyp = DLT_HTYP_UEH | DLT_HTYP_WEID | DLT_HTYP_WSID | DLT_HTYP_WTMS
    mcnt = 0

    standard_header_extra_size = 4 + 4 + 4

    message = struct.pack('<BB', htyp, mcnt)
    message += struct.pack('<H', len(message_content))

    message += message_content

    return DLT_SERIAL_HEADER + message

def generate_messages():
    messages = []
    base_time = datetime(2026, 5, 11, 10, 0, 0)

    log_entries = [
        (0, DLT_LOG_INFO, "APP1", "MAIN", "Application starting..."),
        (1, DLT_LOG_INFO, "APP1", "INIT", "System initialization begin"),
        (2, DLT_LOG_DEBUG, "APP1", "INIT", "Loading configuration from /etc/app/config.json"),
        (3, DLT_LOG_DEBUG, "APP1", "INIT", "Configuration loaded: 15 parameters"),
        (4, DLT_LOG_INFO, "APP1", "DB", "Database connection established"),
        (5, DLT_LOG_DEBUG, "APP1", "DB", "Connection pool size: 10"),
        (6, DLT_LOG_INFO, "APP1", "NET", "Network interfaces initialized"),
        (7, DLT_LOG_DEBUG, "APP1", "NET", "Available interfaces: eth0, wlan0, can0"),
        (8, DLT_LOG_INFO, "APP1", "INIT", "All services initialized successfully"),

        (10, DLT_LOG_DEBUG, "CAN", "BUS", "CAN0: Initializing at 500kbps"),
        (11, DLT_LOG_DEBUG, "CAN", "BUS", "CAN0: Setting filter for ID 0x100-0x1FF"),
        (12, DLT_LOG_DEBUG, "CAN", "BUS", "CAN0: TX - ID: 0x100, DLC: 8"),
        (13, DLT_LOG_DEBUG, "CAN", "BUS", "CAN0: RX - ID: 0x200, DLC: 8"),
        (14, DLT_LOG_DEBUG, "CAN", "BUS", "CAN0: Message acknowledged"),
        (15, DLT_LOG_INFO, "CAN", "BUS", "CAN0: Communication established"),
        (16, DLT_LOG_DEBUG, "CAN", "BUS", "CAN0: Periodic message task started"),
        (17, DLT_LOG_DEBUG, "CAN", "BUS", "CAN0: Timeout check - all nodes responding"),
        (18, DLT_LOG_ERROR, "CAN", "BUS", "CAN0: TX Error - Bus error detected"),
        (19, DLT_LOG_ERROR, "CAN", "BUS", "CAN0: Error code: 0x00000002"),
        (20, DLT_LOG_INFO, "CAN", "BUS", "CAN0: Initiating recovery sequence"),
        (21, DLT_LOG_DEBUG, "CAN", "BUS", "CAN0: Recovery - resetting controller"),
        (22, DLT_LOG_DEBUG, "CAN", "BUS", "CAN0: Recovery - re-enabling bus"),
        (23, DLT_LOG_INFO, "CAN", "BUS", "CAN0: Bus recovered successfully"),

        (30, DLT_LOG_ERROR, "APP1", "DB", "Database connection lost"),
        (31, DLT_LOG_ERROR, "APP1", "DB", "Error: Connection timeout after 30s"),
        (32, DLT_LOG_DEBUG, "APP1", "DB", "Retrying connection (attempt 1/5)"),
        (33, DLT_LOG_DEBUG, "APP1", "DB", "Retry failed - host unreachable"),
        (34, DLT_LOG_ERROR, "APP1", "DB", "Database unavailable - using cache"),
        (35, DLT_LOG_INFO, "APP1", "CACHE", "Cache mode activated"),
        (36, DLT_LOG_DEBUG, "APP1", "DB", "Connection restored"),
        (37, DLT_LOG_INFO, "APP1", "DB", "Database reconnected"),
        (38, DLT_LOG_INFO, "APP1", "CACHE", "Normal operation resumed"),

        (40, DLT_LOG_WARN, "APP1", "AUTH", "Authentication attempt failed for user: admin"),
        (41, DLT_LOG_WARN, "APP1", "AUTH", "Reason: Invalid password"),
        (42, DLT_LOG_DEBUG, "APP1", "AUTH", "IP: 192.168.1.100, Attempts: 1/5"),
        (43, DLT_LOG_WARN, "APP1", "AUTH", "Authentication attempt failed for user: admin"),
        (44, DLT_LOG_WARN, "APP1", "AUTH", "Reason: Invalid password"),
        (45, DLT_LOG_DEBUG, "APP1", "AUTH", "IP: 192.168.1.100, Attempts: 2/5"),
        (46, DLT_LOG_WARN, "APP1", "AUTH", "Authentication attempt failed for user: admin"),
        (47, DLT_LOG_WARN, "APP1", "AUTH", "Reason: Invalid password"),
        (48, DLT_LOG_DEBUG, "APP1", "AUTH", "IP: 192.168.1.100, Attempts: 3/5"),
        (49, DLT_LOG_ERROR, "APP1", "AUTH", "Account locked: admin@192.168.1.100"),
        (50, DLT_LOG_ERROR, "APP1", "AUTH", "Security alert: Multiple failed attempts"),

        (60, DLT_LOG_INFO, "APP1", "SERV", "Service Manager starting..."),
        (61, DLT_LOG_DEBUG, "APP1", "SERV", "Loading service definitions"),
        (62, DLT_LOG_INFO, "APP1", "SERV", "Starting service: telemetry"),
        (63, DLT_LOG_DEBUG, "APP1", "TELE", "Telemetry service initialized"),
        (64, DLT_LOG_DEBUG, "APP1", "TELE", "Sample rate: 100 Hz"),
        (65, DLT_LOG_INFO, "APP1", "SERV", "Starting service: diagnostics"),
        (66, DLT_LOG_DEBUG, "APP1", "DIAG", "Diagnostics service initialized"),
        (67, DLT_LOG_INFO, "APP1", "SERV", "Starting service: logging"),
        (68, DLT_LOG_DEBUG, "APP1", "LOG", "Logging service initialized"),
        (69, DLT_LOG_DEBUG, "APP1", "LOG", "Log level: DEBUG"),
        (70, DLT_LOG_DEBUG, "APP1", "LOG", "Buffer size: 10000 entries"),
        (71, DLT_LOG_INFO, "APP1", "SERV", "All services started successfully"),
        (72, DLT_LOG_DEBUG, "APP1", "SERV", "Service health check: OK"),

        (80, DLT_LOG_WARN, "APP1", "MEM", "Memory usage at 75%"),
        (81, DLT_LOG_WARN, "APP1", "MEM", "Available: 256 MB / 1024 MB"),
        (82, DLT_LOG_DEBUG, "APP1", "MEM", "GC recommended"),
        (83, DLT_LOG_WARN, "APP1", "MEM", "Memory usage at 85%"),
        (84, DLT_LOG_WARN, "APP1", "MEM", "Available: 153 MB / 1024 MB"),
        (85, DLT_LOG_DEBUG, "APP1", "MEM", "GC triggered"),
        (86, DLT_LOG_INFO, "APP1", "MEM", "GC completed - freed 50 MB"),
        (87, DLT_LOG_WARN, "APP1", "CPU", "CPU temperature at 75C"),
        (88, DLT_LOG_WARN, "APP1", "CPU", "Thermal throttling: pending"),
        (89, DLT_LOG_DEBUG, "APP1", "CPU", "Fan speed: 4500 RPM"),

        (90, DLT_LOG_DEBUG, "APP1", "TCP", "Opening TCP connection to 192.168.1.50:8080"),
        (91, DLT_LOG_DEBUG, "APP1", "TCP", "Socket created: fd=5"),
        (92, DLT_LOG_DEBUG, "APP1", "TCP", "Connection timeout set: 30s"),
        (93, DLT_LOG_DEBUG, "APP1", "TCP", "TCP connected successfully"),
        (94, DLT_LOG_DEBUG, "APP1", "TCP", "Local: 192.168.1.10:54321 -> Remote: 192.168.1.50:8080"),
        (95, DLT_LOG_DEBUG, "APP1", "TCP", "TX - 256 bytes"),
        (96, DLT_LOG_DEBUG, "APP1", "TCP", "Data: GET /api/v1/status HTTP/1.1"),
        (97, DLT_LOG_DEBUG, "APP1", "TCP", "RX - 512 bytes"),
        (98, DLT_LOG_DEBUG, "APP1", "TCP", "Response: HTTP/1.1 200 OK"),
        (99, DLT_LOG_INFO, "APP1", "TCP", "TCP session established"),

        (100, DLT_LOG_DEBUG, "APP1", "SENS", "Sensor reading: temp=23.5C"),
        (101, DLT_LOG_DEBUG, "APP1", "SENS", "Sensor reading: temp=23.6C"),
        (102, DLT_LOG_DEBUG, "APP1", "SENS", "Sensor reading: temp=23.5C"),
        (103, DLT_LOG_DEBUG, "APP1", "SENS", "Sensor reading: temp=23.4C"),
        (104, DLT_LOG_DEBUG, "APP1", "SENS", "Sensor reading: temp=23.5C"),
        (105, DLT_LOG_DEBUG, "APP1", "SENS", "Sensor reading: temp=23.6C"),

        (110, DLT_LOG_FATAL, "APP1", "KERN", "Kernel panic: Out of memory"),
        (111, DLT_LOG_FATAL, "APP1", "KERN", "System cannot recover"),
        (112, DLT_LOG_ERROR, "APP1", "KERN", "Initiating emergency shutdown"),
        (113, DLT_LOG_INFO, "APP1", "KERN", "Saving state to persistent storage"),
        (114, DLT_LOG_ERROR, "APP1", "KERN", "Write failed: Insufficient resources"),
        (115, DLT_LOG_WARN, "APP1", "KERN", "State backup unavailable"),
        (116, DLT_LOG_INFO, "APP1", "KERN", "Emergency shutdown complete"),

        (120, DLT_LOG_DEBUG, "APP1", "FILE", "Opening file: /data/logs/app.log"),
        (121, DLT_LOG_DEBUG, "APP1", "FILE", "File size: 10485760 bytes (10 MB)"),
        (122, DLT_LOG_DEBUG, "APP1", "FILE", "File opened successfully (fd=8)"),
        (123, DLT_LOG_INFO, "APP1", "FILE", "Rotating log file"),
        (124, DLT_LOG_DEBUG, "APP1", "FILE", "Creating backup: /data/logs/app.log.1"),
        (125, DLT_LOG_DEBUG, "APP1", "FILE", "Backup created: 10485760 bytes"),
        (126, DLT_LOG_DEBUG, "APP1", "FILE", "Truncating original file"),
        (127, DLT_LOG_INFO, "APP1", "FILE", "Log rotation complete"),

        (130, DLT_LOG_DEBUG, "APP1", "SQL", "Executing query: SELECT * FROM telemetry"),
        (131, DLT_LOG_DEBUG, "APP1", "SQL", "Query timeout: 30s"),
        (132, DLT_LOG_DEBUG, "APP1", "SQL", "Query result: 1250 rows"),
        (133, DLT_LOG_INFO, "APP1", "SQL", "Query executed in 500ms"),
        (134, DLT_LOG_DEBUG, "APP1", "SQL", "Executing transaction: INSERT INTO events"),
        (135, DLT_LOG_DEBUG, "APP1", "SQL", "Transaction ID: TXN-2026-001"),
        (136, DLT_LOG_DEBUG, "APP1", "SQL", "Affected rows: 1"),
        (137, DLT_LOG_INFO, "APP1", "SQL", "Transaction committed"),

        (140, DLT_LOG_VERBOSE, "APP1", "TRACE", "Entering function: processTelemetryData()"),
        (141, DLT_LOG_VERBOSE, "APP1", "TRACE", "Parameter: timestamp=1715404020"),
        (142, DLT_LOG_VERBOSE, "APP1", "TRACE", "Parameter: sensor_id=SENSOR_001"),
        (143, DLT_LOG_VERBOSE, "APP1", "TRACE", "Parameter: value=23.456"),
        (144, DLT_LOG_VERBOSE, "APP1", "TRACE", "Calling: validateSensorValue()"),
        (145, DLT_LOG_VERBOSE, "APP1", "TRACE", "Result: valid=true"),
        (146, DLT_LOG_VERBOSE, "APP1", "TRACE", "Exiting function: processTelemetryData()"),

        (150, DLT_LOG_INFO, "APP1", "TEST", "Test block for summary query"),
        (151, DLT_LOG_DEBUG, "APP1", "TEST", "This block contains all log levels"),
        (152, DLT_LOG_VERBOSE, "APP1", "TEST", "VERBOSE: Most detailed logging"),
        (153, DLT_LOG_DEBUG, "APP1", "TEST", "DEBUG: Debugging information"),
        (154, DLT_LOG_INFO, "APP1", "TEST", "INFO: General information"),
        (155, DLT_LOG_WARN, "APP1", "TEST", "WARN: Warning conditions"),
        (156, DLT_LOG_ERROR, "APP1", "TEST", "ERROR: Error conditions"),
        (157, DLT_LOG_FATAL, "APP1", "TEST", "FATAL: Critical errors"),
        (158, DLT_LOG_INFO, "APP1", "TEST", "Test block complete"),

        (160, DLT_LOG_INFO, "APP1", "SEARCH", "Searching for keyword: timeout"),
        (161, DLT_LOG_DEBUG, "APP1", "SEARCH", "Timeout value: 30 seconds"),
        (162, DLT_LOG_DEBUG, "APP1", "SEARCH", "Connection timeout detected"),
        (163, DLT_LOG_ERROR, "APP1", "SEARCH", "Request timeout after 30s"),

        (170, DLT_LOG_INFO, "APP1", "ECU_GW", "ECU_GW: Gateway ECU initializing"),
        (171, DLT_LOG_DEBUG, "APP1", "ECU_GW", "ECU_GW: Loading gateway configuration"),
        (172, DLT_LOG_INFO, "APP1", "ECU_GW", "ECU_GW: Gateway ready"),

        (180, DLT_LOG_INFO, "APP1", "ECU_HMI", "ECU_HMI: HMI ECU initializing"),
        (181, DLT_LOG_DEBUG, "APP1", "ECU_HMI", "ECU_HMI: Loading HMI configuration"),
        (182, DLT_LOG_INFO, "APP1", "ECU_HMI", "ECU_HMI: HMI ready"),

        (190, DLT_LOG_INFO, "APP1", "MAIN", "Application shutdown initiated"),
        (191, DLT_LOG_INFO, "APP1", "MAIN", "Saving configuration"),
        (192, DLT_LOG_DEBUG, "APP1", "MAIN", "Flushing log buffer"),
        (193, DLT_LOG_INFO, "APP1", "MAIN", "Closing database connections"),
        (194, DLT_LOG_DEBUG, "APP1", "MAIN", "Connections closed: 10"),
        (195, DLT_LOG_INFO, "APP1", "MAIN", "Shutdown complete"),
        (196, DLT_LOG_INFO, "APP1", "MAIN", "Application terminated normally"),
    ]

    for offset, level, apid, ctid, payload in log_entries:
        msg_time = base_time + timedelta(seconds=offset)
        seconds = int(msg_time.timestamp())
        microseconds = int((msg_time.timestamp() - seconds) * 1000000)
        ts_value = offset * 10000

        messages.append({
            'timestamp': msg_time,
            'level': level,
            'apid': apid,
            'ctid': ctid,
            'payload': payload,
            'timestamp_value': ts_value
        })

    return messages

def main():
    output_file = "/home/user/Desktop/dlt-viewer-plugin/dlt-viewer/plugin/dltchatplugin/mock_dlt_log.dlt"
    messages = generate_messages()

    all_data = bytearray()
    for msg in messages:
        all_data += create_dlt_message(
            msg['timestamp'],
            msg['level'],
            msg['apid'],
            msg['ctid'],
            msg['payload'],
            "ECU1",
            1,
            msg['timestamp_value']
        )

    with open(output_file, 'wb') as f:
        f.write(all_data)

    size = len(all_data)
    print(f"DLT file created: {output_file}")
    print(f"File size: {size} bytes ({size/1024:.1f} KB)")
    print(f"Total messages: {len(messages)}")

    print("\nLog level distribution:")
    levels = {}
    for m in messages:
        levels[m['level']] = levels.get(m['level'], 0) + 1
    level_names = {1: 'FATAL', 2: 'ERROR', 3: 'WARN', 4: 'INFO', 5: 'DEBUG', 6: 'VERBOSE'}
    for lvl, count in sorted(levels.items()):
        print(f"  {level_names.get(lvl, 'UNKNOWN')}: {count}")

if __name__ == "__main__":
    main()
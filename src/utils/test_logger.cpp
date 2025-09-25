#include "test_logger.h"
#include <stdarg.h>

// Global logger instance
TestLogger test_logger;

TestLogger::TestLogger() {
    spiffs_initialized = false;
    console_output_enabled = true;
    file_output_enabled = true;
    use_memory_fallback = false;
    session_start_time = 0;
    active_timer_count = 0;

    // Initialize session ID with timestamp
    sprintf(session_id, "TEST_%08X", (unsigned int)ESP.getCycleCount());

    // Initialize memory logs
    for (int i = 0; i < 6; i++) {
        memory_logs[i] = "";
        memory_logs[i].reserve(MAX_MEMORY_LOG_SIZE);
    }

    // Initialize file paths (SPIFFS doesn't support directories, use flat structure)
    log_file_paths[LOG_FILE_MAIN] = "/main_test.log";
    log_file_paths[LOG_FILE_CONFIDENCE] = "/confidence_test.log";
    log_file_paths[LOG_FILE_DETECTOR] = "/detector_test.log";
    log_file_paths[LOG_FILE_INTEGRATION] = "/integration_test.log";
    log_file_paths[LOG_FILE_SENSOR_DATA] = "/sensor_data.log";
    log_file_paths[LOG_FILE_PERFORMANCE] = "/performance.log";
}

TestLogger::~TestLogger() {
    cleanup();
}

bool TestLogger::init() {
    session_start_time = millis();

    // Try to initialize SPIFFS
    if (SPIFFS.begin(true)) {
        spiffs_initialized = true;
        use_memory_fallback = false;

        if (console_output_enabled) {
            Serial.println("[LOGGER] SPIFFS initialized successfully");
        }
    } else {
        if (console_output_enabled) {
            Serial.println("[LOGGER] SPIFFS initialization failed! Attempting format...");
        }

        if (formatSPIFFS() && SPIFFS.begin(true)) {
            spiffs_initialized = true;
            use_memory_fallback = false;

            if (console_output_enabled) {
                Serial.println("[LOGGER] SPIFFS formatted and initialized successfully");
            }
        } else {
            // SPIFFS failed, fall back to memory logging
            spiffs_initialized = false;
            use_memory_fallback = true;

            if (console_output_enabled) {
                Serial.println("[LOGGER] SPIFFS failed! Using memory-based logging fallback");
                Serial.println("[LOGGER] Logs will be stored in RAM and can be exported via serial");
            }
        }
    }

    // Log initialization
    logSystemInfo();
    log(LOG_INFO, LOG_FILE_MAIN, "Test Logger initialized - Session: " + String(session_id));
    log(LOG_INFO, LOG_FILE_MAIN, "Storage mode: " + String(use_memory_fallback ? "Memory fallback" : "SPIFFS"));

    if (console_output_enabled) {
        Serial.println("[LOGGER] Test Logger initialized successfully");
        Serial.println("[LOGGER] Session ID: " + String(session_id));
        Serial.println("[LOGGER] Storage: " + String(use_memory_fallback ? "Memory" : "SPIFFS"));
    }

    return true;  // Always return true since we have memory fallback
}

void TestLogger::cleanup() {
    // Close all open files
    for (int i = 0; i < 6; i++) {
        if (log_files[i]) {
            log_files[i].close();
        }
    }

    if (spiffs_initialized) {
        log(LOG_INFO, LOG_FILE_MAIN, "Test Logger session ended");
        SPIFFS.end();
        spiffs_initialized = false;
    }
}

bool TestLogger::formatSPIFFS() {
    if (console_output_enabled) {
        Serial.println("[LOGGER] Formatting SPIFFS... This may take a while.");
    }

    bool result = SPIFFS.format();

    if (result) {
        if (console_output_enabled) {
            Serial.println("[LOGGER] SPIFFS formatted successfully");
        }
        // Try to begin again after format
        result = SPIFFS.begin(true);
    }

    return result;
}

void TestLogger::enableConsoleOutput(bool enabled) {
    console_output_enabled = enabled;
}

void TestLogger::enableFileOutput(bool enabled) {
    file_output_enabled = enabled;
}

void TestLogger::setSessionId(const char* new_session_id) {
    strncpy(session_id, new_session_id, sizeof(session_id) - 1);
    session_id[sizeof(session_id) - 1] = '\0';
}

String TestLogger::getLogLevelString(LogLevel level) {
    switch (level) {
        case LOG_INFO: return "INFO";
        case LOG_WARNING: return "WARN";
        case LOG_ERROR: return "ERROR";
        case LOG_DEBUG: return "DEBUG";
        case LOG_TEST_RESULT: return "TEST";
        case LOG_SENSOR_DATA: return "SENSOR";
        case LOG_DETECTION_EVENT: return "DETECT";
        default: return "UNKNOWN";
    }
}

String TestLogger::getTimestamp() {
    uint32_t current_time = millis();
    uint32_t elapsed = current_time - session_start_time;

    uint32_t hours = elapsed / 3600000;
    uint32_t minutes = (elapsed % 3600000) / 60000;
    uint32_t seconds = (elapsed % 60000) / 1000;
    uint32_t milliseconds = elapsed % 1000;

    return String(hours) + ":" +
           (minutes < 10 ? "0" : "") + String(minutes) + ":" +
           (seconds < 10 ? "0" : "") + String(seconds) + "." +
           (milliseconds < 100 ? "0" : "") + (milliseconds < 10 ? "0" : "") + String(milliseconds);
}

String TestLogger::getLogFileName(LogFileType type) {
    return log_file_paths[type];
}

bool TestLogger::ensureLogFileOpen(LogFileType type) {
    if (!spiffs_initialized || !file_output_enabled) {
        return false;
    }

    if (log_files[type]) {
        return true; // Already open
    }

    String filename = getLogFileName(type);
    log_files[type] = SPIFFS.open(filename, FILE_APPEND);

    if (!log_files[type]) {
        if (console_output_enabled) {
            Serial.println("[LOGGER] Failed to open log file: " + filename);
        }
        return false;
    }

    return true;
}

void TestLogger::writeToFile(LogFileType type, const String& message) {
    if (ensureLogFileOpen(type)) {
        log_files[type].println(message);
        log_files[type].flush(); // Ensure data is written immediately
    }
}

void TestLogger::log(LogLevel level, LogFileType file_type, const String& message) {
    String timestamp = getTimestamp();
    String level_str = getLogLevelString(level);
    String formatted_message = "[" + timestamp + "] [" + level_str + "] " + message;

    // Output to console if enabled (but we want it disabled for clean output)
    if (console_output_enabled) {
        Serial.println(formatted_message);
    }

    // Output to storage if enabled
    if (file_output_enabled) {
        if (use_memory_fallback) {
            writeToMemory(file_type, formatted_message);
        } else {
            writeToFile(file_type, formatted_message);
        }
    }
}

void TestLogger::log(LogLevel level, LogFileType file_type, const char* format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    log(level, file_type, String(buffer));
}

void TestLogger::logTestStart(const char* test_name) {
    log(LOG_TEST_RESULT, LOG_FILE_MAIN, "=== TEST STARTED: " + String(test_name) + " ===");
}

void TestLogger::logTestEnd(const char* test_name, bool passed, uint32_t duration_ms) {
    String result = passed ? "PASSED" : "FAILED";
    log(LOG_TEST_RESULT, LOG_FILE_MAIN, "=== TEST " + result + ": " + String(test_name) +
        " (Duration: " + String(duration_ms) + "ms) ===");
}

void TestLogger::logTestResult(LogFileType file_type, const char* test_name, bool passed, const char* details) {
    String result = passed ? "✓ PASS" : "✗ FAIL";
    String message = result + ": " + String(test_name);
    if (details) {
        message += " - " + String(details);
    }
    log(LOG_TEST_RESULT, file_type, message);
}

void TestLogger::logSensorData(const char* sensor_name, float value1, float value2, float value3) {
    String message = String(sensor_name) + ": " + String(value1, 3);
    if (value2 != 0) message += ", " + String(value2, 3);
    if (value3 != 0) message += ", " + String(value3, 3);
    log(LOG_SENSOR_DATA, LOG_FILE_SENSOR_DATA, message);
}

void TestLogger::logDetectionEvent(const char* event_type, const char* details) {
    String message = String(event_type) + ": " + String(details);
    log(LOG_DETECTION_EVENT, LOG_FILE_MAIN, message);
}

void TestLogger::logSystemInfo() {
    log(LOG_INFO, LOG_FILE_MAIN, "=== SYSTEM INFORMATION ===");
    log(LOG_INFO, LOG_FILE_MAIN, "ESP32 Chip Model: " + String(ESP.getChipModel()));
    log(LOG_INFO, LOG_FILE_MAIN, "Chip Revision: " + String(ESP.getChipRevision()));
    log(LOG_INFO, LOG_FILE_MAIN, "CPU Frequency: " + String(ESP.getCpuFreqMHz()) + " MHz");
    log(LOG_INFO, LOG_FILE_MAIN, "Flash Size: " + String(ESP.getFlashChipSize() / (1024 * 1024)) + " MB");
    log(LOG_INFO, LOG_FILE_MAIN, "Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
    log(LOG_INFO, LOG_FILE_MAIN, "SPIFFS Total: " + String(SPIFFS.totalBytes()) + " bytes");
    log(LOG_INFO, LOG_FILE_MAIN, "SPIFFS Used: " + String(SPIFFS.usedBytes()) + " bytes");
    log(LOG_INFO, LOG_FILE_MAIN, "========================");
}

void TestLogger::logMemoryUsage() {
    log(LOG_INFO, LOG_FILE_PERFORMANCE, "Memory - Free Heap: " + String(ESP.getFreeHeap()) +
        " bytes, Largest Block: " + String(ESP.getMaxAllocHeap()) + " bytes");
}

void TestLogger::listLogFiles() {
    if (!spiffs_initialized) {
        if (console_output_enabled) {
            Serial.println("[LOGGER] SPIFFS not initialized");
        }
        return;
    }

    File root = SPIFFS.open("/");
    if (!root || !root.isDirectory()) {
        if (console_output_enabled) {
            Serial.println("[LOGGER] Cannot access SPIFFS root");
        }
        return;
    }

    if (console_output_enabled) {
        Serial.println("\n=== LOG FILES ===");
    }

    File file = root.openNextFile();
    while (file) {
        String filename = String(file.name());
        // Only list our log files (ending with .log)
        if (filename.endsWith(".log")) {
            if (console_output_enabled) {
                Serial.printf("%-25s %6d bytes\n", file.name(), (int)file.size());
            }
            log(LOG_INFO, LOG_FILE_MAIN, "Log file: " + filename + " (" + String(file.size()) + " bytes)");
        }
        file = root.openNextFile();
    }

    if (console_output_enabled) {
        Serial.println("================\n");
    }
}

bool TestLogger::deleteLogFile(LogFileType type) {
    if (!spiffs_initialized) return false;

    String filename = getLogFileName(type);

    // Close file if open
    if (log_files[type]) {
        log_files[type].close();
    }

    return SPIFFS.remove(filename);
}

bool TestLogger::deleteAllLogs() {
    if (!spiffs_initialized) return false;

    bool success = true;
    for (int i = 0; i < 6; i++) {
        if (!deleteLogFile((LogFileType)i)) {
            success = false;
        }
    }

    return success;
}

size_t TestLogger::getLogFileSize(LogFileType type) {
    if (!spiffs_initialized) return 0;

    String filename = getLogFileName(type);
    File file = SPIFFS.open(filename, FILE_READ);
    if (!file) return 0;

    size_t size = file.size();
    file.close();
    return size;
}

String TestLogger::readLogFile(LogFileType type, size_t max_bytes) {
    if (!spiffs_initialized) return "";

    String filename = getLogFileName(type);
    File file = SPIFFS.open(filename, FILE_READ);
    if (!file) return "";

    size_t file_size = file.size();
    if (max_bytes == 0 || max_bytes > file_size) {
        max_bytes = file_size;
    }

    String content;
    content.reserve(max_bytes + 1);

    size_t bytes_read = 0;
    while (file.available() && bytes_read < max_bytes) {
        content += (char)file.read();
        bytes_read++;
    }

    file.close();
    return content;
}

bool TestLogger::exportLogsToSerial() {
    if (!console_output_enabled) return false;

    Serial.println("\n=== EXPORTING LOGS TO SERIAL ===");
    Serial.println("Storage Mode: " + String(use_memory_fallback ? "Memory" : "SPIFFS"));

    for (int i = 0; i < 6; i++) {
        LogFileType type = (LogFileType)i;
        String content;

        if (use_memory_fallback) {
            content = memory_logs[type];
        } else {
            content = readLogFile(type);
        }

        if (content.length() > 0) {
            Serial.println("\n--- " + getLogTypeName(type) + " ---");
            Serial.println(content);
            Serial.println("--- END OF " + getLogTypeName(type) + " ---");
        }
    }

    Serial.println("=== END OF LOG EXPORT ===\n");
    return true;
}

void TestLogger::printLogFileSummary() {
    if (!console_output_enabled) return;

    Serial.println("\n=== LOG SUMMARY ===");
    Serial.println("Storage: " + String(use_memory_fallback ? "Memory (RAM)" : "SPIFFS (Flash)"));

    size_t total_size = 0;
    for (int i = 0; i < 6; i++) {
        LogFileType type = (LogFileType)i;
        size_t size;

        if (use_memory_fallback) {
            size = memory_logs[type].length();
        } else {
            size = getLogFileSize(type);
        }

        total_size += size;

        Serial.printf("%-20s: %6d bytes\n", getLogTypeName(type).c_str(), (int)size);
    }

    Serial.printf("Total log size: %d bytes\n", (int)total_size);

    if (!use_memory_fallback && spiffs_initialized) {
        Serial.printf("SPIFFS usage: %d/%d bytes (%.1f%%)\n",
                      (int)SPIFFS.usedBytes(), (int)SPIFFS.totalBytes(),
                      (float)SPIFFS.usedBytes() / SPIFFS.totalBytes() * 100);
    } else {
        Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    }

    Serial.println("===================\n");
}

void TestLogger::startTimer(const char* timer_name) {
    if (active_timer_count >= 10) return; // Max timers reached

    Timer& timer = active_timers[active_timer_count];
    strncpy(timer.name, timer_name, sizeof(timer.name) - 1);
    timer.name[sizeof(timer.name) - 1] = '\0';
    timer.start_time = millis();
    timer.active = true;

    active_timer_count++;

    log(LOG_DEBUG, LOG_FILE_PERFORMANCE, "Timer started: " + String(timer_name));
}

void TestLogger::endTimer(const char* timer_name) {
    for (int i = 0; i < active_timer_count; i++) {
        Timer& timer = active_timers[i];
        if (timer.active && strcmp(timer.name, timer_name) == 0) {
            uint32_t duration = millis() - timer.start_time;
            timer.active = false;

            log(LOG_INFO, LOG_FILE_PERFORMANCE, "Timer '" + String(timer_name) +
                "' completed in " + String(duration) + "ms");

            // Compact the array
            for (int j = i; j < active_timer_count - 1; j++) {
                active_timers[j] = active_timers[j + 1];
            }
            active_timer_count--;
            break;
        }
    }
}

void TestLogger::writeToMemory(LogFileType type, const String& message) {
    if (memory_logs[type].length() + message.length() + 1 > MAX_MEMORY_LOG_SIZE) {
        // If adding this message would exceed the limit, truncate from the beginning
        size_t excess = (memory_logs[type].length() + message.length() + 1) - MAX_MEMORY_LOG_SIZE + 512; // Extra buffer
        if (excess < memory_logs[type].length()) {
            memory_logs[type] = memory_logs[type].substring(excess);
            memory_logs[type] += "\n[...LOG TRUNCATED...]\n";
        } else {
            // Message is too large, clear and start fresh
            memory_logs[type] = "[...LOG CLEARED - MESSAGE TOO LARGE...]\n";
        }
    }

    memory_logs[type] += message + "\n";
}

String TestLogger::getLogTypeName(LogFileType type) {
    switch (type) {
        case LOG_FILE_MAIN: return "Main Test Log";
        case LOG_FILE_CONFIDENCE: return "Confidence Test Log";
        case LOG_FILE_DETECTOR: return "Detector Test Log";
        case LOG_FILE_INTEGRATION: return "Integration Test Log";
        case LOG_FILE_SENSOR_DATA: return "Sensor Data Log";
        case LOG_FILE_PERFORMANCE: return "Performance Log";
        default: return "Unknown Log";
    }
}
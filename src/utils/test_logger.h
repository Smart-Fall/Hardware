#ifndef TEST_LOGGER_H
#define TEST_LOGGER_H

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>

// Log levels for different types of output
enum LogLevel {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_DEBUG,
    LOG_TEST_RESULT,
    LOG_SENSOR_DATA,
    LOG_DETECTION_EVENT
};

// Log file types for organized storage
enum LogFileType {
    LOG_FILE_MAIN,          // Main test execution log
    LOG_FILE_CONFIDENCE,    // Confidence scorer test results
    LOG_FILE_DETECTOR,      // Fall detector test results
    LOG_FILE_INTEGRATION,   // Integration test results
    LOG_FILE_SENSOR_DATA,   // Raw sensor data during tests
    LOG_FILE_PERFORMANCE    // Performance and timing data
};

class TestLogger {
private:
    bool spiffs_initialized;
    bool console_output_enabled;
    bool file_output_enabled;
    bool use_memory_fallback;
    uint32_t session_start_time;
    char session_id[16];

    // File handles for different log types
    File log_files[6];
    String log_file_paths[6];

    // Memory fallback for when SPIFFS is not available
    String memory_logs[6];
    static const size_t MAX_MEMORY_LOG_SIZE = 8192;  // 8KB per log type

    // Helper functions
    String getLogLevelString(LogLevel level);
    String getTimestamp();
    String getLogFileName(LogFileType type);
    bool ensureLogFileOpen(LogFileType type);
    void writeToFile(LogFileType type, const String& message);
    void writeToMemory(LogFileType type, const String& message);
    String getLogTypeName(LogFileType type);

public:
    TestLogger();
    ~TestLogger();

    // Initialization and cleanup
    bool init();
    void cleanup();
    bool formatSPIFFS();

    // Configuration
    void enableConsoleOutput(bool enabled);
    void enableFileOutput(bool enabled);
    void setSessionId(const char* session_id);

    // Logging functions
    void log(LogLevel level, LogFileType file_type, const String& message);
    void log(LogLevel level, LogFileType file_type, const char* format, ...);

    // Convenience functions for different log types
    void logTestStart(const char* test_name);
    void logTestEnd(const char* test_name, bool passed, uint32_t duration_ms);
    void logTestResult(LogFileType file_type, const char* test_name, bool passed, const char* details = nullptr);
    void logSensorData(const char* sensor_name, float value1, float value2 = 0, float value3 = 0);
    void logDetectionEvent(const char* event_type, const char* details);
    void logSystemInfo();
    void logMemoryUsage();

    // File management
    void listLogFiles();
    bool deleteLogFile(LogFileType type);
    bool deleteAllLogs();
    size_t getLogFileSize(LogFileType type);
    String readLogFile(LogFileType type, size_t max_bytes = 0);

    // Export functions
    bool exportLogsToSerial();
    void printLogFileSummary();

    // Performance tracking
    void startTimer(const char* timer_name);
    void endTimer(const char* timer_name);

private:
    // Performance tracking
    struct Timer {
        char name[32];
        uint32_t start_time;
        bool active;
    };
    Timer active_timers[10];
    int active_timer_count;
};

// Global logger instance
extern TestLogger test_logger;

// Convenience macros for easier logging
#define LOG_INFO_MAIN(msg) test_logger.log(LOG_INFO, LOG_FILE_MAIN, msg)
#define LOG_ERROR_MAIN(msg) test_logger.log(LOG_ERROR, LOG_FILE_MAIN, msg)
#define LOG_TEST_PASS(file_type, test_name) test_logger.logTestResult(file_type, test_name, true)
#define LOG_TEST_FAIL(file_type, test_name, details) test_logger.logTestResult(file_type, test_name, false, details)
#define LOG_DETECTION(event, details) test_logger.logDetectionEvent(event, details)
#define LOG_SENSOR(sensor, v1, v2, v3) test_logger.logSensorData(sensor, v1, v2, v3)

#endif // TEST_LOGGER_H
#pragma once
#include <Arduino.h>

// Forward declaration to avoid circular include
class WiFi_Manager;

// Configurable via Config.h
#ifndef LOG_BUFFER_SIZE
#define LOG_BUFFER_SIZE 30
#endif

#ifndef LOG_BATCH_INTERVAL_MS
#define LOG_BATCH_INTERVAL_MS 30000
#endif

#ifndef ENABLE_REMOTE_LOGGING
#define ENABLE_REMOTE_LOGGING true
#endif

typedef enum {
  LOG_LEVEL_DEBUG = 0,
  LOG_LEVEL_INFO,
  LOG_LEVEL_WARN,
  LOG_LEVEL_ERROR
} LogLevel_t;

typedef enum {
  LOG_CAT_SYSTEM = 0,
  LOG_CAT_FALL_DETECTION,
  LOG_CAT_SENSOR,
  LOG_CAT_WIFI,
  LOG_CAT_EMERGENCY
} LogCategory_t;

struct LogEntry_t {
  LogLevel_t    level;
  LogCategory_t category;
  char          message[128];
  float         value;      // optional numeric context (0 = unused)
  float         threshold;  // optional threshold (0 = unused)
  uint32_t      ms;         // millis() at creation
};

class Log_Manager {
public:
  Log_Manager() {}

  void begin(WiFi_Manager* wifi, const char* deviceId);
  void log(LogLevel_t level, LogCategory_t category, const char* message,
           float value = 0.0f, float threshold = 0.0f);
  void flush();           // Send batch if interval elapsed
  void flushImmediate();  // Force send now (e.g., on fall detection)
  bool isReady() const { return _initialized; }

private:
  WiFi_Manager*  _wifi      = nullptr;
  char           _deviceId[32] = "";
  LogEntry_t     _buffer[LOG_BUFFER_SIZE];
  uint8_t        _head      = 0;   // index of oldest entry
  uint8_t        _count     = 0;   // entries currently in buffer
  uint32_t       _lastFlush = 0;
  bool           _initialized = false;

  const char* levelStr(LogLevel_t l);
  const char* categoryStr(LogCategory_t c);
  bool        sendBatch();
  String      buildJSON();
};

extern Log_Manager logManager;

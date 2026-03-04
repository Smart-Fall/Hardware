#include "Log_Manager.h"
#include "WiFi_Manager.h"
#include "Config.h"

// Global singleton
Log_Manager logManager;

void Log_Manager::begin(WiFi_Manager* wifi, const char* deviceId) {
  _wifi = wifi;
  strncpy(_deviceId, deviceId, sizeof(_deviceId) - 1);
  _deviceId[sizeof(_deviceId) - 1] = '\0';
  _head      = 0;
  _count     = 0;
  _lastFlush = millis();
  _initialized = true;
  Serial.println("[LogManager] Initialized");
}

void Log_Manager::log(LogLevel_t level, LogCategory_t category,
                      const char* message, float value, float threshold) {
  if (!_initialized) return;

  // Write into next slot (ring buffer — overwrites oldest when full)
  uint8_t idx = (_head + _count) % LOG_BUFFER_SIZE;
  LogEntry_t& entry = _buffer[idx];
  entry.level     = level;
  entry.category  = category;
  entry.value     = value;
  entry.threshold = threshold;
  entry.ms        = millis();
  strncpy(entry.message, message, sizeof(entry.message) - 1);
  entry.message[sizeof(entry.message) - 1] = '\0';

  if (_count < LOG_BUFFER_SIZE) {
    _count++;
  } else {
    // Buffer full — advance head to discard oldest
    _head = (_head + 1) % LOG_BUFFER_SIZE;
  }

  // Mirror to serial
  Serial.printf("[%s][%s] %s", levelStr(level), categoryStr(category), message);
  if (value != 0.0f || threshold != 0.0f) {
    Serial.printf(" (val=%.3f thr=%.3f)", value, threshold);
  }
  Serial.println();
}

void Log_Manager::flush() {
  if (!_initialized || !ENABLE_REMOTE_LOGGING) return;
  if (_count == 0) return;
  if ((millis() - _lastFlush) < LOG_BATCH_INTERVAL_MS) return;
  sendBatch();
}

void Log_Manager::flushImmediate() {
  if (!_initialized || !ENABLE_REMOTE_LOGGING) return;
  if (_count == 0) return;
  sendBatch();
}

bool Log_Manager::sendBatch() {
  if (_wifi == nullptr || !_wifi->isConnected()) {
    Serial.println("[LogManager] Cannot send — WiFi not connected");
    return false;
  }

  Serial.printf("[LogManager] Sending batch (%d entries)\n", _count);
  String json = buildJSON();
  bool ok = _wifi->sendJSONToEndpoint("/api/device/logs", json);

  if (ok) {
    _head  = 0;
    _count = 0;
    Serial.println("[LogManager] Batch sent successfully");
  } else {
    Serial.println("[LogManager] Batch send failed — will retry");
  }

  _lastFlush = millis();
  return ok;
}

String Log_Manager::buildJSON() {
  String json = "{\"device_id\":\"";
  json += _deviceId;
  json += "\",\"logs\":[";

  for (uint8_t i = 0; i < _count; i++) {
    const LogEntry_t& e = _buffer[(_head + i) % LOG_BUFFER_SIZE];
    if (i > 0) json += ",";
    json += "{\"level\":\"";
    json += levelStr(e.level);
    json += "\",\"category\":\"";
    json += categoryStr(e.category);
    json += "\",\"message\":\"";
    // Escape double quotes in message
    for (int c = 0; e.message[c] != '\0'; c++) {
      if (e.message[c] == '"') json += "\\\"";
      else json += e.message[c];
    }
    json += "\"";
    if (e.value != 0.0f || e.threshold != 0.0f) {
      json += ",\"value\":";
      json += String(e.value, 4);
      json += ",\"threshold\":";
      json += String(e.threshold, 4);
    }
    json += "}";
  }

  json += "]}";
  return json;
}

const char* Log_Manager::levelStr(LogLevel_t l) {
  switch (l) {
    case LOG_LEVEL_DEBUG: return "DEBUG";
    case LOG_LEVEL_INFO:  return "INFO";
    case LOG_LEVEL_WARN:  return "WARN";
    case LOG_LEVEL_ERROR: return "ERROR";
    default:              return "INFO";
  }
}

const char* Log_Manager::categoryStr(LogCategory_t c) {
  switch (c) {
    case LOG_CAT_SYSTEM:         return "SYSTEM";
    case LOG_CAT_FALL_DETECTION: return "FALL_DETECTION";
    case LOG_CAT_SENSOR:         return "SENSOR";
    case LOG_CAT_WIFI:           return "WIFI";
    case LOG_CAT_EMERGENCY:      return "EMERGENCY";
    default:                     return "SYSTEM";
  }
}

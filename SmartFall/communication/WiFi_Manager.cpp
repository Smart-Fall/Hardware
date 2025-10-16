#include "WiFi_Manager.h"
#include <ArduinoJson.h>

WiFi_Manager::WiFi_Manager() : initialized(false), connected(false),
                                 last_reconnect_attempt(0), reconnect_interval(30000),
                                 connection_attempts(0), auto_reconnect(true),
                                 last_status_check(0) {
}

WiFi_Manager::~WiFi_Manager() {
    if (connected) {
        disconnect();
    }
}

bool WiFi_Manager::begin() {
    // Use credentials from config.h
    return begin(WIFI_SSID, WIFI_PASSWORD);
}

bool WiFi_Manager::begin(const char* ssid_param, const char* password_param) {
    if (initialized) {
        Serial.println("[WiFi] Already initialized");
        return true;
    }

    ssid = String(ssid_param);
    password = String(password_param);

    if (ssid.length() == 0) {
        Serial.println("[WiFi] ERROR: SSID is empty!");
        return false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);  // We handle reconnection manually

    initialized = true;
    Serial.println("[WiFi] Manager initialized");

    return connect();
}

void WiFi_Manager::setServerURL(const char* url) {
    server_url = String(url);
    if (DEBUG_COMMUNICATION) {
        Serial.print("[WiFi] Server URL set to: ");
        Serial.println(server_url);
    }
}

bool WiFi_Manager::connect() {
    return connect(ssid.c_str(), password.c_str());
}

bool WiFi_Manager::connect(const char* ssid_param, const char* password_param) {
    if (!initialized && !begin(ssid_param, password_param)) {
        return false;
    }

    if (connected) {
        Serial.println("[WiFi] Already connected");
        return true;
    }

    Serial.print("[WiFi] Connecting to: ");
    Serial.println(ssid_param);

    WiFi.begin(ssid_param, password_param);

    uint32_t start_time = millis();
    connection_attempts++;

    while (WiFi.status() != WL_CONNECTED && (millis() - start_time) < WIFI_TIMEOUT_MS) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        connected = true;
        connection_attempts = 0;
        Serial.println("[WiFi] ✓ Connected!");
        printConnectionInfo();
        return true;
    } else {
        connected = false;
        Serial.print("[WiFi] Connection failed (attempt ");
        Serial.print(connection_attempts);
        Serial.println(")");
        return false;
    }
}

void WiFi_Manager::disconnect() {
    if (connected) {
        WiFi.disconnect();
        connected = false;
        Serial.println("[WiFi] Disconnected");
    }
}

bool WiFi_Manager::reconnect() {
    Serial.println("[WiFi] Attempting reconnection...");
    disconnect();
    delay(1000);
    return connect();
}

bool WiFi_Manager::isConnected() {
    updateConnectionStatus();
    return connected;
}

void WiFi_Manager::enableAutoReconnect(bool enable) {
    auto_reconnect = enable;
    if (DEBUG_COMMUNICATION) {
        Serial.print("[WiFi] Auto-reconnect: ");
        Serial.println(enable ? "enabled" : "disabled");
    }
}

void WiFi_Manager::checkConnection() {
    if (!initialized || !auto_reconnect) {
        return;
    }

    uint32_t current_time = millis();

    // Check status every 5 seconds
    if (current_time - last_status_check >= 5000) {
        last_status_check = current_time;
        updateConnectionStatus();

        if (!connected && (current_time - last_reconnect_attempt >= reconnect_interval)) {
            last_reconnect_attempt = current_time;
            Serial.println("[WiFi] Connection lost, attempting reconnect...");
            reconnect();
        }
    }
}

String WiFi_Manager::getSSID() {
    return WiFi.SSID();
}

int8_t WiFi_Manager::getSignalStrength() {
    return WiFi.RSSI();
}

String WiFi_Manager::getIPAddress() {
    return WiFi.localIP().toString();
}

String WiFi_Manager::getMACAddress() {
    return WiFi.macAddress();
}

bool WiFi_Manager::sendEmergencyAlert(const EmergencyData_t& emergency_data) {
    if (!connected) {
        Serial.println("[WiFi] Cannot send alert - not connected");
        return false;
    }

    if (server_url.length() == 0) {
        Serial.println("[WiFi] ERROR: Server URL not set!");
        return false;
    }

    String json_payload = createEmergencyJSON(emergency_data);
    String response;

    String endpoint = server_url + "/api/emergency";

    if (DEBUG_COMMUNICATION) {
        Serial.println("[WiFi] Sending emergency alert...");
        Serial.println(json_payload);
    }

    bool success = sendHTTPPost(endpoint.c_str(), json_payload);

    if (success) {
        Serial.println("[WiFi] ✓ Emergency alert sent successfully");
    } else {
        Serial.println("[WiFi] ✗ Failed to send emergency alert");
    }

    return success;
}

bool WiFi_Manager::sendStatusUpdate(const StatusData_t& status_data) {
    if (!connected) return false;

    String json_payload = createStatusJSON(status_data);
    String endpoint = server_url + "/api/status";

    return sendHTTPPost(endpoint.c_str(), json_payload);
}

bool WiFi_Manager::sendSensorData(const SensorData_t& sensor_data) {
    if (!connected) return false;

    String json_payload = createSensorDataJSON(sensor_data);
    String endpoint = server_url + "/api/sensor";

    return sendHTTPPost(endpoint.c_str(), json_payload);
}

bool WiFi_Manager::sendHTTPPost(const char* endpoint, const String& json_payload) {
    if (!connected) {
        Serial.println("[WiFi] Cannot send POST - not connected");
        return false;
    }

    http.begin(endpoint);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(10000);  // 10-second timeout

    int http_code = http.POST(json_payload);

    bool success = (http_code == HTTP_CODE_OK || http_code == HTTP_CODE_CREATED);

    if (DEBUG_COMMUNICATION) {
        Serial.print("[WiFi] POST ");
        Serial.print(endpoint);
        Serial.print(" - Status: ");
        Serial.println(http_code);
    }

    http.end();
    return success;
}

bool WiFi_Manager::sendHTTPGet(const char* endpoint, String& response) {
    if (!connected) return false;

    http.begin(endpoint);
    http.setTimeout(10000);

    int http_code = http.GET();

    if (http_code == HTTP_CODE_OK) {
        response = http.getString();
        http.end();
        return true;
    }

    http.end();
    return false;
}

void WiFi_Manager::setReconnectInterval(uint32_t interval_ms) {
    reconnect_interval = interval_ms;
}

uint8_t WiFi_Manager::getConnectionAttempts() {
    return connection_attempts;
}

void WiFi_Manager::resetConnectionAttempts() {
    connection_attempts = 0;
}

void WiFi_Manager::printConnectionInfo() {
    Serial.println("=== WiFi Connection Info ===");
    Serial.print("SSID: ");
    Serial.println(getSSID());
    Serial.print("IP Address: ");
    Serial.println(getIPAddress());
    Serial.print("MAC Address: ");
    Serial.println(getMACAddress());
    Serial.print("Signal Strength: ");
    Serial.print(getSignalStrength());
    Serial.println(" dBm");
    Serial.println("============================");
}

void WiFi_Manager::printNetworkStatus() {
    Serial.print("[WiFi] Status: ");
    if (connected) {
        Serial.print("Connected to ");
        Serial.print(getSSID());
        Serial.print(" (");
        Serial.print(getSignalStrength());
        Serial.println(" dBm)");
    } else {
        Serial.println("Not connected");
    }
}

bool WiFi_Manager::isInitialized() {
    return initialized;
}

// Private helper functions

String WiFi_Manager::createEmergencyJSON(const EmergencyData_t& data) {
    DynamicJsonDocument doc(4096);

    doc["timestamp"] = data.timestamp;
    doc["confidence_score"] = data.confidence_score;
    doc["confidence_level"] = data.confidence;
    doc["battery_level"] = data.battery_level;
    doc["sos_triggered"] = data.sos_triggered;
    doc["device_id"] = String(data.device_id);

    // Add sensor history (last 10 samples for brevity)
    JsonArray history = doc.createNestedArray("sensor_history");
    for (int i = 90; i < 100; i++) {
        JsonObject sample = history.createNestedObject();
        sample["timestamp"] = data.sensor_history[i].timestamp;
        sample["accel_x"] = data.sensor_history[i].accel_x;
        sample["accel_y"] = data.sensor_history[i].accel_y;
        sample["accel_z"] = data.sensor_history[i].accel_z;
        sample["gyro_x"] = data.sensor_history[i].gyro_x;
        sample["gyro_y"] = data.sensor_history[i].gyro_y;
        sample["gyro_z"] = data.sensor_history[i].gyro_z;
        sample["heart_rate"] = data.sensor_history[i].heart_rate;
    }

    String json_string;
    serializeJson(doc, json_string);
    return json_string;
}

String WiFi_Manager::createStatusJSON(const StatusData_t& data) {
    DynamicJsonDocument doc(512);

    doc["timestamp"] = data.timestamp;
    doc["battery_level"] = data.battery_level;
    doc["system_health"] = data.system_health;
    doc["uptime"] = data.uptime;
    doc["status_message"] = String(data.status_message);

    String json_string;
    serializeJson(doc, json_string);
    return json_string;
}

String WiFi_Manager::createSensorDataJSON(const SensorData_t& data) {
    DynamicJsonDocument doc(512);

    doc["timestamp"] = data.timestamp;
    doc["accel_x"] = data.accel_x;
    doc["accel_y"] = data.accel_y;
    doc["accel_z"] = data.accel_z;
    doc["gyro_x"] = data.gyro_x;
    doc["gyro_y"] = data.gyro_y;
    doc["gyro_z"] = data.gyro_z;
    doc["pressure"] = data.pressure;
    doc["heart_rate"] = data.heart_rate;
    doc["fsr_value"] = data.fsr_value;

    String json_string;
    serializeJson(doc, json_string);
    return json_string;
}

void WiFi_Manager::updateConnectionStatus() {
    bool prev_connected = connected;
    connected = (WiFi.status() == WL_CONNECTED);

    if (prev_connected && !connected) {
        Serial.println("[WiFi] Connection lost!");
    } else if (!prev_connected && connected) {
        Serial.println("[WiFi] Connection restored!");
    }
}

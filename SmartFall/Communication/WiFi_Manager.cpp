#include "WiFi_Manager.h"
#include <ArduinoJson.h>

WiFi_Manager::WiFi_Manager() {
    lastReconnectAttempt = 0;
    reconnectInterval = 30000;  // 30 seconds
    autoReconnect = false;
    initialized = false;
}

bool WiFi_Manager::begin(const char* wifi_ssid, const char* wifi_password) {
    ssid = String(wifi_ssid);
    password = String(wifi_password);

    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_password);

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected successfully!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.print("Signal Strength (RSSI): ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        initialized = true;
        return true;
    } else {
        Serial.println("WiFi connection failed!");
        initialized = false;
        return false;
    }
}

void WiFi_Manager::setServerURL(const char* url) {
    serverURL = String(url);
    Serial.print("Server URL set to: ");
    Serial.println(serverURL);
}

void WiFi_Manager::enableAutoReconnect(bool enable) {
    autoReconnect = enable;
    Serial.print("Auto-reconnect: ");
    Serial.println(enable ? "Enabled" : "Disabled");
}

bool WiFi_Manager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WiFi_Manager::checkConnection() {
    if (!autoReconnect) return;

    if (!isConnected()) {
        unsigned long currentTime = millis();
        if (currentTime - lastReconnectAttempt >= reconnectInterval) {
            lastReconnectAttempt = currentTime;
            Serial.println("\n[WiFi] Connection lost. Attempting to reconnect...");
            reconnect();
        }
    }
}

bool WiFi_Manager::reconnect() {
    WiFi.disconnect();
    delay(100);
    return begin(ssid.c_str(), password.c_str());
}

bool WiFi_Manager::sendTestMessage(const String& message) {
    if (!isConnected()) {
        Serial.println("Error: WiFi not connected!");
        return false;
    }

    if (serverURL.length() == 0) {
        Serial.println("Error: Server URL not set!");
        return false;
    }

    HTTPClient http;
    http.begin(serverURL);
    http.addHeader("Content-Type", "text/plain");

    Serial.print("Sending test message to: ");
    Serial.println(serverURL);

    int httpResponseCode = http.POST(message);

    if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String response = http.getString();
        Serial.print("Server response: ");
        Serial.println(response);
        http.end();
        return true;
    } else {
        Serial.print("Error sending message. HTTP error code: ");
        Serial.println(httpResponseCode);
        http.end();
        return false;
    }
}

bool WiFi_Manager::sendJSON(const String& jsonPayload) {
    if (!isConnected()) {
        Serial.println("Error: WiFi not connected!");
        return false;
    }

    if (serverURL.length() == 0) {
        Serial.println("Error: Server URL not set!");
        return false;
    }

    HTTPClient http;
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");

    Serial.println("Sending JSON payload:");
    Serial.println(jsonPayload);

    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String response = http.getString();
        Serial.print("Server response: ");
        Serial.println(response);
        http.end();
        return true;
    } else {
        Serial.print("Error sending JSON. HTTP error code: ");
        Serial.println(httpResponseCode);
        http.end();
        return false;
    }
}

void WiFi_Manager::printConnectionInfo() {
    Serial.println("\n=== WiFi Connection Info ===");
    Serial.print("Status: ");
    Serial.println(isConnected() ? "Connected" : "Disconnected");

    if (isConnected()) {
        Serial.print("SSID: ");
        Serial.println(WiFi.SSID());
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.print("Gateway: ");
        Serial.println(WiFi.gatewayIP());
        Serial.print("Subnet Mask: ");
        Serial.println(WiFi.subnetMask());
        Serial.print("DNS: ");
        Serial.println(WiFi.dnsIP());
        Serial.print("Signal Strength (RSSI): ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        Serial.print("MAC Address: ");
        Serial.println(WiFi.macAddress());
    }
    Serial.println("============================\n");
}

String WiFi_Manager::getLocalIP() {
    return WiFi.localIP().toString();
}

int WiFi_Manager::getRSSI() {
    return WiFi.RSSI();
}

bool WiFi_Manager::sendEmergencyAlert(const EmergencyData_t& emergency_data) {
    if (!isConnected()) {
        Serial.println("[WiFi] Error: Not connected!");
        return false;
    }

    DynamicJsonDocument doc(2048);
    doc["device_id"] = String(emergency_data.device_id);
    doc["timestamp"] = emergency_data.timestamp;
    doc["confidence_score"] = emergency_data.confidence_score;
    doc["confidence_level"] = emergency_data.confidence;
    doc["battery_level"] = emergency_data.battery_level;
    doc["sos_triggered"] = emergency_data.sos_triggered;

    JsonObject sensors = doc.createNestedObject("sensor_data");
    sensors["accel_x"] = emergency_data.sensor_history[0].accel_x;
    sensors["accel_y"] = emergency_data.sensor_history[0].accel_y;
    sensors["accel_z"] = emergency_data.sensor_history[0].accel_z;
    sensors["gyro_x"] = emergency_data.sensor_history[0].gyro_x;
    sensors["gyro_y"] = emergency_data.sensor_history[0].gyro_y;
    sensors["gyro_z"] = emergency_data.sensor_history[0].gyro_z;
    sensors["pressure"] = emergency_data.sensor_history[0].pressure;

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    String endpoint = getEndpointURL("/api/falls");

    HTTPClient http;
    http.begin(endpoint);
    http.addHeader("Content-Type", "application/json");

    Serial.println("[WiFi] Sending emergency alert to: " + endpoint);

    int httpResponseCode = http.POST(jsonPayload);

    bool success = (httpResponseCode >= 200 && httpResponseCode < 300);

    if (success) {
        Serial.print("[WiFi] Emergency alert sent successfully (");
        Serial.print(httpResponseCode);
        Serial.println(")");
    } else {
        Serial.print("[WiFi] Emergency alert failed (");
        Serial.print(httpResponseCode);
        Serial.println(")");
    }

    http.end();
    return success;
}

bool WiFi_Manager::sendStatusUpdate(const SystemStatus_t& status_data) {
    if (!isConnected()) {
        return false;
    }

    DynamicJsonDocument doc(512);
    doc["battery_level"] = status_data.battery_percentage;
    doc["wifi_connected"] = status_data.wifi_connected;
    doc["bluetooth_connected"] = status_data.bluetooth_connected;
    doc["sensors_initialized"] = status_data.sensors_initialized;
    doc["uptime"] = status_data.uptime_ms;
    doc["current_status"] = status_data.current_status;

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    String endpoint = getEndpointURL("/api/device/status");

    HTTPClient http;
    http.begin(endpoint);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Device-ID", WiFi.macAddress());

    int httpResponseCode = http.POST(jsonPayload);
    bool success = (httpResponseCode >= 200 && httpResponseCode < 300);

    http.end();
    return success;
}

bool WiFi_Manager::sendSensorData(const SensorData_t& sensor_data) {
    if (!isConnected()) {
        return false;
    }

    DynamicJsonDocument doc(512);
    doc["accel_x"] = sensor_data.accel_x;
    doc["accel_y"] = sensor_data.accel_y;
    doc["accel_z"] = sensor_data.accel_z;
    doc["gyro_x"] = sensor_data.gyro_x;
    doc["gyro_y"] = sensor_data.gyro_y;
    doc["gyro_z"] = sensor_data.gyro_z;
    doc["pressure"] = sensor_data.pressure;

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    String endpoint = getEndpointURL("/api/device/sensor-stream");

    HTTPClient http;
    http.begin(endpoint);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Device-ID", WiFi.macAddress());

    int httpResponseCode = http.POST(jsonPayload);
    bool success = (httpResponseCode >= 200 && httpResponseCode < 300);

    http.end();
    return success;
}

String WiFi_Manager::getEndpointURL(const char* endpoint) {
    String url = serverURL;
    if (!url.endsWith("/")) {
        url += endpoint;
    } else {
        url += (endpoint[0] == '/') ? (endpoint + 1) : endpoint;
    }
    return url;
}

bool WiFi_Manager::pingServer() {
    if (!isConnected()) {
        Serial.println("[WiFi] Cannot ping server - WiFi not connected");
        return false;
    }

    String endpoint = getEndpointURL("/api/health");

    HTTPClient http;
    http.begin(endpoint);
    http.setTimeout(5000); // 5 second timeout

    Serial.print("[WiFi] Pinging server: ");
    Serial.println(endpoint);

    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
        String response = http.getString();
        Serial.println("[WiFi] ✓ Server ping successful!");
        Serial.print("[WiFi] Response: ");
        Serial.println(response);
        http.end();
        return true;
    } else {
        Serial.print("[WiFi] ✗ Server ping failed (HTTP ");
        Serial.print(httpResponseCode);
        Serial.println(")");
        http.end();
        return false;
    }
}

bool WiFi_Manager::testServerConnection() {
    if (!isConnected()) {
        Serial.println("[WiFi] Cannot test server - WiFi not connected");
        return false;
    }

    Serial.println("\n[WiFi] ========== SERVER CONNECTION TEST ==========");
    Serial.print("[WiFi] Server URL: ");
    Serial.println(serverURL);
    Serial.print("[WiFi] Device MAC: ");
    Serial.println(WiFi.macAddress());

    String endpoint = getEndpointURL("/api/health");

    HTTPClient http;
    http.begin(endpoint);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Device-ID", WiFi.macAddress());
    http.setTimeout(5000);

    // Create test JSON
    DynamicJsonDocument doc(256);
    doc["device_id"] = "SF-" + WiFi.macAddress();
    doc["test"] = true;

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    Serial.println("[WiFi] Sending POST request to /api/health...");

    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode >= 200 && httpResponseCode < 300) {
        Serial.print("[WiFi] ✓ Server connection TEST PASSED! (HTTP ");
        Serial.print(httpResponseCode);
        Serial.println(")");

        String response = http.getString();
        Serial.println("[WiFi] Server response:");
        Serial.println(response);

        http.end();
        Serial.println("[WiFi] =============================================\n");
        return true;
    } else {
        Serial.print("[WiFi] ✗ Server connection TEST FAILED (HTTP ");
        Serial.print(httpResponseCode);
        Serial.println(")");

        if (httpResponseCode == -1) {
            Serial.println("[WiFi] Error: Connection refused or timeout");
            Serial.println("[WiFi] Check:");
            Serial.println("[WiFi]   1. Web app is running (npm run dev)");
            Serial.println("[WiFi]   2. SERVER_URL IP address is correct");
            Serial.println("[WiFi]   3. Computer firewall allows port 3000");
        } else if (httpResponseCode == 404) {
            Serial.println("[WiFi] Error: API endpoint not found");
            Serial.println("[WiFi] Check: Web app has /api/health endpoint");
        }

        http.end();
        Serial.println("[WiFi] =============================================\n");
        return false;
    }
}

#include "WiFi_Manager.h"

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

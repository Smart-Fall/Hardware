#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <Arduino.h>
#include "Config.h"

class WiFi_Manager
{
private:
    String ssid;
    String password;
    String serverURL;
    unsigned long lastReconnectAttempt;
    unsigned long reconnectInterval;
    bool autoReconnect;
    bool initialized;
    uint8_t reconnectFailCount;
    WiFiClient plainClient;
    WiFiClientSecure secureClient;

    static const uint8_t MAX_RETRIES = WIFI_CONNECT_MAX_RETRIES;
    static const uint16_t RETRY_DELAY_MS = WIFI_CONNECT_RETRY_DELAY_MS;
    static const uint8_t HTTP_MAX_RETRIES = WIFI_HTTP_MAX_RETRIES;
    static const uint16_t HTTP_RETRY_DELAY_MS = WIFI_HTTP_RETRY_DELAY_MS;

public:
    WiFi_Manager();

    // Initialization
    bool begin(const char *wifi_ssid, const char *wifi_password);
    void setServerURL(const char *url);
    void enableAutoReconnect(bool enable);

    // Connection management
    bool isConnected();
    void checkConnection();
    bool reconnect();
    void shutdown();

    // HTTP communication
    bool sendTestMessage(const String &message);
    bool sendJSON(const String &jsonPayload);
    bool sendJSONToEndpoint(const String &path, const String &jsonPayload);
    String getFromEndpoint(const String &path);

    // Utility
    void printConnectionInfo();
    String getLocalIP();
    int getRSSI();

private:
    bool beginHTTP(HTTPClient &http, const String &url);
    bool isHTTPS(const String &url);
    void addDeviceAuthHeader(HTTPClient &http);
    bool isHTTPSuccess(int httpStatusCode);
};

#endif // WIFI_MANAGER_H

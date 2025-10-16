#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <HTTPClient.h>

class WiFi_Manager {
private:
    String ssid;
    String password;
    String serverURL;
    unsigned long lastReconnectAttempt;
    unsigned long reconnectInterval;
    bool autoReconnect;
    bool initialized;

public:
    WiFi_Manager();

    // Initialization
    bool begin(const char* wifi_ssid, const char* wifi_password);
    void setServerURL(const char* url);
    void enableAutoReconnect(bool enable);

    // Connection management
    bool isConnected();
    void checkConnection();
    bool reconnect();

    // HTTP communication
    bool sendTestMessage(const String& message);
    bool sendJSON(const String& jsonPayload);

    // Utility
    void printConnectionInfo();
    String getLocalIP();
    int getRSSI();
};

#endif // WIFI_MANAGER_H

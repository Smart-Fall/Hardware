#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino.h>

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

    // Emergency and data transmission
    bool sendEmergencyAlert(const EmergencyData_t& emergency_data);
    bool sendStatusUpdate(const SystemStatus_t& status_data, const char* device_id = nullptr);
    bool sendSensorData(const SensorData_t& sensor_data, const char* device_id = nullptr);

    // Server connectivity
    bool pingServer();
    bool testServerConnection();

    // Utility
    void printConnectionInfo();
    String getLocalIP();
    int getRSSI();

private:
    String getEndpointURL(const char* endpoint);
};

#endif // WIFI_MANAGER_H

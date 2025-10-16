#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "../utils/data_types.h"
#include "../utils/config.h"

class WiFi_Manager {
private:
    bool initialized;
    bool connected;
    String ssid;
    String password;
    String server_url;
    uint32_t last_reconnect_attempt;
    uint32_t reconnect_interval;
    uint8_t connection_attempts;

    // Connection state
    bool auto_reconnect;
    uint32_t last_status_check;

    // HTTP client
    HTTPClient http;

public:
    WiFi_Manager();
    ~WiFi_Manager();

    // Initialization
    bool begin(const char* ssid, const char* password);
    bool begin();  // Use credentials from config.h
    void setServerURL(const char* url);

    // Connection management
    bool connect();
    bool connect(const char* ssid, const char* password);
    void disconnect();
    bool reconnect();
    bool isConnected();

    // Auto-reconnect
    void enableAutoReconnect(bool enable = true);
    void checkConnection();  // Call in loop to maintain connection

    // Network information
    String getSSID();
    int8_t getSignalStrength();  // RSSI in dBm
    String getIPAddress();
    String getMACAddress();

    // Emergency alert transmission
    bool sendEmergencyAlert(const EmergencyData_t& emergency_data);
    bool sendStatusUpdate(const StatusData_t& status_data);
    bool sendSensorData(const SensorData_t& sensor_data);

    // HTTP requests
    bool sendHTTPPost(const char* endpoint, const String& json_payload);
    bool sendHTTPGet(const char* endpoint, String& response);

    // Utility functions
    void setReconnectInterval(uint32_t interval_ms);
    uint8_t getConnectionAttempts();
    void resetConnectionAttempts();

    // Debug functions
    void printConnectionInfo();
    void printNetworkStatus();
    bool isInitialized();

private:
    // Internal helper functions
    String createEmergencyJSON(const EmergencyData_t& data);
    String createStatusJSON(const StatusData_t& data);
    String createSensorDataJSON(const SensorData_t& data);
    bool performHTTPRequest(const String& url, const String& payload, String& response);
    void updateConnectionStatus();
};

#endif // WIFI_MANAGER_H

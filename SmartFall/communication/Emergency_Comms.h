#ifndef EMERGENCY_COMMS_H
#define EMERGENCY_COMMS_H

#include <Arduino.h>
#include "WiFi_Manager.h"
#include "BLE_Server.h"
#include "../utils/data_types.h"
#include "../utils/config.h"

// Alert transmission status
typedef enum {
    ALERT_STATUS_PENDING,
    ALERT_STATUS_SENDING,
    ALERT_STATUS_SENT_WIFI,
    ALERT_STATUS_SENT_BLE,
    ALERT_STATUS_SENT_BOTH,
    ALERT_STATUS_FAILED,
    ALERT_STATUS_RETRY
} AlertStatus_t;

class Emergency_Comms {
private:
    WiFi_Manager* wifi_manager;
    BLE_Server* ble_server;

    bool wifi_enabled;
    bool ble_enabled;
    bool initialized;

    // Alert state
    AlertStatus_t current_alert_status;
    uint8_t retry_count;
    uint8_t max_retries;
    uint32_t last_alert_time;
    uint32_t retry_interval;

    // Emergency data queue
    EmergencyData_t pending_alert;
    bool alert_pending;

public:
    Emergency_Comms(WiFi_Manager* wifi, BLE_Server* ble);
    ~Emergency_Comms();

    // Initialization
    bool begin();
    void setMaxRetries(uint8_t retries);
    void setRetryInterval(uint32_t interval_ms);

    // Enable/disable protocols
    void enableWiFi(bool enable = true);
    void enableBLE(bool enable = true);
    bool isWiFiEnabled();
    bool isBLEEnabled();

    // Emergency alert transmission
    bool sendEmergencyAlert(const EmergencyData_t& emergency_data);
    bool sendEmergencyAlert(const EmergencyData_t& emergency_data, bool urgent);

    // Status updates
    bool sendStatusUpdate(const SystemStatus_t& status_data);
    bool sendSensorData(const SensorData_t& sensor_data);

    // Alert management
    void processAlertQueue();  // Call in loop to handle retries
    AlertStatus_t getAlertStatus();
    bool isAlertPending();
    void clearPendingAlert();

    // Connection status
    bool isConnected();  // Returns true if either WiFi or BLE is connected
    bool isWiFiConnected();
    bool isBLEConnected();

    // Utility functions
    void printStatus();
    bool isInitialized();
    uint8_t getRetryCount();

private:
    // Internal transmission functions
    bool sendViaWiFi(const EmergencyData_t& data);
    bool sendViaBLE(const EmergencyData_t& data);
    bool retryFailedAlert();
    void updateAlertStatus();

    // Helper functions
    String getAlertStatusString(AlertStatus_t status);
};

#endif // EMERGENCY_COMMS_H

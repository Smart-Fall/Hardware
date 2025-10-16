#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "../utils/data_types.h"
#include "../utils/config.h"

// SmartFall BLE Service UUIDs
#define SERVICE_UUID                "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define EMERGENCY_CHARACTERISTIC    "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define SENSOR_CHARACTERISTIC       "beb5483f-36e1-4688-b7f5-ea07361b26a8"
#define STATUS_CHARACTERISTIC       "beb54840-36e1-4688-b7f5-ea07361b26a8"
#define COMMAND_CHARACTERISTIC      "beb54841-36e1-4688-b7f5-ea07361b26a8"
#define CONFIG_CHARACTERISTIC       "beb54842-36e1-4688-b7f5-ea07361b26a8"

// BLE Commands
#define BLE_CMD_CANCEL_ALERT        0x01
#define BLE_CMD_TEST_ALERT          0x02
#define BLE_CMD_GET_STATUS          0x03
#define BLE_CMD_SET_CONFIG          0x04
#define BLE_CMD_START_STREAMING     0x05
#define BLE_CMD_STOP_STREAMING      0x06

class BLE_Server {
private:
    bool initialized;
    bool device_connected;
    bool streaming_enabled;
    uint32_t last_notification;
    uint32_t notification_interval;

    String device_name;

    // BLE objects
    BLEServer* ble_server;
    BLEService* ble_service;
    BLECharacteristic* emergency_char;
    BLECharacteristic* sensor_char;
    BLECharacteristic* status_char;
    BLECharacteristic* command_char;
    BLECharacteristic* config_char;

    // Callback functions
    void (*on_connect_callback)();
    void (*on_disconnect_callback)();
    void (*on_command_callback)(uint8_t command, uint8_t* data, size_t length);

    // Server callbacks
    class ServerCallbacks : public BLEServerCallbacks {
    private:
        BLE_Server* parent;
    public:
        ServerCallbacks(BLE_Server* p) : parent(p) {}
        void onConnect(BLEServer* server);
        void onDisconnect(BLEServer* server);
    };

    // Characteristic callbacks
    class CommandCallbacks : public BLECharacteristicCallbacks {
    private:
        BLE_Server* parent;
    public:
        CommandCallbacks(BLE_Server* p) : parent(p) {}
        void onWrite(BLECharacteristic* characteristic);
    };

    ServerCallbacks* server_callbacks;
    CommandCallbacks* command_callbacks;

public:
    BLE_Server();
    ~BLE_Server();

    // Initialization
    bool begin(const char* device_name);
    bool begin();  // Use default device name
    void end();

    // Connection management
    bool isConnected();
    void startAdvertising();
    void stopAdvertising();

    // Data transmission
    bool sendEmergencyAlert(const EmergencyData_t& emergency_data);
    bool sendSensorData(const SensorData_t& sensor_data);
    bool sendStatusUpdate(const SystemStatus_t& status_data);

    // Streaming mode
    void enableStreaming(bool enable = true);
    bool isStreaming();
    void setStreamingInterval(uint32_t interval_ms);
    bool shouldStream();  // Check if it's time to stream

    // Callback registration
    void onConnect(void (*callback)());
    void onDisconnect(void (*callback)());
    void onCommand(void (*callback)(uint8_t command, uint8_t* data, size_t length));

    // Utility functions
    String getDeviceName();
    bool isInitialized();
    void printConnectionInfo();

private:
    // Internal helper functions
    void createCharacteristics();
    void handleCommand(uint8_t command, uint8_t* data, size_t length);
    bool notifyCharacteristic(BLECharacteristic* characteristic, uint8_t* data, size_t length);

    // JSON conversion helpers
    String createEmergencyJSON(const EmergencyData_t& data);
    String createSensorDataJSON(const SensorData_t& data);
    String createStatusJSON(const SystemStatus_t& data);

    // Friend classes for callbacks
    friend class ServerCallbacks;
    friend class CommandCallbacks;
};

#endif // BLE_SERVER_H

#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// BLE Service UUID
#define SERVICE_UUID                "4fafc201-1fb5-459e-8fcc-c5c9c331914b"

// BLE Characteristic UUIDs
#define EMERGENCY_CHARACTERISTIC    "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define SENSOR_CHARACTERISTIC       "beb5483f-36e1-4688-b7f5-ea07361b26a8"
#define STATUS_CHARACTERISTIC       "beb54840-36e1-4688-b7f5-ea07361b26a8"
#define COMMAND_CHARACTERISTIC      "beb54841-36e1-4688-b7f5-ea07361b26a8"
#define CONFIG_CHARACTERISTIC       "beb54842-36e1-4688-b7f5-ea07361b26a8"

// BLE Commands
#define BLE_CMD_CANCEL_ALERT        0x01
#define BLE_CMD_TEST_ALERT          0x02
#define BLE_CMD_GET_STATUS          0x03
#define BLE_CMD_GET_CONFIG          0x04
#define BLE_CMD_START_STREAMING     0x05
#define BLE_CMD_STOP_STREAMING      0x06

class BLE_Server {
private:
    BLEServer* pServer;
    BLEService* pService;
    BLECharacteristic* pEmergencyChar;
    BLECharacteristic* pSensorChar;
    BLECharacteristic* pStatusChar;
    BLECharacteristic* pCommandChar;
    BLECharacteristic* pConfigChar;

    bool deviceConnected;
    bool initialized;
    String deviceName;

    // Callbacks
    static void onConnect(BLEServer* pServer);
    static void onDisconnect(BLEServer* pServer);

public:
    BLE_Server();

    // Initialization
    bool begin(const char* device_name);

    // Connection status
    bool isConnected();

    // Data transmission
    bool sendEmergency(const String& message);
    bool sendSensorData(const String& data);
    bool sendStatus(const String& status);

    // Utility
    void printConnectionInfo();
    int getConnectedDevices();
};

#endif // BLE_SERVER_H

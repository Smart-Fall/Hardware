#include "BLE_Server.h"

// Static variables for callbacks
static bool g_deviceConnected = false;

// Callback class for server events
class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        g_deviceConnected = true;
        Serial.println("\n[BLE] Client connected!");
    }

    void onDisconnect(BLEServer* pServer) {
        g_deviceConnected = false;
        Serial.println("\n[BLE] Client disconnected!");
        // Restart advertising
        BLEDevice::startAdvertising();
        Serial.println("[BLE] Advertising restarted");
    }
};

// Callback class for characteristic writes
class CommandCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        std::string value = pCharacteristic->getValue();

        if (value.length() > 0) {
            Serial.print("[BLE] Command received: 0x");
            Serial.println((uint8_t)value[0], HEX);

            switch ((uint8_t)value[0]) {
                case BLE_CMD_CANCEL_ALERT:
                    Serial.println("  → Cancel Alert");
                    break;
                case BLE_CMD_TEST_ALERT:
                    Serial.println("  → Test Alert");
                    break;
                case BLE_CMD_GET_STATUS:
                    Serial.println("  → Get Status");
                    break;
                case BLE_CMD_GET_CONFIG:
                    Serial.println("  → Get Config");
                    break;
                case BLE_CMD_START_STREAMING:
                    Serial.println("  → Start Streaming");
                    break;
                case BLE_CMD_STOP_STREAMING:
                    Serial.println("  → Stop Streaming");
                    break;
                default:
                    Serial.println("  → Unknown Command");
                    break;
            }
        }
    }
};

BLE_Server::BLE_Server() {
    pServer = nullptr;
    pService = nullptr;
    pEmergencyChar = nullptr;
    pSensorChar = nullptr;
    pStatusChar = nullptr;
    pCommandChar = nullptr;
    pConfigChar = nullptr;
    deviceConnected = false;
    initialized = false;
    deviceName = "SmartFall";
}

bool BLE_Server::begin(const char* device_name) {
    deviceName = String(device_name);

    Serial.print("Initializing BLE Server: ");
    Serial.println(deviceName);

    // Initialize BLE device
    BLEDevice::init(device_name);

    // Create BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    // Create BLE Service
    pService = pServer->createService(SERVICE_UUID);

    // Create Emergency Characteristic (Notify)
    pEmergencyChar = pService->createCharacteristic(
        EMERGENCY_CHARACTERISTIC,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pEmergencyChar->addDescriptor(new BLE2902());

    // Create Sensor Data Characteristic (Notify)
    pSensorChar = pService->createCharacteristic(
        SENSOR_CHARACTERISTIC,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pSensorChar->addDescriptor(new BLE2902());

    // Create Status Characteristic (Read/Notify)
    pStatusChar = pService->createCharacteristic(
        STATUS_CHARACTERISTIC,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pStatusChar->addDescriptor(new BLE2902());

    // Create Command Characteristic (Write)
    pCommandChar = pService->createCharacteristic(
        COMMAND_CHARACTERISTIC,
        BLECharacteristic::PROPERTY_WRITE
    );
    pCommandChar->setCallbacks(new CommandCallbacks());

    // Create Config Characteristic (Read/Write)
    pConfigChar = pService->createCharacteristic(
        CONFIG_CHARACTERISTIC,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
    );

    // Start the service
    pService->start();

    // Start advertising
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // Functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.println("✓ BLE Server started");
    Serial.println("✓ Service UUID: " + String(SERVICE_UUID));
    Serial.println("✓ Advertising started - Device is discoverable");

    initialized = true;
    return true;
}

bool BLE_Server::isConnected() {
    deviceConnected = g_deviceConnected;
    return deviceConnected;
}

bool BLE_Server::sendEmergency(const String& message) {
    if (!initialized) {
        Serial.println("Error: BLE not initialized!");
        return false;
    }

    pEmergencyChar->setValue(message.c_str());
    pEmergencyChar->notify();

    Serial.print("[BLE] Emergency sent: ");
    Serial.println(message);

    return true;
}

bool BLE_Server::sendSensorData(const String& data) {
    if (!initialized) {
        Serial.println("Error: BLE not initialized!");
        return false;
    }

    pSensorChar->setValue(data.c_str());
    pSensorChar->notify();

    return true;
}

bool BLE_Server::sendStatus(const String& status) {
    if (!initialized) {
        Serial.println("Error: BLE not initialized!");
        return false;
    }

    pStatusChar->setValue(status.c_str());
    pStatusChar->notify();

    Serial.print("[BLE] Status sent: ");
    Serial.println(status);

    return true;
}

void BLE_Server::printConnectionInfo() {
    Serial.println("\n=== BLE Connection Info ===");
    Serial.print("Device Name: ");
    Serial.println(deviceName);
    Serial.print("Status: ");
    Serial.println(isConnected() ? "Connected" : "Advertising");
    Serial.print("Service UUID: ");
    Serial.println(SERVICE_UUID);

    Serial.println("\nCharacteristics:");
    Serial.println("  Emergency:  " + String(EMERGENCY_CHARACTERISTIC) + " (Notify)");
    Serial.println("  Sensor:     " + String(SENSOR_CHARACTERISTIC) + " (Notify)");
    Serial.println("  Status:     " + String(STATUS_CHARACTERISTIC) + " (Notify)");
    Serial.println("  Command:    " + String(COMMAND_CHARACTERISTIC) + " (Write)");
    Serial.println("  Config:     " + String(CONFIG_CHARACTERISTIC) + " (Read/Write)");
    Serial.println("===========================\n");
}

int BLE_Server::getConnectedDevices() {
    return pServer->getConnectedCount();
}

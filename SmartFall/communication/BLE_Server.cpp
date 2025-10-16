#include "BLE_Server.h"
#include <ArduinoJson.h>

// Server Callbacks Implementation
void BLE_Server::ServerCallbacks::onConnect(BLEServer* server) {
    parent->device_connected = true;
    Serial.println("[BLE] Client connected");

    if (parent->on_connect_callback != nullptr) {
        parent->on_connect_callback();
    }
}

void BLE_Server::ServerCallbacks::onDisconnect(BLEServer* server) {
    parent->device_connected = false;
    Serial.println("[BLE] Client disconnected");

    if (parent->on_disconnect_callback != nullptr) {
        parent->on_disconnect_callback();
    }

    // Restart advertising
    parent->startAdvertising();
}

// Command Callbacks Implementation
void BLE_Server::CommandCallbacks::onWrite(BLECharacteristic* characteristic) {
    std::string value = characteristic->getValue();

    if (value.length() > 0) {
        uint8_t command = value[0];
        uint8_t* data = (uint8_t*)value.data() + 1;
        size_t length = value.length() - 1;

        if (DEBUG_COMMUNICATION) {
            Serial.print("[BLE] Received command: 0x");
            Serial.println(command, HEX);
        }

        parent->handleCommand(command, data, length);
    }
}

// BLE_Server Implementation
BLE_Server::BLE_Server() : initialized(false), device_connected(false),
                             streaming_enabled(false), last_notification(0),
                             notification_interval(1000), device_name("SmartFall"),
                             ble_server(nullptr), ble_service(nullptr),
                             emergency_char(nullptr), sensor_char(nullptr),
                             status_char(nullptr), command_char(nullptr),
                             config_char(nullptr), on_connect_callback(nullptr),
                             on_disconnect_callback(nullptr), on_command_callback(nullptr),
                             server_callbacks(nullptr), command_callbacks(nullptr) {
}

BLE_Server::~BLE_Server() {
    end();
}

bool BLE_Server::begin() {
    return begin("SmartFall");
}

bool BLE_Server::begin(const char* name) {
    if (initialized) {
        Serial.println("[BLE] Already initialized");
        return true;
    }

    device_name = String(name);

    Serial.print("[BLE] Initializing as: ");
    Serial.println(device_name);

    // Initialize BLE Device
    BLEDevice::init(device_name.c_str());

    // Create BLE Server
    ble_server = BLEDevice::createServer();

    // Set callbacks
    server_callbacks = new ServerCallbacks(this);
    ble_server->setCallbacks(server_callbacks);

    // Create BLE Service
    ble_service = ble_server->createService(SERVICE_UUID);

    // Create characteristics
    createCharacteristics();

    // Start service
    ble_service->start();

    // Start advertising
    startAdvertising();

    initialized = true;
    Serial.println("[BLE] ✓ Initialized successfully");

    return true;
}

void BLE_Server::end() {
    if (initialized) {
        stopAdvertising();
        BLEDevice::deinit(true);
        initialized = false;
        device_connected = false;

        if (server_callbacks) delete server_callbacks;
        if (command_callbacks) delete command_callbacks;

        Serial.println("[BLE] Service stopped");
    }
}

bool BLE_Server::isConnected() {
    return device_connected;
}

void BLE_Server::startAdvertising() {
    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(SERVICE_UUID);
    advertising->setScanResponse(true);
    advertising->setMinPreferred(0x06);  // Help with iPhone connections
    advertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    if (DEBUG_COMMUNICATION) {
        Serial.println("[BLE] Advertising started");
    }
}

void BLE_Server::stopAdvertising() {
    BLEDevice::stopAdvertising();
    if (DEBUG_COMMUNICATION) {
        Serial.println("[BLE] Advertising stopped");
    }
}

bool BLE_Server::sendEmergencyAlert(const EmergencyData_t& emergency_data) {
    if (!initialized || !device_connected) {
        if (DEBUG_COMMUNICATION) {
            Serial.println("[BLE] Cannot send alert - not connected");
        }
        return false;
    }

    String json_payload = createEmergencyJSON(emergency_data);

    if (DEBUG_COMMUNICATION) {
        Serial.println("[BLE] Sending emergency alert...");
    }

    bool success = notifyCharacteristic(emergency_char,
                                       (uint8_t*)json_payload.c_str(),
                                       json_payload.length());

    if (success) {
        Serial.println("[BLE] ✓ Emergency alert sent");
    } else {
        Serial.println("[BLE] ✗ Failed to send alert");
    }

    return success;
}

bool BLE_Server::sendSensorData(const SensorData_t& sensor_data) {
    if (!initialized || !device_connected) {
        return false;
    }

    if (!streaming_enabled) {
        return false;
    }

    String json_payload = createSensorDataJSON(sensor_data);
    return notifyCharacteristic(sensor_char,
                               (uint8_t*)json_payload.c_str(),
                               json_payload.length());
}

bool BLE_Server::sendStatusUpdate(const SystemStatus_t& status_data) {
    if (!initialized || !device_connected) {
        return false;
    }

    String json_payload = createStatusJSON(status_data);
    return notifyCharacteristic(status_char,
                               (uint8_t*)json_payload.c_str(),
                               json_payload.length());
}

void BLE_Server::enableStreaming(bool enable) {
    streaming_enabled = enable;

    if (DEBUG_COMMUNICATION) {
        Serial.print("[BLE] Streaming: ");
        Serial.println(enable ? "enabled" : "disabled");
    }
}

bool BLE_Server::isStreaming() {
    return streaming_enabled;
}

void BLE_Server::setStreamingInterval(uint32_t interval_ms) {
    notification_interval = interval_ms;
}

bool BLE_Server::shouldStream() {
    if (!streaming_enabled || !device_connected) {
        return false;
    }

    uint32_t current_time = millis();
    if (current_time - last_notification >= notification_interval) {
        last_notification = current_time;
        return true;
    }

    return false;
}

void BLE_Server::onConnect(void (*callback)()) {
    on_connect_callback = callback;
}

void BLE_Server::onDisconnect(void (*callback)()) {
    on_disconnect_callback = callback;
}

void BLE_Server::onCommand(void (*callback)(uint8_t, uint8_t*, size_t)) {
    on_command_callback = callback;
}

String BLE_Server::getDeviceName() {
    return device_name;
}

bool BLE_Server::isInitialized() {
    return initialized;
}

void BLE_Server::printConnectionInfo() {
    Serial.println("=== BLE Connection Info ===");
    Serial.print("Device Name: ");
    Serial.println(device_name);
    Serial.print("Status: ");
    Serial.println(device_connected ? "Connected" : "Advertising");
    Serial.print("Streaming: ");
    Serial.println(streaming_enabled ? "Enabled" : "Disabled");
    Serial.println("===========================");
}

// Private helper functions

void BLE_Server::createCharacteristics() {
    // Emergency Alert Characteristic (Notify)
    emergency_char = ble_service->createCharacteristic(
        EMERGENCY_CHARACTERISTIC,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    emergency_char->addDescriptor(new BLE2902());

    // Sensor Data Characteristic (Notify)
    sensor_char = ble_service->createCharacteristic(
        SENSOR_CHARACTERISTIC,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    sensor_char->addDescriptor(new BLE2902());

    // Status Characteristic (Read + Notify)
    status_char = ble_service->createCharacteristic(
        STATUS_CHARACTERISTIC,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    status_char->addDescriptor(new BLE2902());

    // Command Characteristic (Write)
    command_char = ble_service->createCharacteristic(
        COMMAND_CHARACTERISTIC,
        BLECharacteristic::PROPERTY_WRITE
    );
    command_callbacks = new CommandCallbacks(this);
    command_char->setCallbacks(command_callbacks);

    // Config Characteristic (Read + Write)
    config_char = ble_service->createCharacteristic(
        CONFIG_CHARACTERISTIC,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
    );
}

void BLE_Server::handleCommand(uint8_t command, uint8_t* data, size_t length) {
    switch (command) {
        case BLE_CMD_CANCEL_ALERT:
            Serial.println("[BLE] Command: Cancel Alert");
            break;

        case BLE_CMD_TEST_ALERT:
            Serial.println("[BLE] Command: Test Alert");
            break;

        case BLE_CMD_GET_STATUS:
            Serial.println("[BLE] Command: Get Status");
            break;

        case BLE_CMD_SET_CONFIG:
            Serial.println("[BLE] Command: Set Config");
            break;

        case BLE_CMD_START_STREAMING:
            Serial.println("[BLE] Command: Start Streaming");
            enableStreaming(true);
            break;

        case BLE_CMD_STOP_STREAMING:
            Serial.println("[BLE] Command: Stop Streaming");
            enableStreaming(false);
            break;

        default:
            Serial.print("[BLE] Unknown command: 0x");
            Serial.println(command, HEX);
            break;
    }

    // Call external command callback if registered
    if (on_command_callback != nullptr) {
        on_command_callback(command, data, length);
    }
}

bool BLE_Server::notifyCharacteristic(BLECharacteristic* characteristic, uint8_t* data, size_t length) {
    if (characteristic == nullptr || !device_connected) {
        return false;
    }

    try {
        characteristic->setValue(data, length);
        characteristic->notify();
        return true;
    } catch (...) {
        Serial.println("[BLE] Error sending notification");
        return false;
    }
}

String BLE_Server::createEmergencyJSON(const EmergencyData_t& data) {
    DynamicJsonDocument doc(2048);

    doc["type"] = "emergency";
    doc["timestamp"] = data.timestamp;
    doc["confidence_score"] = data.confidence_score;
    doc["confidence_level"] = data.confidence;
    doc["battery_level"] = data.battery_level;
    doc["sos_triggered"] = data.sos_triggered;
    doc["device_id"] = String(data.device_id);

    String json_string;
    serializeJson(doc, json_string);
    return json_string;
}

String BLE_Server::createSensorDataJSON(const SensorData_t& data) {
    DynamicJsonDocument doc(512);

    doc["type"] = "sensor";
    doc["timestamp"] = data.timestamp;
    doc["accel_x"] = data.accel_x;
    doc["accel_y"] = data.accel_y;
    doc["accel_z"] = data.accel_z;
    doc["gyro_x"] = data.gyro_x;
    doc["gyro_y"] = data.gyro_y;
    doc["gyro_z"] = data.gyro_z;
    doc["heart_rate"] = data.heart_rate;
    doc["pressure"] = data.pressure;

    String json_string;
    serializeJson(doc, json_string);
    return json_string;
}

String BLE_Server::createStatusJSON(const SystemStatus_t& data) {
    DynamicJsonDocument doc(512);

    doc["type"] = "status";
    doc["sensors_initialized"] = data.sensors_initialized;
    doc["wifi_connected"] = data.wifi_connected;
    doc["bluetooth_connected"] = data.bluetooth_connected;
    doc["battery_percentage"] = data.battery_percentage;
    doc["current_status"] = data.current_status;
    doc["uptime_ms"] = data.uptime_ms;

    String json_string;
    serializeJson(doc, json_string);
    return json_string;
}

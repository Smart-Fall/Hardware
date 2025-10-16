#include "Emergency_Comms.h"

Emergency_Comms::Emergency_Comms(WiFi_Manager* wifi, BLE_Server* ble)
    : wifi_manager(wifi), ble_server(ble), wifi_enabled(true), ble_enabled(true),
      initialized(false), current_alert_status(ALERT_STATUS_PENDING),
      retry_count(0), max_retries(3), last_alert_time(0), retry_interval(5000),
      alert_pending(false) {
}

Emergency_Comms::~Emergency_Comms() {
    // Cleanup if needed
}

bool Emergency_Comms::begin() {
    if (initialized) {
        Serial.println("[Emergency] Already initialized");
        return true;
    }

    if (wifi_manager == nullptr && ble_server == nullptr) {
        Serial.println("[Emergency] ERROR: No communication modules provided!");
        return false;
    }

    Serial.println("[Emergency] Communication system initialized");
    initialized = true;

    return true;
}

void Emergency_Comms::setMaxRetries(uint8_t retries) {
    max_retries = retries;
}

void Emergency_Comms::setRetryInterval(uint32_t interval_ms) {
    retry_interval = interval_ms;
}

void Emergency_Comms::enableWiFi(bool enable) {
    wifi_enabled = enable;
    if (DEBUG_COMMUNICATION) {
        Serial.print("[Emergency] WiFi alerts: ");
        Serial.println(enable ? "enabled" : "disabled");
    }
}

void Emergency_Comms::enableBLE(bool enable) {
    ble_enabled = enable;
    if (DEBUG_COMMUNICATION) {
        Serial.print("[Emergency] BLE alerts: ");
        Serial.println(enable ? "enabled" : "disabled");
    }
}

bool Emergency_Comms::isWiFiEnabled() {
    return wifi_enabled;
}

bool Emergency_Comms::isBLEEnabled() {
    return ble_enabled;
}

bool Emergency_Comms::sendEmergencyAlert(const EmergencyData_t& emergency_data) {
    return sendEmergencyAlert(emergency_data, true);
}

bool Emergency_Comms::sendEmergencyAlert(const EmergencyData_t& emergency_data, bool urgent) {
    if (!initialized) {
        Serial.println("[Emergency] ERROR: Not initialized!");
        return false;
    }

    Serial.println("\n!!! SENDING EMERGENCY ALERT !!!");
    Serial.print("Confidence Score: ");
    Serial.print(emergency_data.confidence_score);
    Serial.println("/105");
    Serial.print("SOS Triggered: ");
    Serial.println(emergency_data.sos_triggered ? "YES" : "NO");

    bool wifi_success = false;
    bool ble_success = false;

    // Try WiFi transmission
    if (wifi_enabled && wifi_manager != nullptr) {
        Serial.println("[Emergency] Attempting WiFi transmission...");
        wifi_success = sendViaWiFi(emergency_data);

        if (wifi_success) {
            Serial.println("[Emergency] ✓ WiFi transmission successful");
        } else {
            Serial.println("[Emergency] ✗ WiFi transmission failed");
        }
    }

    // Try BLE transmission
    if (ble_enabled && ble_server != nullptr) {
        Serial.println("[Emergency] Attempting BLE transmission...");
        ble_success = sendViaBLE(emergency_data);

        if (ble_success) {
            Serial.println("[Emergency] ✓ BLE transmission successful");
        } else {
            Serial.println("[Emergency] ✗ BLE transmission failed");
        }
    }

    // Update status based on results
    if (wifi_success && ble_success) {
        current_alert_status = ALERT_STATUS_SENT_BOTH;
        retry_count = 0;
    } else if (wifi_success) {
        current_alert_status = ALERT_STATUS_SENT_WIFI;
        retry_count = 0;
    } else if (ble_success) {
        current_alert_status = ALERT_STATUS_SENT_BLE;
        retry_count = 0;
    } else {
        current_alert_status = ALERT_STATUS_FAILED;

        // Queue for retry if urgent
        if (urgent && retry_count < max_retries) {
            pending_alert = emergency_data;
            alert_pending = true;
            current_alert_status = ALERT_STATUS_RETRY;
            last_alert_time = millis();

            Serial.print("[Emergency] Queued for retry (");
            Serial.print(retry_count + 1);
            Serial.print("/");
            Serial.print(max_retries);
            Serial.println(")");
        }
    }

    updateAlertStatus();

    return (wifi_success || ble_success);
}

bool Emergency_Comms::sendStatusUpdate(const SystemStatus_t& status_data) {
    if (!initialized) return false;

    bool success = false;

    // Create StatusData_t from SystemStatus_t
    StatusData_t status_packet;
    status_packet.timestamp = millis();
    status_packet.battery_level = status_data.battery_percentage;
    status_packet.system_health = status_data.sensors_initialized;
    status_packet.uptime = status_data.uptime_ms;
    strncpy(status_packet.status_message, "Status update", sizeof(status_packet.status_message));

    if (wifi_enabled && wifi_manager != nullptr && wifi_manager->isConnected()) {
        success |= wifi_manager->sendStatusUpdate(status_packet);
    }

    if (ble_enabled && ble_server != nullptr && ble_server->isConnected()) {
        success |= ble_server->sendStatusUpdate(status_data);
    }

    return success;
}

bool Emergency_Comms::sendSensorData(const SensorData_t& sensor_data) {
    if (!initialized) return false;

    bool success = false;

    if (wifi_enabled && wifi_manager != nullptr && wifi_manager->isConnected()) {
        success |= wifi_manager->sendSensorData(sensor_data);
    }

    if (ble_enabled && ble_server != nullptr && ble_server->isStreaming()) {
        success |= ble_server->sendSensorData(sensor_data);
    }

    return success;
}

void Emergency_Comms::processAlertQueue() {
    if (!alert_pending || current_alert_status != ALERT_STATUS_RETRY) {
        return;
    }

    uint32_t current_time = millis();

    if (current_time - last_alert_time >= retry_interval) {
        retry_count++;

        Serial.print("[Emergency] Retry attempt ");
        Serial.print(retry_count);
        Serial.print("/");
        Serial.println(max_retries);

        if (retryFailedAlert()) {
            alert_pending = false;
            Serial.println("[Emergency] ✓ Retry successful!");
        } else if (retry_count >= max_retries) {
            alert_pending = false;
            current_alert_status = ALERT_STATUS_FAILED;
            Serial.println("[Emergency] ✗ Max retries reached, alert failed");
        } else {
            last_alert_time = current_time;
        }
    }
}

AlertStatus_t Emergency_Comms::getAlertStatus() {
    return current_alert_status;
}

bool Emergency_Comms::isAlertPending() {
    return alert_pending;
}

void Emergency_Comms::clearPendingAlert() {
    alert_pending = false;
    retry_count = 0;
    current_alert_status = ALERT_STATUS_PENDING;
}

bool Emergency_Comms::isConnected() {
    return isWiFiConnected() || isBLEConnected();
}

bool Emergency_Comms::isWiFiConnected() {
    return (wifi_manager != nullptr && wifi_manager->isConnected());
}

bool Emergency_Comms::isBLEConnected() {
    return (ble_server != nullptr && ble_server->isConnected());
}

void Emergency_Comms::printStatus() {
    Serial.println("=== Emergency Communication Status ===");

    if (wifi_manager != nullptr) {
        Serial.print("WiFi: ");
        Serial.print(wifi_enabled ? "Enabled" : "Disabled");
        Serial.print(" | ");
        Serial.println(wifi_manager->isConnected() ? "Connected" : "Disconnected");
    }

    if (ble_server != nullptr) {
        Serial.print("BLE: ");
        Serial.print(ble_enabled ? "Enabled" : "Disabled");
        Serial.print(" | ");
        Serial.println(ble_server->isConnected() ? "Connected" : "Advertising");
    }

    Serial.print("Alert Status: ");
    Serial.println(getAlertStatusString(current_alert_status));

    if (alert_pending) {
        Serial.print("Pending Alert - Retry ");
        Serial.print(retry_count);
        Serial.print("/");
        Serial.println(max_retries);
    }

    Serial.println("======================================");
}

bool Emergency_Comms::isInitialized() {
    return initialized;
}

uint8_t Emergency_Comms::getRetryCount() {
    return retry_count;
}

// Private helper functions

bool Emergency_Comms::sendViaWiFi(const EmergencyData_t& data) {
    if (wifi_manager == nullptr || !wifi_manager->isConnected()) {
        return false;
    }

    return wifi_manager->sendEmergencyAlert(data);
}

bool Emergency_Comms::sendViaBLE(const EmergencyData_t& data) {
    if (ble_server == nullptr || !ble_server->isConnected()) {
        return false;
    }

    return ble_server->sendEmergencyAlert(data);
}

bool Emergency_Comms::retryFailedAlert() {
    if (!alert_pending) {
        return false;
    }

    return sendEmergencyAlert(pending_alert, false);  // Non-urgent retry
}

void Emergency_Comms::updateAlertStatus() {
    if (DEBUG_COMMUNICATION) {
        Serial.print("[Emergency] Alert status: ");
        Serial.println(getAlertStatusString(current_alert_status));
    }
}

String Emergency_Comms::getAlertStatusString(AlertStatus_t status) {
    switch (status) {
        case ALERT_STATUS_PENDING:    return "Pending";
        case ALERT_STATUS_SENDING:    return "Sending";
        case ALERT_STATUS_SENT_WIFI:  return "Sent via WiFi";
        case ALERT_STATUS_SENT_BLE:   return "Sent via BLE";
        case ALERT_STATUS_SENT_BOTH:  return "Sent via Both";
        case ALERT_STATUS_FAILED:     return "Failed";
        case ALERT_STATUS_RETRY:      return "Retrying";
        default:                       return "Unknown";
    }
}

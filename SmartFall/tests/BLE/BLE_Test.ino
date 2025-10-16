/*
 * SmartFall - BLE (Bluetooth Low Energy) Module Test
 *
 * Tests BLE GATT server functionality, characteristic notifications,
 * and mobile app connectivity
 *
 * Hardware: ESP32 HUZZAH32 Feather
 *
 * This test verifies:
 * - BLE server initialization
 * - GATT service and characteristics creation
 * - Device advertising and discoverability
 * - Client connection/disconnection
 * - Characteristic notifications (Emergency, Sensor, Status)
 * - Command reception from mobile app
 * - Data streaming capabilities
 *
 * Connect using:
 * - iOS: LightBlue or nRF Connect app
 * - Android: nRF Connect app
 * - Look for device name: "SmartFall-Test"
 */

#include "BLE_Server.h"

#define BLE_DEVICE_NAME  "SmartFall-Test"

BLE_Server bleServer;

unsigned long lastNotification = 0;
unsigned long notificationInterval = 5000;  // Send notifications every 5 seconds
int notificationCounter = 0;

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n========================================");
    Serial.println("       SmartFall BLE Module Test");
    Serial.println("========================================\n");

    // Test 1: BLE Initialization
    Serial.println("TEST 1: BLE Initialization");
    Serial.println("---------------------------");
    if (bleServer.begin(BLE_DEVICE_NAME)) {
        Serial.println("✓ BLE server initialized successfully\n");

        // Print connection info
        bleServer.printConnectionInfo();
    } else {
        Serial.println("✗ BLE initialization failed!");
        Serial.println("\nTest cannot continue.\n");
        while (true) {
            delay(1000);
        }
    }

    Serial.println("========================================");
    Serial.println("  BLE Test Ready - Waiting for Client");
    Serial.println("========================================\n");
    Serial.println("Instructions:");
    Serial.println("1. Open BLE scanner app on your phone:");
    Serial.println("   - iOS: LightBlue or nRF Connect");
    Serial.println("   - Android: nRF Connect");
    Serial.println("2. Look for device: " + String(BLE_DEVICE_NAME));
    Serial.println("3. Connect to the device");
    Serial.println("4. Explore characteristics and enable notifications");
    Serial.println("5. Try sending commands via Command characteristic\n");
    Serial.println("Waiting for connection...\n");
}

void loop() {
    unsigned long currentTime = millis();

    // Check connection status
    static bool wasConnected = false;
    bool isConnected = bleServer.isConnected();

    // Detect connection state changes
    if (isConnected && !wasConnected) {
        Serial.println("\n========================================");
        Serial.println("   ✓ Mobile App Connected!");
        Serial.println("========================================\n");
        wasConnected = true;
        notificationCounter = 0;
    } else if (!isConnected && wasConnected) {
        Serial.println("\n========================================");
        Serial.println("   Mobile App Disconnected");
        Serial.println("========================================\n");
        Serial.println("Waiting for new connection...\n");
        wasConnected = false;
    }

    // Send periodic notifications when connected
    if (isConnected && (currentTime - lastNotification >= notificationInterval)) {
        lastNotification = currentTime;
        notificationCounter++;

        Serial.println("--- Sending Notification #" + String(notificationCounter) + " ---");

        // Test Emergency Characteristic
        String emergencyMsg = "{\"type\":\"test\",\"count\":" + String(notificationCounter) +
                              ",\"timestamp\":" + String(millis()) + "}";
        if (bleServer.sendEmergency(emergencyMsg)) {
            Serial.println("✓ Emergency notification sent");
        }

        delay(200);

        // Test Sensor Data Characteristic
        String sensorData = "{\"accel_x\":0.05,\"accel_y\":-0.02,\"accel_z\":0.98," +
                           String("\"gyro_x\":1.2,\"gyro_y\":-0.5,\"gyro_z\":0.3,") +
                           "\"temp\":24.5,\"count\":" + String(notificationCounter) + "}";
        if (bleServer.sendSensorData(sensorData)) {
            Serial.println("✓ Sensor data notification sent");
        }

        delay(200);

        // Test Status Characteristic
        String statusMsg = "{\"status\":\"monitoring\",\"uptime\":" + String(millis()) +
                          ",\"count\":" + String(notificationCounter) + "}";
        if (bleServer.sendStatus(statusMsg)) {
            Serial.println("✓ Status notification sent");
        }

        Serial.println("\nData sent to mobile app. Check your BLE scanner.");
        Serial.println("You should see these notifications in the app.\n");
    }

    // Display connection status periodically
    static unsigned long lastStatusPrint = 0;
    if (!isConnected && (currentTime - lastStatusPrint >= 10000)) {
        lastStatusPrint = currentTime;
        Serial.print("Still advertising... Uptime: ");
        Serial.print(millis() / 1000);
        Serial.println(" seconds");
    }

    delay(100);
}

/*
 * Testing Guide:
 *
 * 1. Upload this sketch to ESP32
 * 2. Open Serial Monitor (115200 baud)
 * 3. On your phone, open nRF Connect or LightBlue
 * 4. Scan for BLE devices
 * 5. Connect to "SmartFall-Test"
 *
 * You should see:
 * - Service: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
 * - 5 Characteristics:
 *   1. Emergency (Notify) - beb5483e-36e1-4688-b7f5-ea07361b26a8
 *   2. Sensor (Notify) - beb5483f-36e1-4688-b7f5-ea07361b26a8
 *   3. Status (Notify) - beb54840-36e1-4688-b7f5-ea07361b26a8
 *   4. Command (Write) - beb54841-36e1-4688-b7f5-ea07361b26a8
 *   5. Config (Read/Write) - beb54842-36e1-4688-b7f5-ea07361b26a8
 *
 * Testing Commands:
 * - Write 0x01 to Command char → Cancel Alert
 * - Write 0x02 to Command char → Test Alert
 * - Write 0x03 to Command char → Get Status
 * - Write 0x05 to Command char → Start Streaming
 * - Write 0x06 to Command char → Stop Streaming
 *
 * Enable notifications on Emergency, Sensor, and Status characteristics
 * to see periodic test data being sent every 5 seconds.
 */

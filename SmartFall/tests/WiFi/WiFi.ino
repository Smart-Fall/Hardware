/*
 * SmartFall - WiFi Module Test
 *
 * Tests WiFi connectivity, HTTP communication, and auto-reconnect features
 *
 * Hardware: ESP32 Feather V2
 *
 * This test verifies:
 * - WiFi connection to configured network
 * - Signal strength (RSSI) monitoring
 * - HTTP GET/POST requests
 * - JSON payload transmission
 * - Auto-reconnect functionality
 * - Connection stability
 */

#include "WiFi_Manager.h"
#include <HTTPClient.h>

// WiFi Configuration - CHANGE THESE!
#define WIFI_SSID "Mohammed network"
#define WIFI_PASSWORD "87654321"
#define SENSOR_STREAM_URL "https://smartfall.vercel.app/api/device/sensor-stream" // Sensor data endpoint
#define SERVER_URL "https://smartfall.vercel.app/api/health" // Production backend

WiFi_Manager wifiManager;
String deviceId; // Will be set to MAC address

unsigned long lastTest = 0;
unsigned long testInterval = 5000; // Send sensor data every 5 seconds
unsigned long lastSensorTest = 0;
int testCounter = 0;

// Mock sensor values for testing
float mockAccelX = 0.0;
float mockAccelY = 0.0;
float mockAccelZ = 9.81; // Simulating gravity
float mockGyroX = 0.0;
float mockGyroY = 0.0;
float mockGyroZ = 0.0;
float mockPressure = 1013.25;
float mockFSR = 0.0; // Force Sensing Resistor (0-1023)
float mockBatteryLevel = 100.0; // Start at 100%
unsigned long startTime = 0;

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n========================================");
    Serial.println("      SmartFall WiFi Module Test");
    Serial.println("========================================\n");

    // Test 1: Scan Available WiFi Networks (with timeout for target SSID)
    Serial.println("TEST 1: Scanning Available WiFi Networks");
    Serial.println("-----------------------------------------");
    Serial.print("Looking for SSID: ");
    Serial.println(WIFI_SSID);
    Serial.println("Scanning continuously for 30 seconds...\n");

    unsigned long scanStartTime = millis();
    unsigned long scanTimeout = 30000; // 30 seconds
    bool ssidFound = false;
    int scanCount = 0;

    while (millis() - scanStartTime < scanTimeout && !ssidFound)
    {
        scanCount++;
        Serial.print("Scan #");
        Serial.println(scanCount);

        int numNetworks = WiFi.scanNetworks();

        if (numNetworks == 0)
        {
            Serial.println("  No networks found\n");
        }
        else
        {
            Serial.print("  Found ");
            Serial.print(numNetworks);
            Serial.println(" network(s):");

            for (int i = 0; i < numNetworks; i++)
            {
                String currentSSID = WiFi.SSID(i);
                Serial.print("  ");
                Serial.print(i + 1);
                Serial.print(". ");
                Serial.print(currentSSID);
                Serial.print(" (");
                Serial.print(WiFi.RSSI(i));
                Serial.print(" dBm) ");
                Serial.print(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "Open" : "Encrypted");
                Serial.println();

                // Check if this is our target SSID
                if (currentSSID == WIFI_SSID)
                {
                    ssidFound = true;
                }
            }
            Serial.println();
        }

        if (!ssidFound)
        {
            delay(1000); // Wait 1 second before next scan
        }
    }

    if (!ssidFound)
    {
        Serial.println("✗ Target SSID not found after 30 seconds!");
        Serial.println("\nPlease check:");
        Serial.println("1. WiFi SSID is correct: " + String(WIFI_SSID));
        Serial.println("2. WiFi network is powered on and in range");
        Serial.println("3. ESP32 antenna is properly connected");
        Serial.println("\nTest cannot continue without finding target SSID.\n");
        while (true)
        {
            delay(1000);
        }
    }

    Serial.println("✓ Target SSID found! Proceeding with connection...\n");

    // Test 2: WiFi Connection
    Serial.println("TEST 2: WiFi Connection");
    Serial.println("------------------------");
    if (wifiManager.begin(WIFI_SSID, WIFI_PASSWORD))
    {
        Serial.println("✓ WiFi connection successful\n");

        // Get MAC address as device ID
        deviceId = WiFi.macAddress();
        Serial.print("Device ID (MAC): ");
        Serial.println(deviceId);
        Serial.println();

        // Set server URL
        wifiManager.setServerURL(SERVER_URL);

        // Enable auto-reconnect
        wifiManager.enableAutoReconnect(true);

        // Print detailed connection info
        wifiManager.printConnectionInfo();
    }
    else
    {
        Serial.println("✗ WiFi connection failed!");
        Serial.println("\nPlease check:");
        Serial.println("1. WiFi SSID and password are correct");
        Serial.println("2. WiFi network is available");
        Serial.println("3. ESP32 antenna is properly connected");
        Serial.println("\nTest cannot continue without WiFi.\n");
        while (true)
        {
            delay(1000);
        }
    }

    // Test 2: Signal Strength
    Serial.println("TEST 2: Signal Strength");
    Serial.println("------------------------");
    int rssi = wifiManager.getRSSI();
    Serial.print("RSSI: ");
    Serial.print(rssi);
    Serial.println(" dBm");

    if (rssi > -50)
    {
        Serial.println("✓ Excellent signal strength");
    }
    else if (rssi > -60)
    {
        Serial.println("✓ Good signal strength");
    }
    else if (rssi > -70)
    {
        Serial.println("⚠ Fair signal strength");
    }
    else
    {
        Serial.println("⚠ Weak signal strength - may affect reliability");
    }
    Serial.println();

    // Test 4: HTTP POST (Plain Text)
    Serial.println("TEST 4: HTTP POST (Plain Text)");
    Serial.println("--------------------------------");
    String testMessage = "SmartFall WiFi Test - Plain Text Message";
    if (wifiManager.sendTestMessage(testMessage))
    {
        Serial.println("✓ Plain text HTTP POST successful\n");
    }
    else
    {
        Serial.println("✗ Plain text HTTP POST failed\n");
    }

    delay(2000);

    // Test 5: HTTP POST (JSON)
    Serial.println("TEST 5: HTTP POST (JSON)");
    Serial.println("-------------------------");
    String jsonPayload = "{\"device\":\"SmartFall\",\"device_id\":\"" + deviceId + "\",\"test\":\"WiFi Module\",\"timestamp\":" + String(millis()) + "}";
    if (wifiManager.sendJSON(jsonPayload))
    {
        Serial.println("✓ JSON HTTP POST successful\n");
    }
    else
    {
        Serial.println("✗ JSON HTTP POST failed\n");
    }

    // Record start time for battery simulation
    startTime = millis();

    Serial.println("========================================");
    Serial.println("    WiFi Test Complete - Monitoring");
    Serial.println("========================================\n");
    Serial.println("Now monitoring connection and sending periodic sensor data with battery info...\n");
}

void loop()
{
    unsigned long currentTime = millis();

    // Check WiFi connection (auto-reconnect if enabled)
    wifiManager.checkConnection();

    // Send periodic sensor data
    if (currentTime - lastTest >= testInterval)
    {
        lastTest = currentTime;
        testCounter++;

        Serial.println("--- Sending Sensor Data #" + String(testCounter) + " ---");

        // Check if still connected
        if (wifiManager.isConnected())
        {
            Serial.println("✓ WiFi connected");
            Serial.print("IP: ");
            Serial.println(wifiManager.getLocalIP());

            // Update mock sensor values with slight variations to simulate real data
            mockAccelX = 0.5 * sin(currentTime / 1000.0);
            mockAccelY = 0.5 * cos(currentTime / 1000.0);
            mockAccelZ = 9.81 + 0.2 * sin(currentTime / 2000.0);
            mockGyroX = 5.0 * sin(currentTime / 1500.0);
            mockGyroY = 5.0 * cos(currentTime / 1500.0);
            mockGyroZ = 0.0;
            mockPressure = 1013.25 + random(-5, 5) / 10.0;

            // Simulate FSR (Force Sensing Resistor): occasional pressure spikes to simulate presses
            // Base at ~50 with occasional jumps to 300+ (simulating foot pressure)
            if (random(0, 10) > 7) {
              mockFSR = 200 + random(0, 200); // Simulate pressure press (200-400)
            } else {
              mockFSR = 50 + random(-10, 10); // Normal baseline
            }
            mockFSR = max(0.0f, min(1023.0f, mockFSR)); // Clamp to 0-1023

            // Simulate battery discharge: 0.1% per minute (100% over ~1000 minutes)
            unsigned long elapsedMinutes = (currentTime - startTime) / 60000;
            mockBatteryLevel = max(5.0, 100.0 - (elapsedMinutes * 0.1));
            mockBatteryLevel += random(-1, 2) / 10.0; // Add noise

            // Send sensor data to sensor-stream endpoint
            String sensorPayload = "{\"device_id\":\"" + deviceId +
                                   "\",\"accel_x\":" + String(mockAccelX, 3) +
                                   ",\"accel_y\":" + String(mockAccelY, 3) +
                                   ",\"accel_z\":" + String(mockAccelZ, 3) +
                                   ",\"gyro_x\":" + String(mockGyroX, 3) +
                                   ",\"gyro_y\":" + String(mockGyroY, 3) +
                                   ",\"gyro_z\":" + String(mockGyroZ, 3) +
                                   ",\"pressure\":" + String(mockPressure, 2) +
                                   ",\"fsr\":" + String((int)mockFSR) +
                                   ",\"battery_level\":" + String(mockBatteryLevel, 1) +
                                   ",\"wifi_connected\":true" +
                                   ",\"sensors_initialized\":true" +
                                   ",\"uptime_ms\":" + String(currentTime) + "}";

            Serial.println("Sending sensor data:");
            Serial.print("  Battery: ");
            Serial.print(mockBatteryLevel);
            Serial.println("%");
            Serial.print("  Accel: X=");
            Serial.print(mockAccelX);
            Serial.print(", Y=");
            Serial.print(mockAccelY);
            Serial.print(", Z=");
            Serial.println(mockAccelZ);
            Serial.print("  FSR: ");
            Serial.println(mockFSR);

            // Send using HTTPClient directly to sensor-stream endpoint
            if (sendSensorData(sensorPayload))
            {
                Serial.println("✓ Sensor data sent successfully");
            }
            else
            {
                Serial.println("✗ Failed to send sensor data");
            }
        }
        else
        {
            Serial.println("✗ WiFi disconnected - waiting for reconnect...");
        }

        Serial.println();
    }

    delay(100);
}

// Helper function to send sensor data via HTTP POST
bool sendSensorData(String payload)
{
    HTTPClient http;
    http.setConnectTimeout(5000);

    if (http.begin(SENSOR_STREAM_URL))
    {
        http.addHeader("Content-Type", "application/json");
        int httpCode = http.POST(payload);

        if (httpCode == HTTP_CODE_OK)
        {
            String response = http.getString();
            http.end();
            return true;
        }
        else
        {
            Serial.print("HTTP Error: ");
            Serial.println(httpCode);
            http.end();
            return false;
        }
    }
    else
    {
        Serial.println("Failed to connect to HTTP server");
        return false;
    }
}

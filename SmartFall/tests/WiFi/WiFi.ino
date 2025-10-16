/*
 * SmartFall - WiFi Module Test
 *
 * Tests WiFi connectivity, HTTP communication, and auto-reconnect features
 *
 * Hardware: ESP32 HUZZAH32 Feather
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

// WiFi Configuration - CHANGE THESE!
#define WIFI_SSID        "Your_WiFi_SSID"
#define WIFI_PASSWORD    "Your_WiFi_Password"
#define SERVER_URL       "http://httpbin.org/post"  // Test server

WiFi_Manager wifiManager;

unsigned long lastTest = 0;
unsigned long testInterval = 10000;  // Test every 10 seconds
int testCounter = 0;

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n========================================");
    Serial.println("      SmartFall WiFi Module Test");
    Serial.println("========================================\n");

    // Test 1: WiFi Connection
    Serial.println("TEST 1: WiFi Connection");
    Serial.println("------------------------");
    if (wifiManager.begin(WIFI_SSID, WIFI_PASSWORD)) {
        Serial.println("✓ WiFi connection successful\n");

        // Set server URL
        wifiManager.setServerURL(SERVER_URL);

        // Enable auto-reconnect
        wifiManager.enableAutoReconnect(true);

        // Print detailed connection info
        wifiManager.printConnectionInfo();
    } else {
        Serial.println("✗ WiFi connection failed!");
        Serial.println("\nPlease check:");
        Serial.println("1. WiFi SSID and password are correct");
        Serial.println("2. WiFi network is available");
        Serial.println("3. ESP32 antenna is properly connected");
        Serial.println("\nTest cannot continue without WiFi.\n");
        while (true) {
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

    if (rssi > -50) {
        Serial.println("✓ Excellent signal strength");
    } else if (rssi > -60) {
        Serial.println("✓ Good signal strength");
    } else if (rssi > -70) {
        Serial.println("⚠ Fair signal strength");
    } else {
        Serial.println("⚠ Weak signal strength - may affect reliability");
    }
    Serial.println();

    // Test 3: HTTP POST (Plain Text)
    Serial.println("TEST 3: HTTP POST (Plain Text)");
    Serial.println("--------------------------------");
    String testMessage = "SmartFall WiFi Test - Plain Text Message";
    if (wifiManager.sendTestMessage(testMessage)) {
        Serial.println("✓ Plain text HTTP POST successful\n");
    } else {
        Serial.println("✗ Plain text HTTP POST failed\n");
    }

    delay(2000);

    // Test 4: HTTP POST (JSON)
    Serial.println("TEST 4: HTTP POST (JSON)");
    Serial.println("-------------------------");
    String jsonPayload = "{\"device\":\"SmartFall\",\"test\":\"WiFi Module\",\"timestamp\":" + String(millis()) + "}";
    if (wifiManager.sendJSON(jsonPayload)) {
        Serial.println("✓ JSON HTTP POST successful\n");
    } else {
        Serial.println("✗ JSON HTTP POST failed\n");
    }

    Serial.println("========================================");
    Serial.println("    WiFi Test Complete - Monitoring");
    Serial.println("========================================\n");
    Serial.println("Now monitoring connection and sending periodic test messages...\n");
}

void loop() {
    unsigned long currentTime = millis();

    // Check WiFi connection (auto-reconnect if enabled)
    wifiManager.checkConnection();

    // Send periodic test messages
    if (currentTime - lastTest >= testInterval) {
        lastTest = currentTime;
        testCounter++;

        Serial.println("--- Periodic Test #" + String(testCounter) + " ---");

        // Check if still connected
        if (wifiManager.isConnected()) {
            Serial.println("✓ WiFi connected");
            Serial.print("IP: ");
            Serial.println(wifiManager.getLocalIP());
            Serial.print("RSSI: ");
            Serial.print(wifiManager.getRSSI());
            Serial.println(" dBm");

            // Send test JSON payload
            String jsonTest = "{\"device\":\"SmartFall\",\"test_number\":" + String(testCounter) +
                              ",\"uptime_ms\":" + String(millis()) +
                              ",\"rssi\":" + String(wifiManager.getRSSI()) + "}";

            if (wifiManager.sendJSON(jsonTest)) {
                Serial.println("✓ Test message sent successfully");
            } else {
                Serial.println("✗ Failed to send test message");
            }
        } else {
            Serial.println("✗ WiFi disconnected - waiting for reconnect...");
        }

        Serial.println();
    }

    delay(100);
}

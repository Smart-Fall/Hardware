#include <esp_mac.h>

#include <Wire.h>
#include "Board_Config.h"
#include "MPU6050_Sensor.h"
#include "BMP280_Sensor.h"
#include "FSR_Sensor.h"
#include "Fall_Detector.h"
#include "Audio_Manager.h"
#include "Data_Types.h"
#include "Fall_Store.h"
#include "WiFi_Manager.h"
#include "WiFi_Credentials.h"
#include "Config.h"

MPU6050_Sensor imuSensor;
BMP280_Sensor pressureSensor;
FSR_Sensor fsr4(FSR_ANALOG_PIN);
FallDetector fallDetector;
Audio_Manager audioManager(SPEAKER_PIN);
Fall_Store fallStore;
WiFi_Manager wifiManager;

char deviceID[32];
char wifiSSID[32];
char wifiPassword[64];
SensorData_t currentSensorData = {};
EmergencyData_t fallWorkBuffer = {};

struct SensorSnapshot
{
    float accel_x = 0.0f;
    float accel_y = 0.0f;
    float accel_z = 1.0f;
    float gyro_x = 0.0f;
    float gyro_y = 0.0f;
    float gyro_z = 0.0f;
    float imu_temp = 0.0f;
    float pressure = 1013.25f;
    float bmp_temp = 0.0f;
    float altitude = 0.0f;
    uint16_t fsr_raw = 0;
    bool imu_ok = false;
    bool bmp_ok = false;
    bool fsr_ok = false;
};

SensorSnapshot snapshot;

uint32_t lastImuRead = 0;
uint32_t lastBarometerRead = 0;
uint32_t lastHeartbeat = 0;
FallStatus_t lastFallStatus = FALL_STATUS_MONITORING;
bool alertLatched = false;
bool wifiLoadedFromNVS = false;
bool fallStoreReady = false;

static void generateDeviceID()
{
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    snprintf(deviceID, sizeof(deviceID), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void printBanner()
{
    Serial.println();
    Serial.println("========================================");
    Serial.println(" SmartFall Sensor + Audio Firmware");
    Serial.println("========================================");
    Serial.print("Device ID: ");
    Serial.println(deviceID);
    Serial.println("Commands: S=sensors, H=help, R=reset detector, B=beep test");
    Serial.println("          I=WiFi info, C=connect WiFi, D=disconnect WiFi");
    Serial.println("          G=HTTP health GET, P=HTTP sensor POST");
    Serial.println("          T=HTTP status POST, M=HTTP commands GET");
    Serial.println("          F=fall POST, U=retry queued falls");
    Serial.println("WiFi reconnect and HTTP requests are manual-only.");
    Serial.println();
}

static void printHelp()
{
    Serial.println("Commands:");
    Serial.println("  S - Print full sensor snapshot");
    Serial.println("  H - Show this help");
    Serial.println("  R - Reset fall detector state");
    Serial.println("  B - Play audio test pattern");
    Serial.println("  I - Show WiFi status");
    Serial.println("  C - Connect WiFi using stored credentials");
    Serial.println("  D - Disconnect WiFi");
    Serial.println("  G - Run manual GET /api/health");
    Serial.println("  P - Run manual POST /api/device/sensor-stream");
    Serial.println("  T - Run manual POST /api/device/status");
    Serial.println("  M - Run manual GET /api/device/commands and apply mute");
    Serial.println("  F - Run manual POST /api/falls using current sensor snapshot");
    Serial.println("  U - Retry queued fall alerts from flash");
    Serial.println("JSON provisioning:");
    Serial.println("  {\"identify\":true} or {\"get_status\":true}");
    Serial.println("  {\"set_wifi\":true,\"ssid\":\"YOUR_SSID\",\"password\":\"YOUR_PASS\"}");
}

static void printWiFiStatus()
{
    Serial.println("--- WiFi Status ---");
    Serial.print("Credential source: ");
    Serial.println(wifiLoadedFromNVS ? "NVS" : "Config.h fallback");
    Serial.print("Configured SSID: ");
    Serial.println(wifiSSID);
    Serial.print("Connected: ");
    Serial.println(wifiManager.isConnected() ? "YES" : "NO");
    Serial.print("Queued fall alerts: ");
    Serial.println(fallStoreReady ? fallStore.pendingCount() : 0);
    if (wifiManager.isConnected())
    {
        wifiManager.printConnectionInfo();
    }
    Serial.println();
}

static void printDetectorStatus()
{
    FallStatus_t status = fallDetector.getCurrentStatus();
    Serial.print("Fall Detector: ");
    Serial.println(fallDetector.getStatusString(status));
}

static bool sensorsReadyForHttpPayload()
{
    return snapshot.imu_ok && snapshot.bmp_ok && snapshot.fsr_ok;
}

static const char *currentStatusString()
{
    return fallDetector.getStatusString(fallDetector.getCurrentStatus());
}

static const char *confidenceLevelString(FallConfidence_t confidence)
{
    switch (confidence)
    {
    case CONFIDENCE_HIGH:
        return "HIGH";
    case CONFIDENCE_CONFIRMED:
        return "CONFIRMED";
    case CONFIDENCE_POTENTIAL:
        return "POTENTIAL";
    case CONFIDENCE_SUSPICIOUS:
        return "SUSPICIOUS";
    case CONFIDENCE_NO_FALL:
    default:
        return "NO_FALL";
    }
}

static void populateEmergencyData(EmergencyData_t &data,
                                  FallConfidence_t confidence,
                                  uint8_t confidenceScore,
                                  bool sosTriggered)
{
    data = {};
    data.timestamp = millis();
    data.confidence = confidence;
    data.confidence_score = confidenceScore;
    data.battery_level = 0.0f;
    data.sos_triggered = sosTriggered;
    strncpy(data.device_id, deviceID, sizeof(data.device_id) - 1);

    SensorData_t &sensor = data.sensor_history[0];
    sensor.timestamp = millis();
    sensor.valid = currentSensorData.valid;
    sensor.accel_x = snapshot.accel_x;
    sensor.accel_y = snapshot.accel_y;
    sensor.accel_z = snapshot.accel_z;
    sensor.gyro_x = snapshot.gyro_x;
    sensor.gyro_y = snapshot.gyro_y;
    sensor.gyro_z = snapshot.gyro_z;
    sensor.pressure = snapshot.pressure;
    sensor.fsr_value = snapshot.fsr_raw;
    sensor.fsr_values[3] = snapshot.fsr_raw;
}

static bool sendFallAlert(const EmergencyData_t &data)
{
    if (!wifiManager.isConnected())
    {
        Serial.println("[Fall] Skipped send: WiFi is not connected");
        return false;
    }

    const SensorData_t &sensor = data.sensor_history[0];
    String json = "{";
    json += "\"device_id\":\"" + String(data.device_id) + "\",";
    json += "\"confidence_score\":" + String(data.confidence_score);
    json += ",\"confidence_level\":\"" + String(confidenceLevelString(data.confidence)) + "\"";
    json += ",\"sos_triggered\":" + String(data.sos_triggered ? "true" : "false");
    json += ",\"battery_level\":" + String(data.battery_level, 1);
    json += ",\"sensor_data\":{";
    json += "\"accel_x\":" + String(sensor.accel_x, 3);
    json += ",\"accel_y\":" + String(sensor.accel_y, 3);
    json += ",\"accel_z\":" + String(sensor.accel_z, 3);
    json += ",\"gyro_x\":" + String(sensor.gyro_x, 3);
    json += ",\"gyro_y\":" + String(sensor.gyro_y, 3);
    json += ",\"gyro_z\":" + String(sensor.gyro_z, 3);
    json += ",\"pressure\":" + String(sensor.pressure, 2);
    json += "}}";

    return wifiManager.sendJSONToEndpoint("/api/falls", json);
}

static void queueAndTransmitFall(const EmergencyData_t &data, const char *source)
{
    bool saved = false;
    if (fallStoreReady)
    {
        saved = fallStore.save(data);
    }

    Serial.print("[Fall] Transmitting ");
    Serial.println(source);

    if (sendFallAlert(data))
    {
        Serial.println("[Fall] Alert sent successfully");
        if (saved)
        {
            fallStore.remove(data.timestamp);
        }
    }
    else
    {
        Serial.println(saved ? "[Fall] Alert queued for later retry" : "[Fall] Alert send failed");
    }
}

static void runManualFallPost()
{
    populateEmergencyData(fallWorkBuffer, CONFIDENCE_HIGH, 100, false);
    queueAndTransmitFall(fallWorkBuffer, "manual fall alert");
}

static void retryQueuedFalls()
{
    Serial.println("[Fall] Manual retry of queued alerts");

    if (!fallStoreReady)
    {
        Serial.println("[Fall] Retry skipped: Fall store is not ready");
        return;
    }

    if (!wifiManager.isConnected())
    {
        Serial.println("[Fall] Retry skipped: WiFi is not connected");
        return;
    }

    uint8_t retried = 0;
    while (fallStore.loadPending(&fallWorkBuffer, 1) == 1)
    {
        Serial.print("[Fall] Retrying queued alert ts=");
        Serial.println((unsigned long)fallWorkBuffer.timestamp);
        if (sendFallAlert(fallWorkBuffer))
        {
            fallStore.remove(fallWorkBuffer.timestamp);
            retried++;
            continue;
        }

        Serial.println("[Fall] Retry stopped after send failure");
        return;
    }

    if (retried == 0)
    {
        Serial.println("[Fall] No queued fall alerts");
    }
}

static bool parseMuteValue(const String &response, bool &mute)
{
    int muteIdx = response.indexOf("\"mute\"");
    if (muteIdx < 0)
    {
        return false;
    }

    int colonIdx = response.indexOf(':', muteIdx);
    if (colonIdx < 0)
    {
        return false;
    }

    int valueStart = colonIdx + 1;
    while (valueStart < (int)response.length() &&
           (response[valueStart] == ' ' || response[valueStart] == '\t'))
    {
        valueStart++;
    }

    if (response.substring(valueStart, valueStart + 4) == "true")
    {
        mute = true;
        return true;
    }

    if (response.substring(valueStart, valueStart + 5) == "false")
    {
        mute = false;
        return true;
    }

    return false;
}

static void runManualHealthCheck()
{
    Serial.println("[HTTP] Manual GET /api/health");

    if (!wifiManager.isConnected())
    {
        Serial.println("[HTTP] Skipped: WiFi is not connected");
        return;
    }

    String response = wifiManager.getFromEndpoint("/api/health");
    if (response.length() > 0)
    {
        Serial.println("[HTTP] Health check OK");
        Serial.print("[HTTP] Response: ");
        Serial.println(response);
    }
    else
    {
        Serial.println("[HTTP] Health check failed or returned an empty body");
    }
}

static void runManualSensorPost()
{
    Serial.println("[HTTP] Manual POST /api/device/sensor-stream");

    if (!wifiManager.isConnected())
    {
        Serial.println("[HTTP] Skipped: WiFi is not connected");
        return;
    }

    String json = "{";
    json += "\"device_id\":\"" + String(deviceID) + "\",";
    json += "\"accel_x\":" + String(snapshot.accel_x, 3);
    json += ",\"accel_y\":" + String(snapshot.accel_y, 3);
    json += ",\"accel_z\":" + String(snapshot.accel_z, 3);
    json += ",\"gyro_x\":" + String(snapshot.gyro_x, 3);
    json += ",\"gyro_y\":" + String(snapshot.gyro_y, 3);
    json += ",\"gyro_z\":" + String(snapshot.gyro_z, 3);
    json += ",\"pressure\":" + String(snapshot.pressure, 2);
    json += ",\"fsr\":" + String(snapshot.fsr_raw);
    json += ",\"heart_rate\":0";
    json += ",\"spo2\":0";
    json += ",\"sensors_initialized\":" + String(sensorsReadyForHttpPayload() ? "true" : "false");
    json += "}";

    bool ok = wifiManager.sendJSONToEndpoint("/api/device/sensor-stream", json);
    Serial.println(ok ? "[HTTP] Sensor POST succeeded" : "[HTTP] Sensor POST failed");
}

static void runManualStatusPost()
{
    Serial.println("[HTTP] Manual POST /api/device/status");

    if (!wifiManager.isConnected())
    {
        Serial.println("[HTTP] Skipped: WiFi is not connected");
        return;
    }

    String json = "{";
    json += "\"device_id\":\"" + String(deviceID) + "\",";
    json += "\"battery_level\":0,";
    json += "\"wifi_connected\":" + String(wifiManager.isConnected() ? "true" : "false");
    json += ",\"bluetooth_connected\":false";
    json += ",\"sensors_initialized\":" + String(sensorsReadyForHttpPayload() ? "true" : "false");
    json += ",\"uptime\":" + String(millis());
    json += ",\"current_status\":\"" + String(currentStatusString()) + "\"";
    json += "}";

    bool ok = wifiManager.sendJSONToEndpoint("/api/device/status", json);
    Serial.println(ok ? "[HTTP] Status POST succeeded" : "[HTTP] Status POST failed");
}

static void runManualCommandFetch()
{
    Serial.println("[HTTP] Manual GET /api/device/commands");

    if (!wifiManager.isConnected())
    {
        Serial.println("[HTTP] Skipped: WiFi is not connected");
        return;
    }

    String path = String("/api/device/commands?device_id=") + deviceID;
    String response = wifiManager.getFromEndpoint(path);
    if (response.length() == 0)
    {
        Serial.println("[HTTP] Command fetch failed or returned an empty body");
        return;
    }

    Serial.print("[HTTP] Response: ");
    Serial.println(response);

    bool mute = false;
    if (!parseMuteValue(response, mute))
    {
        Serial.println("[HTTP] Could not parse mute field");
        return;
    }

    audioManager.setMute(mute);
    Serial.print("[HTTP] Audio mute set to: ");
    Serial.println(mute ? "true" : "false");
}

static void initializeFallStore()
{
    Serial.println("--- Initializing Fall Store ---");
    fallStoreReady = fallStore.begin();
    if (!fallStoreReady)
    {
        Serial.println("ERROR: Fall store initialization failed");
    }
}

static bool parseJSONStringField(const char *line,
                                 const char *fieldName,
                                 char *output,
                                 size_t outputSize)
{
    const char *field = strstr(line, fieldName);
    if (!field)
    {
        return false;
    }

    const char *colon = strchr(field, ':');
    if (!colon)
    {
        return false;
    }

    const char *valueStart = strchr(colon, '"');
    if (!valueStart)
    {
        return false;
    }

    valueStart++;
    const char *valueEnd = strchr(valueStart, '"');
    if (!valueEnd)
    {
        return false;
    }

    size_t valueLength = valueEnd - valueStart;
    if (valueLength >= outputSize)
    {
        return false;
    }

    strncpy(output, valueStart, valueLength);
    output[valueLength] = '\0';
    return true;
}

static void handleSerialLine(const char *line)
{
    if (line[0] != '{')
    {
        return;
    }

    if (strstr(line, "\"identify\"") || strstr(line, "\"get_status\""))
    {
        String savedSSID = WiFi_Credentials::getSavedSSID();
        char response[192];
        snprintf(response, sizeof(response),
                 "{\"device_id\":\"%s\",\"ssid\":\"%s\",\"wifi_connected\":%s,\"firmware\":\"1.0\"}",
                 deviceID,
                 savedSSID.c_str(),
                 wifiManager.isConnected() ? "true" : "false");
        Serial.println(response);
        return;
    }

    if (strstr(line, "\"set_wifi\""))
    {
        char newSSID[sizeof(wifiSSID)] = "";
        char newPassword[sizeof(wifiPassword)] = "";

        if (!parseJSONStringField(line, "\"ssid\"", newSSID, sizeof(newSSID)) || strlen(newSSID) == 0)
        {
            Serial.println("{\"status\":\"error\",\"message\":\"Missing ssid\"}");
            return;
        }

        parseJSONStringField(line, "\"password\"", newPassword, sizeof(newPassword));

        if (WiFi_Credentials::save(newSSID, newPassword))
        {
            Serial.println("{\"status\":\"ok\",\"message\":\"Credentials saved. Rebooting...\"}");
            Serial.flush();
            delay(1000);
            ESP.restart();
        }
        else
        {
            Serial.println("{\"status\":\"error\",\"message\":\"NVS write failed\"}");
        }
        return;
    }

    Serial.println("{\"status\":\"error\",\"message\":\"Unknown action\"}");
}

static void printSensorSnapshot()
{
    Serial.println("--- Sensor Snapshot ---");
    Serial.print("IMU: ");
    Serial.println(snapshot.imu_ok ? "OK" : "ERROR");
    Serial.print("Accel (g): ");
    Serial.print(snapshot.accel_x, 3);
    Serial.print(", ");
    Serial.print(snapshot.accel_y, 3);
    Serial.print(", ");
    Serial.println(snapshot.accel_z, 3);
    Serial.print("Gyro (dps): ");
    Serial.print(snapshot.gyro_x, 2);
    Serial.print(", ");
    Serial.print(snapshot.gyro_y, 2);
    Serial.print(", ");
    Serial.println(snapshot.gyro_z, 2);
    Serial.print("IMU Temp (C): ");
    Serial.println(snapshot.imu_temp, 2);
    Serial.print("BMP280: ");
    Serial.println(snapshot.bmp_ok ? "OK" : "ERROR");
    Serial.print("Pressure (hPa): ");
    Serial.println(snapshot.pressure, 2);
    Serial.print("BMP Temp (C): ");
    Serial.println(snapshot.bmp_temp, 2);
    Serial.print("Altitude (m): ");
    Serial.println(snapshot.altitude, 2);
    Serial.print("FSR raw: ");
    Serial.println(snapshot.fsr_raw);
    Serial.print("FSR baseline: ");
    Serial.println(fsr4.getBaseline());
    printDetectorStatus();
    Serial.println();
}

static void printHeartbeat(uint32_t now)
{
    FallStatus_t status = fallDetector.getCurrentStatus();
    float accel_mag = sqrtf(snapshot.accel_x * snapshot.accel_x +
                            snapshot.accel_y * snapshot.accel_y +
                            snapshot.accel_z * snapshot.accel_z);

    Serial.print("[Alive] t=");
    Serial.print(now / 1000UL);
    Serial.print("s | IMU=");
    Serial.print(snapshot.imu_ok ? "OK" : "ERR");
    Serial.print(" ");
    Serial.print(accel_mag, 3);
    Serial.print("g | BMP=");
    Serial.print(snapshot.bmp_ok ? "OK" : "ERR");
    Serial.print(" ");
    Serial.print(snapshot.pressure, 1);
    Serial.print("hPa | FSR=");
    Serial.print(snapshot.fsr_raw);
    Serial.print(" | FALL=");
    Serial.print(fallDetector.getStatusString(status));
    Serial.print(" | WIFI=");
    Serial.println(wifiManager.isConnected() ? "ON" : "OFF");
}

static void handleSingleCharCommand(char command)
{
    if (command == 's' || command == 'S')
    {
        printSensorSnapshot();
    }
    else if (command == 'h' || command == 'H')
    {
        printHelp();
    }
    else if (command == 'r' || command == 'R')
    {
        fallDetector.resetDetection();
        alertLatched = false;
        lastFallStatus = fallDetector.getCurrentStatus();
        Serial.println("[Detector] Reset complete");
    }
    else if (command == 'b' || command == 'B')
    {
        if (audioManager.isInitialized())
        {
            Serial.println("[Audio] Playing test pattern");
            audioManager.playPattern(ALERT_PATTERN_DOUBLE_BEEP, 1);
        }
        else
        {
            Serial.println("[Audio] Not initialized");
        }
    }
    else if (command == 'i' || command == 'I')
    {
        printWiFiStatus();
    }
    else if (command == 'c' || command == 'C')
    {
        Serial.println("[WiFi] Manual connect requested");
        if (wifiManager.begin(wifiSSID, wifiPassword))
        {
            wifiManager.setServerURL(SERVER_URL);
            wifiManager.enableAutoReconnect(false);
            Serial.println("[WiFi] Connected (manual)");
        }
        else
        {
            Serial.println("[WiFi] Manual connect failed");
        }
    }
    else if (command == 'd' || command == 'D')
    {
        wifiManager.shutdown();
    }
    else if (command == 'g' || command == 'G')
    {
        runManualHealthCheck();
    }
    else if (command == 'p' || command == 'P')
    {
        runManualSensorPost();
    }
    else if (command == 't' || command == 'T')
    {
        runManualStatusPost();
    }
    else if (command == 'm' || command == 'M')
    {
        runManualCommandFetch();
    }
    else if (command == 'f' || command == 'F')
    {
        runManualFallPost();
    }
    else if (command == 'u' || command == 'U')
    {
        retryQueuedFalls();
    }
}

static void handleSerialCommands()
{
    static char serialLineBuffer[256];
    static size_t serialLineLength = 0;
    static bool capturingJSON = false;

    while (Serial.available() > 0)
    {
        char command = Serial.read();

        if (capturingJSON)
        {
            if (command == '\n' || command == '\r')
            {
                if (serialLineLength > 0)
                {
                    serialLineBuffer[serialLineLength] = '\0';
                    handleSerialLine(serialLineBuffer);
                }
                serialLineLength = 0;
                capturingJSON = false;
            }
            else if (serialLineLength < sizeof(serialLineBuffer) - 1)
            {
                serialLineBuffer[serialLineLength++] = command;
            }

            continue;
        }

        if (command == '{')
        {
            capturingJSON = true;
            serialLineLength = 0;
            serialLineBuffer[serialLineLength++] = command;
            continue;
        }

        if (command == '\n' || command == '\r' || command == ' ' || command == '\t')
        {
            continue;
        }

        handleSingleCharCommand(command);
    }
}

static void initializeSensors()
{
    Wire.begin(Board_Config::getSDA(), Board_Config::getSCL());
    Wire.setTimeout(50);

    currentSensorData.valid = false;
    currentSensorData.accel_z = 1.0f;
    currentSensorData.pressure = 1013.25f;

    if (imuSensor.begin())
    {
        imuSensor.configure();
        snapshot.imu_ok = true;
        Serial.println("✓ MPU6050 initialized");
    }
    else
    {
        snapshot.imu_ok = false;
        Serial.println("ERROR: MPU6050 initialization failed");
    }

    if (pressureSensor.begin())
    {
        pressureSensor.configure();
        delay(1000);
        pressureSensor.resetBaselineAltitude();
        snapshot.bmp_ok = true;
        Serial.println("✓ BMP280 initialized");
    }
    else
    {
        snapshot.bmp_ok = false;
        Serial.println("ERROR: BMP280 initialization failed");
    }

    if (fsr4.begin())
    {
        fsr4.calibrate();
        snapshot.fsr_ok = true;
        Serial.println("✓ FSR initialized");
    }
    else
    {
        snapshot.fsr_ok = false;
        Serial.println("ERROR: FSR initialization failed");
    }
}

static void initializeAudioAndDetection()
{
    if (audioManager.begin())
    {
        audioManager.setVolume(AUDIO_DEFAULT_VOLUME);
        Serial.println("✓ Audio initialized");
    }
    else
    {
        Serial.println("ERROR: Audio initialization failed");
    }

    if (fallDetector.init())
    {
        fallDetector.enableMonitoring();
        lastFallStatus = fallDetector.getCurrentStatus();
        Serial.println("✓ Fall detector initialized");
    }
    else
    {
        Serial.println("ERROR: Fall detector initialization failed");
    }
}

static void initializeWiFi()
{
    wifiLoadedFromNVS = WiFi_Credentials::load(wifiSSID, sizeof(wifiSSID),
                                               wifiPassword, sizeof(wifiPassword));

    Serial.println("--- Initializing WiFi ---");
    Serial.print("[WiFi] Credential source: ");
    Serial.println(wifiLoadedFromNVS ? "NVS" : "Config.h fallback");

    if (wifiManager.begin(wifiSSID, wifiPassword))
    {
        wifiManager.setServerURL(SERVER_URL);
        wifiManager.enableAutoReconnect(false);
        Serial.println("[WiFi] Connected for stage-1 integration");
        Serial.println("[WiFi] Auto-reconnect is disabled in this build");
    }
    else
    {
        Serial.println("[WiFi] Initial connection failed");
    }
}

static void sampleImuAndFsr()
{
    currentSensorData.timestamp = millis();
    currentSensorData.valid = true;

    if (imuSensor.isInitialized())
    {
        float temp = 0.0f;
        snapshot.imu_ok = imuSensor.readData(snapshot.accel_x,
                                             snapshot.accel_y,
                                             snapshot.accel_z,
                                             snapshot.gyro_x,
                                             snapshot.gyro_y,
                                             snapshot.gyro_z,
                                             temp);
        snapshot.imu_temp = temp;

        currentSensorData.accel_x = snapshot.accel_x;
        currentSensorData.accel_y = snapshot.accel_y;
        currentSensorData.accel_z = snapshot.accel_z;
        currentSensorData.gyro_x = snapshot.gyro_x;
        currentSensorData.gyro_y = snapshot.gyro_y;
        currentSensorData.gyro_z = snapshot.gyro_z;
    }
    else
    {
        snapshot.imu_ok = false;
        currentSensorData.valid = false;
    }

    if (fsr4.isInitialized())
    {
        snapshot.fsr_raw = fsr4.readRaw();
        snapshot.fsr_ok = true;
        currentSensorData.fsr_values[3] = snapshot.fsr_raw;
        currentSensorData.fsr_value = snapshot.fsr_raw;
    }
    else
    {
        snapshot.fsr_raw = 0;
        snapshot.fsr_ok = false;
        currentSensorData.fsr_values[3] = 0;
        currentSensorData.fsr_value = 0;
    }
}

static void sampleBarometer()
{
    if (pressureSensor.isInitialized())
    {
        snapshot.bmp_ok = pressureSensor.readData(snapshot.bmp_temp,
                                                  snapshot.pressure,
                                                  snapshot.altitude);
        currentSensorData.pressure = snapshot.pressure;
    }
    else
    {
        snapshot.bmp_ok = false;
    }
}

static void processFallDetection()
{
    if (!fallDetector.isMonitoring())
    {
        return;
    }

    fallDetector.processSensorData(currentSensorData);
    FallStatus_t status = fallDetector.getCurrentStatus();

    if (status != lastFallStatus)
    {
        Serial.print("[Detector] Status -> ");
        Serial.println(fallDetector.getStatusString(status));
        lastFallStatus = status;
    }

    if (status == FALL_STATUS_FALL_DETECTED && !alertLatched)
    {
        alertLatched = true;
        Serial.println("[Detector] FALL DETECTED");
        if (audioManager.isInitialized())
        {
            audioManager.playFallDetectedSequence();
        }

        populateEmergencyData(fallWorkBuffer, CONFIDENCE_CONFIRMED, 80, false);
        queueAndTransmitFall(fallWorkBuffer, "detected fall");
    }
    else if (status == FALL_STATUS_MONITORING)
    {
        alertLatched = false;
    }
}

void setup()
{
    setCpuFrequencyMhz(CPU_FREQUENCY_MHZ);

#if !PRODUCTION_MODE
    Serial.begin(SERIAL_BAUD_RATE);
    delay(300);
#endif

    generateDeviceID();
    Board_Config::begin();
    initializeSensors();
    initializeAudioAndDetection();
    initializeFallStore();
    initializeWiFi();
    printBanner();

    uint32_t now = millis();
    lastImuRead = now;
    lastBarometerRead = now;
    lastHeartbeat = now;
}

void loop()
{
    uint32_t now = millis();

    handleSerialCommands();

    if (now - lastImuRead >= SENSOR_READ_INTERVAL_MS)
    {
        lastImuRead = now;
        sampleImuAndFsr();
        processFallDetection();
    }

    if (now - lastBarometerRead >= BMP280_READ_INTERVAL_MS)
    {
        lastBarometerRead = now;
        sampleBarometer();
    }

    if (now - lastHeartbeat >= 5000)
    {
        lastHeartbeat = now;
        printHeartbeat(now);
    }

    delay(10);
}

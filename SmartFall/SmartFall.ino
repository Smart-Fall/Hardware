/*
 * SmartFall - Complete Wearable Fall Detection System
 * with WiFi/BLE Communication + Audio Alerts
 *
 * Hardware: ESP32 HUZZAH32 Feather
 * Sensors: MPU6050 (IMU), BMP280 (Pressure), MAX30102 (Heart Rate), FSR (Force)
 * Communication: WiFi + Bluetooth Low Energy (BLE)
 * Audio: PAM8302 2.5W Class D Amplifier
 *
 * This is the complete implementation with all features integrated.
 */

#include "sensors/MPU6050_Sensor.h"
#include "sensors/BMP280_Sensor.h"
#include "sensors/MAX30102_Sensor.h"
#include "sensors/FSR_Sensor.h"
#include "detection/fall_detector.h"
#include "detection/confidence_scorer.h"
#include "communication/WiFi_Manager.h"
#include "communication/BLE_Server.h"
#include "communication/Emergency_Comms.h"
#include "audio/Audio_Manager.h"
#include "utils/config.h"
#include "utils/data_types.h"

// Sensor instances
MPU6050_Sensor imuSensor;
BMP280_Sensor pressureSensor;
MAX30102_Sensor heartRateSensor;
FSR_Sensor forceSensor(FSR_ANALOG_PIN);

// Detection system
FallDetector fallDetector;
ConfidenceScorer confidenceScorer;

// Communication system
WiFi_Manager wifiManager;
BLE_Server bleServer;
Emergency_Comms emergencyComms(&wifiManager, &bleServer);

// Audio system
Audio_Manager audioManager(SPEAKER_PIN);

// System state
SensorData_t currentSensorData;
SystemStatus_t systemStatus;
uint32_t lastSensorRead = 0;
uint32_t lastStatusUpdate = 0;
bool systemInitialized = false;
bool alertActive = false;

// Device ID (MAC address based)
char deviceID[32];

void setup() {
  // Initialize serial communication
  Serial.begin(SERIAL_BAUD_RATE);
  delay(2000);  // Wait for serial monitor

  Serial.println("\n========================================");
  Serial.println("      SmartFall Detection System");
  Serial.println("   Complete with Audio & Communication");
  Serial.println("========================================\n");

  // Generate device ID from MAC address
  generateDeviceID();

  // Initialize SOS button
  pinMode(SOS_BUTTON_PIN, INPUT_PULLUP);

  // Initialize haptic and visual alert outputs
  pinMode(HAPTIC_PIN, OUTPUT);
  pinMode(VISUAL_ALERT_PIN, OUTPUT);

  digitalWrite(HAPTIC_PIN, LOW);
  digitalWrite(VISUAL_ALERT_PIN, LOW);

  // Initialize audio system
  Serial.println("--- Initializing Audio System ---");
  if (audioManager.begin()) {
    audioManager.setVolume(AUDIO_DEFAULT_VOLUME);
    Serial.println("✓ PAM8302 amplifier initialized");

    // Play startup melody
    audioManager.playStartupMelody();
    delay(500);
  } else {
    Serial.println("ERROR: Failed to initialize audio!");
  }

  // Initialize sensors
  Serial.println("\n--- Initializing Sensors ---");
  initializeSensors();

  // Initialize communication modules
  Serial.println("\n--- Initializing Communication ---");
  initializeCommunication();

  // Initialize fall detector
  if (fallDetector.init()) {
    Serial.println("✓ Fall detector initialized");
    fallDetector.enableMonitoring();
  } else {
    Serial.println("ERROR: Failed to initialize fall detector!");
  }

  // Initialize system status
  updateSystemStatus();

  systemInitialized = true;
  Serial.println("\n========================================");
  Serial.println("       SmartFall Ready!");
  Serial.println("========================================");
  Serial.println("Monitoring for falls...\n");

  // Play system ready voice alert
  if (AUDIO_ENABLE_VOICE_ALERTS) {
    audioManager.playVoiceAlert(VOICE_ALERT_SYSTEM_READY);
  }

  // Print connection info
  printSystemInfo();
}

void loop() {
  uint32_t currentTime = millis();

  // Check WiFi connection (auto-reconnect if enabled)
  wifiManager.checkConnection();

  // Process emergency alert queue (handle retries)
  emergencyComms.processAlertQueue();

  // Check SOS button
  if (digitalRead(SOS_BUTTON_PIN) == LOW) {
    handleSOSButton();
  }

  // Read sensors at configured rate
  if (currentTime - lastSensorRead >= SENSOR_READ_INTERVAL_MS) {
    lastSensorRead = currentTime;

    // Read all sensors
    readSensors();

    // Process sensor data through fall detector
    fallDetector.processSensorData(currentSensorData);

    // Check fall status
    FallStatus_t status = fallDetector.getCurrentStatus();

    if (status == FALL_STATUS_FALL_DETECTED && !alertActive) {
      handleFallDetected();
    }

    // Stream sensor data via BLE if enabled
    if (bleServer.shouldStream()) {
      bleServer.sendSensorData(currentSensorData);
    }

    // Debug output
    if (DEBUG_SENSOR_DATA && (currentTime % 1000 == 0)) {
      printSensorData();
    }
  }

  // Send periodic status updates
  if (currentTime - lastStatusUpdate >= 60000) {  // Every minute
    lastStatusUpdate = currentTime;
    updateSystemStatus();
    emergencyComms.sendStatusUpdate(systemStatus);

    // Check battery level
    if (systemStatus.battery_percentage < 20.0) {
      audioManager.playVoiceAlert(VOICE_ALERT_LOW_BATTERY);
    }
  }

  delay(MAIN_LOOP_DELAY_MS);
}

void generateDeviceID() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  snprintf(deviceID, sizeof(deviceID), "SF-%02X%02X%02X%02X%02X%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.print("Device ID: ");
  Serial.println(deviceID);
}

void initializeSensors() {
  systemStatus.sensors_initialized = true;

  if (!imuSensor.begin()) {
    Serial.println("ERROR: Failed to initialize MPU6050!");
    systemStatus.sensors_initialized = false;
    audioManager.playErrorTone();
  } else {
    Serial.println("✓ MPU6050 initialized");
    imuSensor.configure();
  }

  if (!pressureSensor.begin()) {
    Serial.println("ERROR: Failed to initialize BMP280!");
    systemStatus.sensors_initialized = false;
    audioManager.playErrorTone();
  } else {
    Serial.println("✓ BMP280 initialized");
    pressureSensor.configure();
    delay(1000);
    pressureSensor.resetBaselineAltitude();
  }

  if (!heartRateSensor.begin()) {
    Serial.println("ERROR: Failed to initialize MAX30102!");
    systemStatus.sensors_initialized = false;
    audioManager.playErrorTone();
  } else {
    Serial.println("✓ MAX30102 initialized");
    heartRateSensor.configure();
  }

  if (!forceSensor.begin()) {
    Serial.println("ERROR: Failed to initialize FSR!");
    systemStatus.sensors_initialized = false;
    audioManager.playErrorTone();
  } else {
    Serial.println("✓ FSR initialized");
    forceSensor.calibrate();
  }

  if (systemStatus.sensors_initialized) {
    audioManager.playConfirmationTone();
  }
}

void initializeCommunication() {
  // Initialize WiFi
  Serial.println("\n[WiFi] Connecting...");
  if (wifiManager.begin(WIFI_SSID, WIFI_PASSWORD)) {
    wifiManager.setServerURL(SERVER_URL);
    wifiManager.enableAutoReconnect(true);
    Serial.println("✓ WiFi connected");
    audioManager.playConfirmationTone();
  } else {
    Serial.println("✗ WiFi connection failed (will retry automatically)");
    audioManager.playErrorTone();
  }

  // Initialize BLE
  Serial.println("\n[BLE] Starting...");
  if (bleServer.begin(BLE_DEVICE_NAME)) {
    bleServer.setStreamingInterval(BLE_STREAMING_INTERVAL_MS);
    Serial.println("✓ BLE server started");
    audioManager.playConfirmationTone();

    // Register BLE callbacks
    bleServer.onConnect([]() {
      Serial.println("[BLE] Mobile app connected!");
      audioManager.playConfirmationTone();
    });

    bleServer.onDisconnect([]() {
      Serial.println("[BLE] Mobile app disconnected");
      if (AUDIO_ENABLE_VOICE_ALERTS) {
        audioManager.playVoiceAlert(VOICE_ALERT_CONNECTION_LOST);
      }
    });

    bleServer.onCommand(handleBLECommand);
  } else {
    Serial.println("✗ BLE initialization failed");
    audioManager.playErrorTone();
  }

  // Initialize emergency communication system
  if (emergencyComms.begin()) {
    emergencyComms.setMaxRetries(EMERGENCY_MAX_RETRIES);
    emergencyComms.setRetryInterval(EMERGENCY_RETRY_INTERVAL_MS);
    Serial.println("✓ Emergency communication system ready");
    audioManager.playConfirmationTone();
  }
}

void readSensors() {
  currentSensorData.timestamp = millis();
  currentSensorData.valid = true;

  // Read IMU (MPU6050)
  if (imuSensor.isInitialized()) {
    float temp;
    imuSensor.readData(currentSensorData.accel_x,
                       currentSensorData.accel_y,
                       currentSensorData.accel_z,
                       currentSensorData.gyro_x,
                       currentSensorData.gyro_y,
                       currentSensorData.gyro_z,
                       temp);
  } else {
    currentSensorData.accel_x = 0;
    currentSensorData.accel_y = 0;
    currentSensorData.accel_z = 1.0;  // 1g gravity
    currentSensorData.gyro_x = 0;
    currentSensorData.gyro_y = 0;
    currentSensorData.gyro_z = 0;
  }

  // Read pressure sensor (BMP280)
  if (pressureSensor.isInitialized()) {
    float temp, altitude;
    pressureSensor.readData(temp, currentSensorData.pressure, altitude);
  } else {
    currentSensorData.pressure = 1013.25;  // Sea level pressure
  }

  // Read heart rate (MAX30102)
  if (heartRateSensor.isInitialized()) {
    float bpm;
    bool fingerDetected;
    if (heartRateSensor.readHeartRate(bpm, fingerDetected)) {
      currentSensorData.heart_rate = bpm;
    } else {
      currentSensorData.heart_rate = 0;
    }
  } else {
    currentSensorData.heart_rate = 0;
  }

  // Read force sensor (FSR)
  if (forceSensor.isInitialized()) {
    currentSensorData.fsr_value = forceSensor.readRaw();
  } else {
    currentSensorData.fsr_value = 0;
  }
}

void handleFallDetected() {
  alertActive = true;

  Serial.println("\n!!! FALL DETECTED !!!");

  // Get confidence score from fall detector
  uint8_t confidence = confidenceScorer.getTotalScore();

  Serial.print("Confidence Score: ");
  Serial.print(confidence);
  Serial.println("/105");

  // Prepare emergency data
  EmergencyData_t emergencyData;
  emergencyData.timestamp = millis();
  emergencyData.confidence = confidenceScorer.getConfidenceLevel();
  emergencyData.confidence_score = confidence;
  emergencyData.battery_level = readBatteryLevel();
  emergencyData.sos_triggered = false;
  strncpy(emergencyData.device_id, deviceID, sizeof(emergencyData.device_id));

  // Copy sensor history (simplified - in full implementation, use detector's history)
  memcpy(emergencyData.sensor_history, &currentSensorData, sizeof(SensorData_t));

  // Activate alerts based on confidence
  if (confidence >= HIGH_CONFIDENCE_THRESHOLD) {
    Serial.println("HIGH CONFIDENCE FALL - Immediate Alert");
    activateFullAlert(true);
  } else if (confidence >= CONFIRMED_THRESHOLD) {
    Serial.println("CONFIRMED FALL - Delayed Alert");
    activateFullAlert(false);
  }

  // Send emergency alert via WiFi/BLE
  Serial.println("\n--- Transmitting Emergency Alert ---");

  // Audio announcement
  if (AUDIO_ENABLE_VOICE_ALERTS) {
    audioManager.playVoiceAlert(VOICE_ALERT_CALLING_HELP);
  }

  bool sent = emergencyComms.sendEmergencyAlert(emergencyData);

  if (sent) {
    Serial.println("✓ Emergency alert transmitted successfully");
    if (AUDIO_ENABLE_VOICE_ALERTS) {
      delay(500);
      audioManager.playVoiceAlert(VOICE_ALERT_HELP_SENT);
    }
  } else {
    Serial.println("⚠ Emergency alert queued for retry");
    audioManager.playWarningTone();
  }

  // Print detailed fall information
  fallDetector.printStageDetails();
  confidenceScorer.printScoreBreakdown();

  // User response countdown
  Serial.println("\n--- Countdown: Press SOS to confirm or wait to cancel ---");

  if (AUDIO_ENABLE_VOICE_ALERTS) {
    delay(1000);
    audioManager.playVoiceAlert(VOICE_ALERT_PRESS_BUTTON);
  }

  // Countdown with audio beeps
  for (int i = COUNTDOWN_DURATION_S; i > 0; i--) {
    // Check if user cancels
    if (digitalRead(SOS_BUTTON_PIN) == LOW) {
      Serial.println("User confirmed emergency!");
      break;
    }

    // Countdown beep every 10 seconds
    if (i % 10 == 0 || i <= 5) {
      audioManager.playTone(1000, 200);
      Serial.print("Countdown: ");
      Serial.println(i);
    }

    delay(1000);
  }

  // Reset detection
  fallDetector.resetDetection();
  deactivateFullAlert();
  alertActive = false;
}

void handleSOSButton() {
  alertActive = true;

  Serial.println("\n!!! SOS BUTTON PRESSED !!!");

  // Play SOS audio sequence
  audioManager.playSOSSequence();

  // Prepare emergency data
  EmergencyData_t emergencyData;
  emergencyData.timestamp = millis();
  emergencyData.confidence = CONFIDENCE_HIGH;
  emergencyData.confidence_score = MAX_CONFIDENCE_SCORE;
  emergencyData.battery_level = readBatteryLevel();
  emergencyData.sos_triggered = true;  // Manual trigger
  strncpy(emergencyData.device_id, deviceID, sizeof(emergencyData.device_id));

  // Activate alerts immediately
  activateFullAlert(true);

  // Audio announcement
  if (AUDIO_ENABLE_VOICE_ALERTS) {
    audioManager.playVoiceAlert(VOICE_ALERT_CALLING_HELP);
  }

  // Send emergency alert
  bool sent = emergencyComms.sendEmergencyAlert(emergencyData);

  if (sent && AUDIO_ENABLE_VOICE_ALERTS) {
    delay(500);
    audioManager.playVoiceAlert(VOICE_ALERT_HELP_SENT);
  }

  // Wait for button release
  while (digitalRead(SOS_BUTTON_PIN) == LOW) {
    delay(100);
  }

  delay(5000);  // Keep alerts active
  deactivateFullAlert();
  alertActive = false;
}

void handleBLECommand(uint8_t command, uint8_t* data, size_t length) {
  switch (command) {
    case BLE_CMD_CANCEL_ALERT:
      Serial.println("[App] Cancel alert command received");
      deactivateFullAlert();
      fallDetector.resetDetection();
      emergencyComms.clearPendingAlert();
      alertActive = false;
      audioManager.playPattern(ALERT_PATTERN_CANCEL);
      break;

    case BLE_CMD_TEST_ALERT:
      Serial.println("[App] Test alert command received");
      audioManager.playFallDetectedSequence();
      activateFullAlert(true);
      delay(2000);
      deactivateFullAlert();
      break;

    case BLE_CMD_GET_STATUS:
      Serial.println("[App] Status request received");
      updateSystemStatus();
      bleServer.sendStatusUpdate(systemStatus);
      break;

    case BLE_CMD_START_STREAMING:
      Serial.println("[App] Start streaming command");
      audioManager.playConfirmationTone();
      break;

    case BLE_CMD_STOP_STREAMING:
      Serial.println("[App] Stop streaming command");
      audioManager.playConfirmationTone();
      break;

    default:
      break;
  }
}

void activateFullAlert(bool immediate) {
  // Visual alert
  digitalWrite(VISUAL_ALERT_PIN, HIGH);

  // Haptic alert
  digitalWrite(HAPTIC_PIN, HIGH);

  // Audio alert
  if (immediate) {
    audioManager.playFallDetectedSequence();
  } else {
    audioManager.playPattern(ALERT_PATTERN_URGENT, 2);
  }
}

void deactivateFullAlert() {
  digitalWrite(HAPTIC_PIN, LOW);
  digitalWrite(VISUAL_ALERT_PIN, LOW);
  audioManager.stopPattern();
}

void updateSystemStatus() {
  systemStatus.wifi_connected = wifiManager.isConnected();
  systemStatus.bluetooth_connected = bleServer.isConnected();
  systemStatus.battery_percentage = readBatteryLevel();
  systemStatus.current_status = fallDetector.getCurrentStatus();
  systemStatus.uptime_ms = millis();
}

float readBatteryLevel() {
  // Read battery voltage (ESP32 ADC)
  // HUZZAH32 has built-in voltage divider on A13
  float voltage = analogRead(BATTERY_SENSE_PIN) * (3.3 / 4095.0) * 2.0;

  // Convert to percentage (3.0V = 0%, 4.2V = 100%)
  float percentage = (voltage - 3.0) / (4.2 - 3.0) * 100.0;
  percentage = constrain(percentage, 0.0, 100.0);

  return percentage;
}

void printSystemInfo() {
  Serial.println("\n========================================");
  Serial.println("         System Information");
  Serial.println("========================================");
  Serial.print("Device ID: ");
  Serial.println(deviceID);
  wifiManager.printConnectionInfo();
  bleServer.printConnectionInfo();
  emergencyComms.printStatus();
  Serial.print("Audio System: ");
  Serial.println(audioManager.isInitialized() ? "Active" : "Inactive");
  Serial.print("Audio Volume: ");
  Serial.print(audioManager.getVolume());
  Serial.println("%");
  Serial.println("========================================\n");
}

void printSensorData() {
  Serial.println("--- Sensor Data ---");
  Serial.print("Accel (g): ");
  Serial.print(currentSensorData.accel_x, 2);
  Serial.print(", ");
  Serial.print(currentSensorData.accel_y, 2);
  Serial.print(", ");
  Serial.println(currentSensorData.accel_z, 2);

  Serial.print("Gyro (°/s): ");
  Serial.print(currentSensorData.gyro_x, 2);
  Serial.print(", ");
  Serial.print(currentSensorData.gyro_y, 2);
  Serial.print(", ");
  Serial.println(currentSensorData.gyro_z, 2);

  Serial.print("Pressure: ");
  Serial.print(currentSensorData.pressure, 2);
  Serial.println(" hPa");

  Serial.print("Heart Rate: ");
  Serial.print(currentSensorData.heart_rate, 1);
  Serial.println(" BPM");

  Serial.print("FSR: ");
  Serial.println(currentSensorData.fsr_value);

  Serial.print("Battery: ");
  Serial.print(systemStatus.battery_percentage, 1);
  Serial.println("%");

  Serial.println();
}

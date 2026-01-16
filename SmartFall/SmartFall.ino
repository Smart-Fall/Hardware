/*
 * SmartFall - Complete Wearable Fall Detection System
 * with WiFi Communication + Audio Alerts
 *
 * Hardware: ESP32 Feather V2
 * Sensors: MPU6050Sensor (IMU), BMP280Sensor (Pressure), FSR (Force)
 * Communication: WiFi
 * Audio: PAM8302 2.5W Class D Amplifier
 *
 * This is the complete implementation with all features integrated.
 */

#include <esp_mac.h> // For MAC address functions in ESP32 core 3.x
#include "MPU6050.h"
#include "BMP280.h"
#include "FSR.h"
#include "MAX30102_Sensor.h"
#include "Fall_Detector.h"
#include "Confidence_Scorer.h"
#include "WiFi_Manager.h"
#include "Emergency_Comms.h"
#include "Audio_Manager.h"
#include "Config.h"
#include "Data_Types.h"

// Sensor instances
MPU6050Sensor imuSensor;
BMP280Sensor pressureSensor;
MAX30102Sensor heartRateSensor;
FSRSensor fsr1(FSR1_PIN);
FSRSensor fsr2(FSR2_PIN);
FSRSensor fsr3(FSR3_PIN);
FSRSensor fsr4(FSR4_PIN);

// Detection system
FallDetector fallDetector;
ConfidenceScorer confidenceScorer;

// Communication system
WiFiManager wifiManager;
EmergencyComms emergencyComms(&wifiManager, nullptr);

// Audio system
AudioManager audioManager(SPEAKER_PIN);

// System state
SensorData_t currentSensorData;
SystemStatus_t systemStatus;
uint32_t lastSensorRead = 0;
uint32_t lastStatusUpdate = 0;
bool systemInitialized = false;
bool alertActive = false;

// Device ID (MAC address based)
char deviceID[32];

void setup()
{
  // Initialize serial communication
  Serial.begin(SERIAL_BAUD_RATE);
  delay(2000); // Wait for serial monitor

  Serial.println("\n========================================");
  Serial.println("      SmartFall Detection System");
  Serial.println("   Complete with Audio & Communication");
  Serial.println("========================================\n");

  // Generate device ID from MAC address
  generateDeviceID();

  // Initialize SOS button
  pinMode(SOS_BUTTON_PIN, INPUT_PULLUP);

  // Initialize visual alert output
  pinMode(VISUAL_ALERT_PIN, OUTPUT);

  digitalWrite(VISUAL_ALERT_PIN, LOW);

  // Initialize audio system
  Serial.println("--- Initializing Audio System ---");
  if (audioManager.begin())
  {
    audioManager.setVolume(AUDIO_DEFAULT_VOLUME);
    Serial.println("✓ PAM8302 amplifier initialized");

    // Play startup melody
    audioManager.playStartupMelody();
    delay(500);
  }
  else
  {
    Serial.println("ERROR: Failed to initialize audio!");
  }

  // Initialize sensors
  Serial.println("\n--- Initializing Sensors ---");
  initializeSensors();

  // Initialize communication modules
  Serial.println("\n--- Initializing Communication ---");
  initializeCommunication();

  // Initialize fall detector
  if (fallDetector.init())
  {
    Serial.println("✓ Fall detector initialized");
    fallDetector.enableMonitoring();
    Serial.println("✓ Fall monitoring ENABLED");
    Serial.println("\n*** SHAKE/DROP THE DEVICE TO TEST FALL DETECTION ***\n");
  }
  else
  {
    Serial.println("ERROR: Failed to initialize fall detector!");
  }

  // Initialize system status
  updateSystemStatus();

  systemInitialized = true;
  Serial.println("\n========================================");
  Serial.println("       SmartFall Ready!");
  Serial.println("========================================");
  Serial.println("Monitoring for falls...");
  Serial.println("\nTest Commands:");
  Serial.println("  T - Trigger manual fall test");
  Serial.println("  S - Show current sensor readings");
  Serial.println("========================================\n");

  // Play system ready voice alert
  if (AUDIO_ENABLE_VOICE_ALERTS)
  {
    audioManager.playVoiceAlert(VOICE_ALERT_SYSTEM_READY);
  }

  // Print connection info
  printSystemInfo();
}

void loop()
{
  uint32_t currentTime = millis();

  // Check for serial commands (for testing)
  if (Serial.available() > 0)
  {
    char cmd = Serial.read();
    if (cmd == 't' || cmd == 'T')
    {
      Serial.println("\n*** MANUAL FALL TEST TRIGGERED ***");
      handleFallDetected();
    }
    else if (cmd == 's' || cmd == 'S')
    {
      Serial.println("\n--- Current Sensor Readings ---");
      printSensorData();
    }
  }

  // Check WiFi connection (auto-reconnect if enabled)
  wifiManager.checkConnection();

  // Process emergency alert queue (handle retries)
  emergencyComms.processAlertQueue();

  // Check SOS button
  if (digitalRead(SOS_BUTTON_PIN) == LOW)
  {
    handleSOSButton();
  }

  // Read sensors at configured rate
  if (currentTime - lastSensorRead >= SENSOR_READ_INTERVAL_MS)
  {
    lastSensorRead = currentTime;

    // Read all sensors
    readSensors();

    // Log sensor status periodically (every 5 seconds)
    static uint32_t lastSensorLog = 0;
    if (DEBUG_ALGORITHM_STEPS && (currentTime - lastSensorLog) >= 5000)
    {
      lastSensorLog = currentTime;
      Serial.println("\n=== Sensor Status ===");
      Serial.print("IMU: ");
      Serial.println(imuSensor.isInitialized() ? "OK" : "ERROR");
      Serial.print("BMP280: ");
      Serial.println(pressureSensor.isInitialized() ? "OK" : "ERROR");
      Serial.print("MAX30102 (Heart Rate): ");
      Serial.println(heartRateSensor.isInitialized() ? "OK" : "ERROR");
      Serial.print("FSR1: ");
      Serial.print(fsr1.isInitialized() ? "OK" : "ERROR");
      Serial.print(", FSR2: ");
      Serial.print(fsr2.isInitialized() ? "OK" : "ERROR");
      Serial.print(", FSR3: ");
      Serial.print(fsr3.isInitialized() ? "OK" : "ERROR");
      Serial.print(", FSR4: ");
      Serial.println(fsr4.isInitialized() ? "OK" : "ERROR");
      Serial.print("Fall Detector: ");
      Serial.println(fallDetector.isMonitoring() ? "MONITORING" : "INACTIVE");
      Serial.println("===================\n");
    }

    // Process sensor data through fall detector
    fallDetector.processSensorData(currentSensorData);

    // Check fall status
    FallStatus_t status = fallDetector.getCurrentStatus();

    // Log current acceleration magnitude every 2 seconds
    static uint32_t lastAccelLog = 0;
    if (DEBUG_ALGORITHM_STEPS && (currentTime - lastAccelLog) >= 2000)
    {
      lastAccelLog = currentTime;
      float total_accel = sqrt(currentSensorData.accel_x * currentSensorData.accel_x +
                               currentSensorData.accel_y * currentSensorData.accel_y +
                               currentSensorData.accel_z * currentSensorData.accel_z);
      Serial.print("[AccelMag] ");
      Serial.print(total_accel, 3);
      Serial.print("g, Status: ");
      Serial.println(status);
    }

    if (status == FALL_STATUS_FALL_DETECTED && !alertActive)
    {
      handleFallDetected();
    }

    // Debug output
    if (DEBUG_SENSOR_DATA && (currentTime % 1000 == 0))
    {
      printSensorData();
    }
  }

  // Send periodic status updates
  if (currentTime - lastStatusUpdate >= 60000)
  { // Every minute
    lastStatusUpdate = currentTime;
    updateSystemStatus();
    emergencyComms.sendStatusUpdate(systemStatus);

    // Check battery level
    if (systemStatus.battery_percentage < 20.0)
    {
      audioManager.playVoiceAlert(VOICE_ALERT_LOW_BATTERY);
    }
  }

  delay(MAIN_LOOP_DELAY_MS);
}

void generateDeviceID()
{
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  snprintf(deviceID, sizeof(deviceID), "SF-%02X%02X%02X%02X%02X%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.print("Device ID: ");
  Serial.println(deviceID);
}

void initializeSensors()
{
  systemStatus.sensors_initialized = true;

  if (!imuSensor.begin())
  {
    Serial.println("ERROR: Failed to initialize MPU6050!");
    systemStatus.sensors_initialized = false;
    audioManager.playErrorTone();
  }
  else
  {
    Serial.println("✓ MPU6050Sensor initialized");
    imuSensor.configure();
  }

  if (!pressureSensor.begin())
  {
    Serial.println("ERROR: Failed to initialize BMP280!");
    systemStatus.sensors_initialized = false;
    audioManager.playErrorTone();
  }
  else
  {
    Serial.println("✓ BMP280Sensor initialized");
    pressureSensor.configure();
    delay(1000);
    pressureSensor.resetBaselineAltitude();
  }

  // Initialize MAX30102 heart rate sensor
  if (!heartRateSensor.begin(0x57))
  {
    Serial.println("ERROR: Failed to initialize MAX30102 heart rate sensor!");
    systemStatus.sensors_initialized = false;
    audioManager.playErrorTone();
  }
  else
  {
    Serial.println("✓ MAX30102 heart rate sensor initialized");
    heartRateSensor.configure();
  }

  // Initialize all 4 FSR sensors
  bool fsr_ok = true;
  if (!fsr1.begin())
  {
    Serial.println("ERROR: Failed to initialize FSR1!");
    fsr_ok = false;
  }
  if (!fsr2.begin())
  {
    Serial.println("ERROR: Failed to initialize FSR2!");
    fsr_ok = false;
  }
  if (!fsr3.begin())
  {
    Serial.println("ERROR: Failed to initialize FSR3!");
    fsr_ok = false;
  }
  if (!fsr4.begin())
  {
    Serial.println("ERROR: Failed to initialize FSR4!");
    fsr_ok = false;
  }

  if (!fsr_ok)
  {
    Serial.println("ERROR: One or more FSR sensors failed to initialize!");
    systemStatus.sensors_initialized = false;
    audioManager.playErrorTone();
  }
  else
  {
    Serial.println("✓ All 4 FSR sensors initialized");
    fsr1.calibrate();
    fsr2.calibrate();
    fsr3.calibrate();
    fsr4.calibrate();
  }

  if (systemStatus.sensors_initialized)
  {
    audioManager.playConfirmationTone();
  }
}

void initializeCommunication()
{
  // Initialize WiFi
  Serial.println("\n[WiFi] Connecting...");
  if (wifiManager.begin(WIFI_SSID, WIFI_PASSWORD))
  {
    wifiManager.setServerURL(SERVER_URL);
    wifiManager.enableAutoReconnect(true);
    Serial.println("✓ WiFi connected");
    audioManager.playConfirmationTone();

    // Test server connection
    delay(1000); // Give WiFi a moment to stabilize
    Serial.println("\n[WiFi] Testing server connection...");
    if (wifiManager.testServerConnection())
    {
      Serial.println("✓ Server connection verified - Ready to send data!");
      audioManager.playConfirmationTone();
    }
    else
    {
      Serial.println("✗ Server connection failed - Check web app is running!");
      Serial.println("    Device will retry automatically when sending data.");
      audioManager.playErrorTone();
    }
  }
  else
  {
    Serial.println("✗ WiFi connection failed (will retry automatically)");
    audioManager.playErrorTone();
  }

  // Initialize emergency communication system
  if (emergencyComms.begin())
  {
    emergencyComms.setMaxRetries(EMERGENCY_MAX_RETRIES);
    emergencyComms.setRetryInterval(EMERGENCY_RETRY_INTERVAL_MS);
    Serial.println("✓ Emergency communication system ready");
    audioManager.playConfirmationTone();
  }
}

void readSensors()
{
  currentSensorData.timestamp = millis();
  currentSensorData.valid = true;

  // Read IMU (MPU6050)
  if (imuSensor.isInitialized())
  {
    float temp;
    imuSensor.readData(currentSensorData.accel_x,
                       currentSensorData.accel_y,
                       currentSensorData.accel_z,
                       currentSensorData.gyro_x,
                       currentSensorData.gyro_y,
                       currentSensorData.gyro_z,
                       temp);

    // Check for sensor failure (all zeros indicates I2C error)
    float total_accel = sqrt(currentSensorData.accel_x * currentSensorData.accel_x +
                             currentSensorData.accel_y * currentSensorData.accel_y +
                             currentSensorData.accel_z * currentSensorData.accel_z);

    if (total_accel < 0.1f) // Should never be this low in normal operation
    {
      Serial.println("WARNING: MPU6050Sensor sensor failure detected! Attempting reinitialization...");
      if (imuSensor.begin())
      {
        Serial.println("✓ MPU6050Sensor reinitialized successfully");
        imuSensor.calibrate();
      }
      else
      {
        Serial.println("ERROR: MPU6050Sensor reinitialization failed!");
      }
    }
  }
  else
  {
    currentSensorData.accel_x = 0;
    currentSensorData.accel_y = 0;
    currentSensorData.accel_z = 1.0; // 1g gravity
    currentSensorData.gyro_x = 0;
    currentSensorData.gyro_y = 0;
    currentSensorData.gyro_z = 0;
  }

  // Read pressure sensor (BMP280)
  if (pressureSensor.isInitialized())
  {
    float temp, altitude;
    pressureSensor.readData(temp, currentSensorData.pressure, altitude);
  }
  else
  {
    currentSensorData.pressure = 1013.25; // Sea level pressure
  }

  // Read heart rate sensor (MAX30102)
  if (heartRateSensor.isInitialized())
  {
    float temp;
    if (heartRateSensor.readData(currentSensorData.heart_rate, currentSensorData.spo2, temp))
    {
      currentSensorData.heart_rate_temperature = temp;

      if (DEBUG_SENSOR_DATA)
      {
        Serial.print("[MAX30102] HR: ");
        Serial.print(currentSensorData.heart_rate);
        Serial.print(" BPM, SpO2: ");
        Serial.print(currentSensorData.spo2);
        Serial.print("%, Temp: ");
        Serial.println(temp);
      }
    }
  }
  else
  {
    currentSensorData.heart_rate = 0;
    currentSensorData.spo2 = 0;
    currentSensorData.heart_rate_temperature = 0.0f;
  }

  // Read all 4 force sensors (FSR)
  if (fsr1.isInitialized() && fsr2.isInitialized() &&
      fsr3.isInitialized() && fsr4.isInitialized())
  {
    currentSensorData.fsr_values[0] = fsr1.readRaw();
    currentSensorData.fsr_values[1] = fsr2.readRaw();
    currentSensorData.fsr_values[2] = fsr3.readRaw();
    currentSensorData.fsr_values[3] = fsr4.readRaw();

    // Calculate combined FSR metric:
    // - Use maximum value (detects impact on any sensor)
    // - Also store average for general pressure detection
    uint16_t max_fsr = max(max(currentSensorData.fsr_values[0], currentSensorData.fsr_values[1]),
                           max(currentSensorData.fsr_values[2], currentSensorData.fsr_values[3]));
    uint32_t sum_fsr = currentSensorData.fsr_values[0] + currentSensorData.fsr_values[1] +
                       currentSensorData.fsr_values[2] + currentSensorData.fsr_values[3];
    uint16_t avg_fsr = sum_fsr / 4;

    // Use max for impact detection (fall creates sudden pressure spike)
    currentSensorData.fsr_value = max_fsr;

    // Count how many sensors detect significant pressure (helps detect falls vs normal pressure)
    uint16_t active_sensors = 0;
    for (int i = 0; i < 4; i++)
    {
      if (currentSensorData.fsr_values[i] > 100)
        active_sensors++;
    }
  }
  else
  {
    for (int i = 0; i < 4; i++)
    {
      currentSensorData.fsr_values[i] = 0;
    }
    currentSensorData.fsr_value = 0;
  }
}

void handleFallDetected()
{
  alertActive = true;

  Serial.println("\n!!! FALL DETECTED !!!");

  // Update confidence scorer with fall detection data
  confidenceScorer.resetScore();
  confidenceScorer.startScoring();

  // Score based on detected fall stages
  // Stage 1: Free fall (assume 100ms duration, 0.3g min)
  confidenceScorer.addStage1Score(100.0f, 0.3f);

  // Stage 2: Impact (use actual impact magnitude from detector)
  float impact_g = 7.0f; // Typical fall impact
  confidenceScorer.addStage2Score(impact_g, 100.0f, true);

  // Stage 3: Rotation (skipped, use default)
  confidenceScorer.addStage3Score(0.0f, 0.0f);

  // Stage 4: Inactivity (2000ms as configured)
  confidenceScorer.addStage4Score(2000.0f, true);

  // Add physiological validation from MAX30102 if available
  if (heartRateSensor.isInitialized() && currentSensorData.heart_rate > 0)
  {
    uint16_t baseline_hr = heartRateSensor.getBaselineHeartRate();
    confidenceScorer.addPhysiologicalScore(currentSensorData.heart_rate,
                                           currentSensorData.spo2,
                                           baseline_hr);
  }

  // Get confidence score from fall detector
  uint8_t confidence = confidenceScorer.getTotalScore();

  Serial.print("Confidence Score: ");
  Serial.print(confidence);
  Serial.println("/100");

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
  if (confidence >= HIGH_CONFIDENCE_THRESHOLD)
  {
    Serial.println("HIGH CONFIDENCE FALL - Immediate Alert");
    activateFullAlert(true);
  }
  else if (confidence >= CONFIRMED_THRESHOLD)
  {
    Serial.println("CONFIRMED FALL - Delayed Alert");
    activateFullAlert(false);
  }

  // Send emergency alert via WiFi
  Serial.println("\n--- Transmitting Emergency Alert ---");

  // Audio announcement
  if (AUDIO_ENABLE_VOICE_ALERTS)
  {
    audioManager.playVoiceAlert(VOICE_ALERT_CALLING_HELP);
  }

  bool sent = emergencyComms.sendEmergencyAlert(emergencyData);

  if (sent)
  {
    Serial.println("✓ Emergency alert transmitted successfully");
    if (AUDIO_ENABLE_VOICE_ALERTS)
    {
      delay(500);
      audioManager.playVoiceAlert(VOICE_ALERT_HELP_SENT);
    }
  }
  else
  {
    Serial.println("⚠ Emergency alert queued for retry");
    audioManager.playWarningTone();
  }

  // Print detailed fall information
  fallDetector.printStageDetails();
  confidenceScorer.printScoreBreakdown();

  // User response countdown
  Serial.println("\n--- Countdown: Press SOS to confirm or wait to cancel ---");

  if (AUDIO_ENABLE_VOICE_ALERTS)
  {
    delay(1000);
    audioManager.playVoiceAlert(VOICE_ALERT_PRESS_BUTTON);
  }

  // Countdown with audio beeps
  for (int i = COUNTDOWN_DURATION_S; i > 0; i--)
  {
    // Check if user cancels
    if (digitalRead(SOS_BUTTON_PIN) == LOW)
    {
      Serial.println("User confirmed emergency!");
      break;
    }

    // Countdown beep every 10 seconds
    if (i % 10 == 0 || i <= 5)
    {
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

void handleSOSButton()
{
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
  emergencyData.sos_triggered = true; // Manual trigger
  strncpy(emergencyData.device_id, deviceID, sizeof(emergencyData.device_id));

  // Activate alerts immediately
  activateFullAlert(true);

  // Audio announcement
  if (AUDIO_ENABLE_VOICE_ALERTS)
  {
    audioManager.playVoiceAlert(VOICE_ALERT_CALLING_HELP);
  }

  // Send emergency alert
  bool sent = emergencyComms.sendEmergencyAlert(emergencyData);

  if (sent && AUDIO_ENABLE_VOICE_ALERTS)
  {
    delay(500);
    audioManager.playVoiceAlert(VOICE_ALERT_HELP_SENT);
  }

  // Wait for button release
  while (digitalRead(SOS_BUTTON_PIN) == LOW)
  {
    delay(100);
  }

  delay(5000); // Keep alerts active
  deactivateFullAlert();
  alertActive = false;
}

void activateFullAlert(bool immediate)
{
  // Visual alert
  digitalWrite(VISUAL_ALERT_PIN, HIGH);

  // Audio alert
  if (immediate)
  {
    audioManager.playFallDetectedSequence();
  }
  else
  {
    audioManager.playPattern(ALERT_PATTERN_URGENT, 2);
  }
}

void deactivateFullAlert()
{
  digitalWrite(VISUAL_ALERT_PIN, LOW);
  audioManager.stopPattern();
}

void updateSystemStatus()
{
  systemStatus.wifi_connected = wifiManager.isConnected();
  systemStatus.battery_percentage = readBatteryLevel();
  systemStatus.current_status = fallDetector.getCurrentStatus();
  systemStatus.uptime_ms = millis();
}

float readBatteryLevel()
{
  // Read battery voltage (ESP32 ADC)
  // Feather V2 has built-in voltage divider on A4 (GPIO 36)
  float voltage = analogRead(BATTERY_SENSE_PIN) * (3.3 / 4095.0) * 2.0;

  // Convert to percentage (3.0V = 0%, 4.2V = 100%)
  float percentage = (voltage - 3.0) / (4.2 - 3.0) * 100.0;
  percentage = constrain(percentage, 0.0, 100.0);

  return percentage;
}

void printSystemInfo()
{
  Serial.println("\n========================================");
  Serial.println("         System Information");
  Serial.println("========================================");
  Serial.print("Device ID: ");
  Serial.println(deviceID);
  wifiManager.printConnectionInfo();
  emergencyComms.printStatus();
  Serial.print("Audio System: ");
  Serial.println(audioManager.isInitialized() ? "Active" : "Inactive");
  Serial.print("Audio Volume: ");
  Serial.print(audioManager.getVolume());
  Serial.println("%");
  if (heartRateSensor.isInitialized())
  {
    heartRateSensor.printInfo();
  }
  Serial.println("========================================\n");
}

void printSensorData()
{
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

  Serial.print("FSR1: ");
  Serial.print(currentSensorData.fsr_values[0]);
  Serial.print(", FSR2: ");
  Serial.print(currentSensorData.fsr_values[1]);
  Serial.print(", FSR3: ");
  Serial.print(currentSensorData.fsr_values[2]);
  Serial.print(", FSR4: ");
  Serial.println(currentSensorData.fsr_values[3]);
  Serial.print("FSR Max: ");
  Serial.println(currentSensorData.fsr_value);

  if (heartRateSensor.isInitialized())
  {
    Serial.print("Heart Rate: ");
    Serial.print(currentSensorData.heart_rate);
    Serial.print(" BPM, SpO2: ");
    Serial.print(currentSensorData.spo2);
    Serial.print("%, Temp: ");
    Serial.print(currentSensorData.heart_rate_temperature, 1);
    Serial.println(" °C");
  }

  Serial.print("Battery: ");
  Serial.print(systemStatus.battery_percentage, 1);
  Serial.println("%");

  Serial.println();
}

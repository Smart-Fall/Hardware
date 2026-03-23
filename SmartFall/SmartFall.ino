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
#include <Wire.h>
#include "Board_Config.h"
#include "MPU6050_Sensor.h"
#include "BMP280_Sensor.h"
#include "FSR_Sensor.h"
#include "MAX30102_Sensor.h"
#include "Fall_Detector.h"
#include "Confidence_Scorer.h"
#include "WiFi_Manager.h"
#include "Emergency_Comms.h"
#include "Audio_Manager.h"
#include "Log_Manager.h"
#include "Config.h"
#include "Data_Types.h"

#if ENABLE_BLE_FALLBACK
#include "BLE_Server.h"
BLE_Server bleServer;
bool bleActive = false;
#endif

// Sensor instances
MPU6050_Sensor imuSensor;
BMP280_Sensor pressureSensor;
MAX30102Sensor heartRateSensor;
FSR_Sensor fsr4(FSR_ANALOG_PIN);

// Detection system
FallDetector fallDetector;
ConfidenceScorer confidenceScorer;

// Communication system
WiFi_Manager wifiManager;
Emergency_Comms emergencyComms(&wifiManager, nullptr);

// Audio system
Audio_Manager audioManager(SPEAKER_PIN);

// System state
SensorData_t currentSensorData;
SystemStatus_t systemStatus;
uint32_t lastSensorRead = 0;
uint32_t lastStatusUpdate = 0;
uint32_t lastSensorStream = 0;
bool systemInitialized = false;
bool alertActive = false;

// Non-blocking alert countdown state
uint32_t alertCountdownEnd = 0;
bool countdownActive = false;

// Device ID (MAC address based)
char deviceID[32];

void setup()
{
  // CPU frequency scaling — 80MHz is sufficient for sensor polling + WiFi
  setCpuFrequencyMhz(CPU_FREQUENCY_MHZ);

  // Initialize serial communication
#if !PRODUCTION_MODE
  Serial.begin(SERIAL_BAUD_RATE);
  delay(2000); // Wait for serial monitor
#endif

  Serial.println("\n========================================");
  Serial.println("      SmartFall Detection System");
  Serial.println("   Complete with Audio & Communication");
  Serial.println("========================================\n");

  // Generate device ID from MAC address
  generateDeviceID();

  // Initialize board detection and configure I2C pins
  Board_Config::begin();

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

  // Power saving: disable MAX30102 collection during normal monitoring
  // It will be enabled on-demand when a fall is detected
  if (!MAX30102_ALWAYS_ON && heartRateSensor.isInitialized())
  {
    heartRateSensor.stopCollection();
    Serial.println("[Power] MAX30102 collection stopped (on-demand mode)");
  }

#if ENABLE_BLE_FALLBACK
  // BLE fallback: only start if WiFi failed
  if (!wifiManager.isConnected())
  {
    if (bleServer.begin(BLE_DEVICE_NAME))
    {
      bleActive = true;
      Serial.println("[Power] BLE started as WiFi fallback");
    }
  }
#endif

  // Initialize remote log manager (after WiFi so it can use the connection)
  logManager.begin(&wifiManager, deviceID);
  logManager.log(LOG_LEVEL_INFO, LOG_CAT_SYSTEM, "SmartFall system initialized");

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

#if ENABLE_BLE_FALLBACK
  // BLE fallback management: start BLE when WiFi drops, stop when WiFi reconnects
  if (!wifiManager.isConnected() && !bleActive)
  {
    if (bleServer.begin(BLE_DEVICE_NAME))
    {
      bleActive = true;
      Serial.println("[Power] BLE activated — WiFi unavailable");
    }
  }
  else if (wifiManager.isConnected() && bleActive && !bleServer.isConnected())
  {
    bleServer.end();
    bleActive = false;
    Serial.println("[Power] BLE deactivated — WiFi restored");
  }
#endif

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
      Serial.print("FSR4: ");
      Serial.println(fsr4.isInitialized() ? "OK" : "ERROR");
      Serial.print("Fall Detector: ");
      Serial.println(fallDetector.isMonitoring() ? "MONITORING" : "INACTIVE");
      Serial.println("===================\n");
    }

    // Process sensor data through fall detector
    fallDetector.processSensorData(currentSensorData);

    // Check fall status
    FallStatus_t status = fallDetector.getCurrentStatus();

    // Log current acceleration magnitude and FSR every 2 seconds
    static uint32_t lastAccelLog = 0;
    if (DEBUG_ALGORITHM_STEPS && (currentTime - lastAccelLog) >= 2000)
    {
      lastAccelLog = currentTime;
      float total_accel = sqrt(currentSensorData.accel_x * currentSensorData.accel_x +
                               currentSensorData.accel_y * currentSensorData.accel_y +
                               currentSensorData.accel_z * currentSensorData.accel_z);
      Serial.print("[AccelMag] ");
      Serial.print(total_accel, 3);
      Serial.print("g | [FSR] raw=");
      Serial.print(currentSensorData.fsr_value);
      Serial.print(" baseline=");
      Serial.print(fsr4.getBaseline());
      Serial.print(" | Status: ");
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

  // Handle alert countdown (non-blocking)
  if (countdownActive)
  {
    static uint32_t lastBeep = 0;
    uint32_t remaining = (alertCountdownEnd > currentTime) ? (alertCountdownEnd - currentTime) / 1000 : 0;

    // Periodic beep feedback
    if (currentTime - lastBeep >= 10000 || remaining <= 5)
    {
      lastBeep = currentTime;
      audioManager.playTone(1000, 200);
      Serial.print("Countdown: ");
      Serial.println(remaining);
    }

    bool expired = (currentTime >= alertCountdownEnd);
    bool confirmed = (digitalRead(SOS_BUTTON_PIN) == LOW);

    if (expired || confirmed)
    {
      if (confirmed) Serial.println("User confirmed emergency!");
      fallDetector.resetDetection();
      deactivateFullAlert();
      alertActive = false;
      countdownActive = false;

      // Power: deactivate MAX30102 after alert resolves
      if (!MAX30102_ALWAYS_ON && heartRateSensor.isInitialized())
      {
        heartRateSensor.stopCollection();
        Serial.println("[Power] MAX30102 deactivated — alert resolved");
      }
    }
  }

  // Send periodic sensor data stream (dynamic interval based on alert state)
  uint32_t streamInterval = alertActive ? SENSOR_STREAM_EMERGENCY_MS : SENSOR_STREAM_INTERVAL_MS;
  if (currentTime - lastSensorStream >= streamInterval)
  {
    lastSensorStream = currentTime;
    emergencyComms.sendSensorData(currentSensorData, deviceID);
  }

  // Send periodic status updates
  if (currentTime - lastStatusUpdate >= 60000)
  { // Every minute
    lastStatusUpdate = currentTime;
    updateSystemStatus();
    emergencyComms.sendStatusUpdate(systemStatus, deviceID);

    // Check battery level
    if (systemStatus.battery_percentage < 20.0)
    {
      audioManager.playVoiceAlert(VOICE_ALERT_LOW_BATTERY);
    }
  }

  // Flush remote log buffer on schedule
  logManager.flush();

  delay(MAIN_LOOP_DELAY_MS);
}

void generateDeviceID()
{
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  snprintf(deviceID, sizeof(deviceID), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.print("Device ID: ");
  Serial.println(deviceID);
}

void initializeSensors()
{
  systemStatus.sensors_initialized = true;

  // Initialize I2C bus once; individual sensors must not call Wire.begin()
  Wire.begin(Board_Config::getSDA(), Board_Config::getSCL());

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

  // Initialize FSR4 sensor
  if (!fsr4.begin())
  {
    Serial.println("ERROR: Failed to initialize FSR4!");
    systemStatus.sensors_initialized = false;
    audioManager.playErrorTone();
  }
  else
  {
    Serial.println("✓ FSR4 sensor initialized");
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

    Serial.println("✓ WiFi connected - Ready to send data!");
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

    // Check for sensor failure: readData() sets initialized=false when it
    // detects a NACK or stale data, so check that flag rather than magnitude.
    if (!imuSensor.isInitialized())
    {
      static uint32_t lastReinitAttempt = 0;
      uint32_t now = millis();

      // Cooldown: don't attempt reinit more than once every 2 seconds.
      // Loose wiring can cause rapid successive failures; without a cooldown
      // multiple reinits stack up and corrupt the library's internal I2C state.
      if (now - lastReinitAttempt >= 2000)
      {
        lastReinitAttempt = now;
        Serial.println("WARNING: MPU6050 not responding - attempting reinitialization...");
        logManager.log(LOG_LEVEL_ERROR, LOG_CAT_SENSOR,
                       "MPU6050 failure detected - reinitializing");

        if (imuSensor.begin())
        {
          delay(50); // Let sensor settle before first read
          imuSensor.configure();
          Serial.println("✓ MPU6050 reinitialized successfully");
        }
        else
        {
          Serial.println("ERROR: MPU6050 reinitialization failed!");
        }
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

  // Read FSR4 force sensor
  if (fsr4.isInitialized())
  {
    currentSensorData.fsr_values[3] = fsr4.readRaw();
    // Use FSR4 value for impact detection
    currentSensorData.fsr_value = currentSensorData.fsr_values[3];
  }
  else
  {
    currentSensorData.fsr_values[3] = 0;
    currentSensorData.fsr_value = 0;
  }
}

void handleFallDetected()
{
  alertActive = true;

  Serial.println("\n!!! FALL DETECTED !!!");

  // Power: activate MAX30102 for vitals monitoring during fall event
  if (!MAX30102_ALWAYS_ON && heartRateSensor.isInitialized())
  {
    heartRateSensor.startCollection();
    Serial.println("[Power] MAX30102 activated for fall event");
  }

  // Update confidence scorer with actual measured values from the fall detector
  confidenceScorer.resetScore();
  confidenceScorer.startScoring();

  // Stage 1: Free fall — use measured duration and minimum accel magnitude during fall
  float freefallDuration = fallDetector.getFreefallDuration();
  confidenceScorer.addStage1Score(freefallDuration, fallDetector.getMinAcceleration());

  // Stage 2: Impact — use actual peak impact and FSR reading
  float impact_g = fallDetector.getMaxImpact();
  bool fsrImpact = (currentSensorData.fsr_value > 200);
  confidenceScorer.addStage2Score(impact_g, freefallDuration, fsrImpact);

  // Stage 3: Rotation — use actual peak angular velocity
  float maxRotation = fallDetector.getMaxRotation();
  confidenceScorer.addStage3Score(maxRotation, 0.0f);

  // Stage 4: Inactivity — use actual measured inactivity duration
  confidenceScorer.addStage4Score(fallDetector.getInactivityDuration(), true);

  // Add physiological validation from MAX30102 if available
  // TODO: Implement addPhysiologicalScore in ConfidenceScorer
  // if (heartRateSensor.isInitialized() && currentSensorData.heart_rate > 0)
  // {
  //   uint16_t baseline_hr = heartRateSensor.getBaselineHeartRate();
  //   confidenceScorer.addPhysiologicalScore(currentSensorData.heart_rate,
  //                                          currentSensorData.spo2,
  //                                          baseline_hr);
  // }

  // Get confidence score from fall detector
  uint8_t confidence = confidenceScorer.getTotalScore();

  Serial.print("Confidence Score: ");
  Serial.print(confidence);
  Serial.println("/100");

  // Log the fall event and flush immediately so it reaches the server ASAP
  logManager.log(LOG_LEVEL_WARN, LOG_CAT_FALL_DETECTION, "FALL DETECTED",
                 (float)confidence, (float)HIGH_CONFIDENCE_THRESHOLD);
  logManager.flushImmediate();

  // Prepare emergency data
  EmergencyData_t emergencyData;
  emergencyData.timestamp = millis();
  emergencyData.confidence = confidenceScorer.getConfidenceLevel();
  emergencyData.confidence_score = confidence;
  emergencyData.battery_level = readBatteryLevel();
  emergencyData.sos_triggered = false;
  strncpy(emergencyData.device_id, deviceID, sizeof(emergencyData.device_id));

  // Copy sensor history from fall detector ring buffer
  uint8_t histCount = fallDetector.getHistoryCount();
  if (histCount > 0) {
    memcpy(emergencyData.sensor_history, fallDetector.getSensorHistory(),
           histCount * sizeof(SensorData_t));
  }

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

  // Start non-blocking countdown — loop() will handle expiry and SOS checks
  Serial.println("\n--- Countdown: Press SOS to confirm or wait to cancel ---");
  alertCountdownEnd = millis() + ((uint32_t)COUNTDOWN_DURATION_S * 1000);
  countdownActive = true;
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

  Serial.print("FSR4: ");
  Serial.println(currentSensorData.fsr_values[3]);

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

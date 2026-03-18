#ifndef CONFIG_H
#define CONFIG_H

// System configuration constants
#define SENSOR_SAMPLE_RATE_HZ 100
#define DETECTION_WINDOW_MS 10000
#define ALERT_TIMEOUT_MS 30000
#define BATTERY_LOW_THRESHOLD 3.3f

// Algorithm thresholds
#define FREEFALL_THRESHOLD_G 0.5f
#define IMPACT_THRESHOLD_G 3.0f
#define ROTATION_THRESHOLD_DPS 250.0f
#define INACTIVITY_THRESHOLD_MS 2000

// Pin Definitions (ESP32 Feather V2 / ESP32 HUZZAH32 Feather)
// I2C Bus - Auto-detected based on board type:
//   ESP32 HUZZAH32: SDA=GPIO 23, SCL=GPIO 22
//   ESP32 Feather V2: SDA=GPIO 22, SCL=GPIO 20
// Note: Actual pins are configured automatically by Board_Config utility
#define MPU6050_SDA_PIN SDA   // I2C Data (auto-detected)
#define MPU6050_SCL_PIN SCL   // I2C Clock (auto-detected)
#define BMP280_SDA_PIN SDA    // I2C Data (shared, auto-detected)
#define BMP280_SCL_PIN SCL    // I2C Clock (shared, auto-detected)
#define FSR_ANALOG_PIN A4     // Force sensor analog input (GPIO 35, ADC1 - WiFi safe)
#define SOS_BUTTON_PIN A8     // SOS button with pull-up (GPIO 15)
#define SPEAKER_PIN A12       // Audio alert output (GPIO 13, PWM capable)
#define HAPTIC_PIN A0         // Haptic motor control (GPIO 26, DAC2)
#define VISUAL_ALERT_PIN A10  // Visual alert LED (GPIO 27)
#define BATTERY_SENSE_PIN A13 // Battery voltage monitoring (GPIO 35, BATT_MONITOR)

// MAX30102 Heart Rate Sensor Communication Configuration
// To use UART mode: Uncomment the line below
// To use I2C mode (default): Keep the line commented
// #define MAX30102_USE_UART

#ifdef MAX30102_USE_UART
  // UART Configuration (Modbus RTU)
  #define MAX30102_UART_RX_PIN 16        // GPIO 16 (Serial1 RX)
  #define MAX30102_UART_TX_PIN 17        // GPIO 17 (Serial1 TX)
  #define MAX30102_UART_BAUD 9600        // Default baud rate
#else
  // I2C Configuration (default)
  #define MAX30102_I2C_ADDRESS 0x57
#endif

// WiFi Configuration
#define WIFI_SSID "Mohammed network"
#define WIFI_PASSWORD "87654321"
#define WIFI_TIMEOUT_MS 10000
#define WIFI_RECONNECT_INTERVAL_MS 30000       // Interval between reconnect attempts (ms)
#define WIFI_MAX_RECONNECT_ATTEMPTS 5          // Failures before switching to long backoff
#define WIFI_RECONNECT_LONG_INTERVAL_MS 300000 // 5-min backoff after max failures

// Server Configuration
#define SERVER_BASE_URL        "https://smartfall.vercel.app"
#define SERVER_URL             SERVER_BASE_URL  // Base URL used by WiFi_Manager (paths appended via sendJSONToEndpoint)
#define SERVER_HEALTH_URL      SERVER_BASE_URL "/api/health"
#define SENSOR_STREAM_URL      SERVER_BASE_URL "/api/device/sensor-stream"
#define SERVER_PORT            443

// BLE Configuration
#define BLE_DEVICE_NAME "SmartFall"
#define BLE_STREAMING_INTERVAL_MS 1000 // Sensor data streaming rate

// Emergency Alert Configuration
#define EMERGENCY_MAX_RETRIES 3
#define EMERGENCY_RETRY_INTERVAL_MS 5000

// Timing constants
#define MAIN_LOOP_DELAY_MS 10      // 100Hz main loop
#define SENSOR_READ_INTERVAL_MS 10 // 100Hz sensor reading
#define HEARTBEAT_INTERVAL_MS 1000 // Status LED blink
#define SERIAL_BAUD_RATE 115200

// Alert system constants
#define ALERT_BEEP_DURATION_MS 500
#define ALERT_BEEP_INTERVAL_MS 1000
#define HAPTIC_DURATION_MS 5000
#define COUNTDOWN_DURATION_S 30

// Audio Configuration (PAM8302 Amplifier)
#define AUDIO_DEFAULT_VOLUME 80        // 0-100, default volume level
#define AUDIO_PWM_CHANNEL 0            // ESP32 PWM channel for audio
#define AUDIO_PWM_FREQUENCY 5000       // Base PWM frequency (Hz)
#define AUDIO_PWM_RESOLUTION 8         // PWM resolution (bits)
#define AUDIO_ENABLE_VOICE_ALERTS true // Enable voice-like alert sequences

// WiFi / HTTP retry configuration
#define WIFI_CONNECT_MAX_RETRIES    3
#define WIFI_CONNECT_RETRY_DELAY_MS 1000
#define WIFI_HTTP_MAX_RETRIES       3
#define WIFI_HTTP_RETRY_DELAY_MS    500
#define WIFI_HTTP_CONNECT_TIMEOUT_MS 5000

// Sensor retry / reliability configuration
#define I2C_SENSOR_MAX_RETRIES    10   // MPU6050, BMP280, MAX30102
#define I2C_SENSOR_RETRY_DELAY_MS 10
#define SENSOR_STALE_THRESHOLD     3   // Consecutive identical reads before reset (MPU6050, MAX30102)
#define BMP280_STALE_THRESHOLD    100   // BMP280 measurement cycle ~38ms; at 10ms reads, 4 identical reads/cycle is normal
#define FSR_MAX_RETRIES           10
#define FSR_RETRY_DELAY_MS        5
#define FSR_IMPACT_THRESHOLD      500  // ADC counts for impact detection
#define AUDIO_MAX_RETRIES         3
#define AUDIO_RETRY_DELAY_MS      100

// Confidence scoring constants
#define MAX_CONFIDENCE_SCORE 100
#define HIGH_CONFIDENCE_THRESHOLD 76
#define CONFIRMED_THRESHOLD 67
#define POTENTIAL_THRESHOLD 48
#define SUSPICIOUS_THRESHOLD 29

// Buffer sizes
#define SENSOR_HISTORY_SIZE 100 // 10 seconds at 10Hz
#define DEVICE_ID_SIZE 32
#define MESSAGE_BUFFER_SIZE 256

// Remote logging configuration
#define LOG_BATCH_INTERVAL_MS 30000 // Send log batch every 30 seconds
#define LOG_BUFFER_SIZE 30          // Max entries in ring buffer
#define ENABLE_REMOTE_LOGGING true  // Set false to disable sending logs to server

// Debug settings
#define DEBUG_SENSOR_DATA false
#define DEBUG_ALGORITHM_STEPS true
#define DEBUG_COMMUNICATION true

// Test output configuration
#define ENABLE_TEST_SERIAL_OUTPUT false // Set to false for clean console, logs go to files only

#endif // CONFIG_H

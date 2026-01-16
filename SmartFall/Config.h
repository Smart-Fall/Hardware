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
#define ROTATION_THRESHOLD_DPS 150.0f
#define INACTIVITY_THRESHOLD_MS 2000

// Pin Definitions (ESP32 Feather V2)
// I2C Bus
#define MPU6050_SDA_PIN SDA   // I2C Data (GPIO 22)
#define MPU6050_SCL_PIN SCL   // I2C Clock (GPIO 20)
#define BMP280_SDA_PIN SDA    // I2C Data (shared)
#define BMP280_SCL_PIN SCL    // I2C Clock (shared)
#define MAX30102_SDA_PIN SDA  // I2C Data (shared, address 0x57)
#define MAX30102_SCL_PIN SCL  // I2C Clock (shared)
#define MAX30102_RST_PIN A9   // Reset Pin (GPIO 21, MI pin)
#define FSR1_PIN A0           // Force sensor 1 (GPIO 26)
#define FSR2_PIN A1           // Force sensor 2 (GPIO 25)
#define FSR3_PIN A2           // Force sensor 3 (GPIO 34, ADC1 - WiFi safe)
#define FSR4_PIN A3           // Force sensor 4 (GPIO 39)
#define SOS_BUTTON_PIN A8     // SOS button with pull-up (GPIO 15)
#define SPEAKER_PIN A12       // Audio alert output (GPIO 13, PWM capable)
#define VISUAL_ALERT_PIN A10  // Visual alert LED (GPIO 27)
#define BATTERY_SENSE_PIN A13 // Battery voltage monitoring (GPIO 35, BATT_MONITOR)

// WiFi Configuration
#define WIFI_SSID "Mohammed network"
#define WIFI_PASSWORD "87654321"
#define WIFI_TIMEOUT_MS 30000
#define WIFI_RECONNECT_INTERVAL_MS 30000
#define WIFI_MAX_RECONNECT_ATTEMPTS 5

// Server Configuration
#define SERVER_URL "http://10.129.112.75:3000" // Your alert server URL
#define SERVER_PORT 3000

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

// Debug settings
#define DEBUG_SENSOR_DATA false
#define DEBUG_ALGORITHM_STEPS true
#define DEBUG_COMMUNICATION true

// Test output configuration
#define ENABLE_TEST_SERIAL_OUTPUT false // Set to false for clean console, logs go to files only

#endif // CONFIG_H

#ifndef CONFIG_H
#define CONFIG_H

// System configuration constants
#define SENSOR_SAMPLE_RATE_HZ       100
#define DETECTION_WINDOW_MS         10000
#define ALERT_TIMEOUT_MS           30000
#define BATTERY_LOW_THRESHOLD      3.3f

// Algorithm thresholds (tuned for Wokwi simulation)
#define FREEFALL_THRESHOLD_G       0.5f
#define IMPACT_THRESHOLD_G         3.0f
#define ROTATION_THRESHOLD_DPS     250.0f
#define INACTIVITY_THRESHOLD_MS    2000

// Wokwi Pin Definitions (ESP32 DevKit V1)
#define MPU6050_SDA_PIN            21    // I2C Data (simulating BMI-323)
#define MPU6050_SCL_PIN            22    // I2C Clock
#define DHT22_PIN                  4     // DHT22 sensor (simulating BMP-280)
#define MAX30102_SIM_PIN           A0    // Potentiometer (simulating MAX30102)
#define FSR_ANALOG_PIN             A3    // Potentiometer (simulating FSR)
#define SOS_BUTTON_PIN             38    // Push button with internal pull-up
#define SPEAKER_PIN                25    // LED (simulating speaker)
#define HAPTIC_PIN                 26    // LED (simulating haptic motor)
#define VISUAL_ALERT_PIN           27    // LED (simulating visual alert)
#define BATTERY_SENSE_PIN          A6    // Analog pin for battery monitoring

// LCD Display pins (optional)
#define LCD_SDA_PIN                21    // Shared with MPU6050
#define LCD_SCL_PIN                22    // Shared with MPU6050
#define LCD_ADDRESS                0x27  // I2C address for LCD1602

// WiFi Configuration for Wokwi
#define WIFI_SSID                  "Wokwi-GUEST"
#define WIFI_PASSWORD              ""
#define WIFI_TIMEOUT_MS            10000

// Timing constants
#define MAIN_LOOP_DELAY_MS         10    // 100Hz main loop
#define SENSOR_READ_INTERVAL_MS    10    // 100Hz sensor reading
#define HEARTBEAT_INTERVAL_MS      1000  // Status LED blink
#define SERIAL_BAUD_RATE          115200

// Alert system constants
#define ALERT_BEEP_DURATION_MS     500
#define ALERT_BEEP_INTERVAL_MS     1000
#define HAPTIC_DURATION_MS         5000
#define COUNTDOWN_DURATION_S       30

// Confidence scoring constants
#define MAX_CONFIDENCE_SCORE       105
#define HIGH_CONFIDENCE_THRESHOLD  80
#define CONFIRMED_THRESHOLD        70
#define POTENTIAL_THRESHOLD        50
#define SUSPICIOUS_THRESHOLD       30

// Buffer sizes
#define SENSOR_HISTORY_SIZE        100   // 10 seconds at 10Hz
#define DEVICE_ID_SIZE             32
#define MESSAGE_BUFFER_SIZE        256

// Debug settings
#define DEBUG_SENSOR_DATA          false
#define DEBUG_ALGORITHM_STEPS      true
#define DEBUG_COMMUNICATION        true

// Test output configuration
#define ENABLE_TEST_SERIAL_OUTPUT  false  // Set to false for clean console, logs go to files only

#endif // CONFIG_H
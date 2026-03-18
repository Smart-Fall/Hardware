# Configuration Reference

Complete documentation of all configurable parameters in `SmartFall/Config.h`.

## File Location

```
SmartFall/
  └── Config.h          # Central configuration file
```

Edit this file to customize:
- Pin assignments
- Sensor thresholds
- WiFi credentials
- BLE settings
- Audio parameters
- Debug options

## Pin Definitions

### I2C Sensors (Shared Bus)

```cpp
// I2C Bus Pins (ESP32 Feather V2)
#define MPU6050_SDA_PIN  SDA   // GPIO 22 - I2C Data
#define MPU6050_SCL_PIN  SCL   // GPIO 20 - I2C Clock
#define BMP280_SDA_PIN   SDA   // GPIO 22 (shared)
#define BMP280_SCL_PIN   SCL   // GPIO 20 (shared)
#define MAX30102_SDA_PIN SDA   // GPIO 22 (shared)
#define MAX30102_SCL_PIN SCL   // GPIO 20 (shared)
#define MAX30102_RST_PIN A9    // GPIO 21 - Hardware reset
```

**For custom boards**, adjust GPIO numbers based on your wiring.

### Analog Sensors (ADC1 - WiFi Safe)

```cpp
// Force Sensor (only ADC1 pins work with WiFi)
#define FSR1_PIN A0   // GPIO 26 - Force sensor 1
#define FSR2_PIN A1   // GPIO 25 - Force sensor 2
#define FSR3_PIN A2   // GPIO 34 - Force sensor 3 (currently used)
#define FSR4_PIN A3   // GPIO 39 - Force sensor 4

// Battery Monitoring
#define BATTERY_SENSE_PIN A13  // GPIO 35 - Battery voltage input
```

!!! danger "ADC2 WARNING"
    Do NOT use ADC2 pins (GPIO 0, 2, 4, 12-15, 25-27) when WiFi is enabled. These pins become unavailable. Only ADC1 pins (GPIO 34, 35, 36, 39) work reliably with WiFi active.

### Digital I/O

```cpp
// Emergency Button
#define SOS_BUTTON_PIN A8    // GPIO 15 - SOS button (internal pull-up)

// Audio Output
#define SPEAKER_PIN A12      // GPIO 13 - Audio amplifier PWM input
                             // Actually GPIO 25 in wiring, A12 is mapping

// Haptic Feedback (Optional)
#define HAPTIC_PIN A10       // GPIO 27 - Vibration motor control

// Visual Indicator (Optional)
#define VISUAL_ALERT_PIN A10 // GPIO 27 - Status LED (shared with haptic in config)
```

## System Timing Constants

```cpp
// Sampling and Processing
#define SENSOR_SAMPLE_RATE_HZ      100     // 100Hz sampling (10ms intervals)
#define DETECTION_WINDOW_MS        10000   // 10-second analysis window
#define MAIN_LOOP_DELAY_MS         10      // 10ms loop timing (100Hz)
#define SENSOR_READ_INTERVAL_MS    10      // Read sensors every 10ms

// Alerts and Timeouts
#define ALERT_TIMEOUT_MS           30000   // 30-second countdown timer
#define COUNTDOWN_DURATION_S       30      // User response window (seconds)
#define ALERT_BEEP_DURATION_MS     500     // Individual beep length
#define ALERT_BEEP_INTERVAL_MS     1000    // Beep repeat interval
#define HAPTIC_DURATION_MS         5000    // Vibration duration

// Status Indicators
#define HEARTBEAT_INTERVAL_MS      1000    // LED blink interval
#define SERIAL_BAUD_RATE           115200  // Serial monitor baud rate
```

## Fall Detection Thresholds

### Stage 1: Free Fall

```cpp
#define FREEFALL_THRESHOLD_G       0.5f    // Maximum acceleration for free fall (g)
                                           // Typical: 0.5g
                                           // Range: 0.3-0.7g acceptable
                                           // Lower = more sensitive
```

### Stage 2: Impact

```cpp
#define IMPACT_THRESHOLD_G         3.0f    // Minimum acceleration for impact (g)
                                           // Typical: 3.0g (hard floor)
                                           // Range: 2.0-4.0g for different surfaces
                                           // Lower = catch softer impacts
```

### Stage 3: Rotation

```cpp
#define ROTATION_THRESHOLD_DPS     250.0f  // Angular velocity threshold (°/s)
                                           // Typical: 250°/s
                                           // Detects body rotation during fall
```

### Confidence Thresholds

```cpp
#define MAX_CONFIDENCE_SCORE       100     // Maximum score (internal calculation uses 105)
#define HIGH_CONFIDENCE_THRESHOLD  76      // ≥76 points: Immediate alert
#define CONFIRMED_THRESHOLD        67      // 67-75: 5-second delay alert
#define POTENTIAL_THRESHOLD        48      // 48-66: Enhanced monitoring
#define SUSPICIOUS_THRESHOLD       29      // 30-47: Normal monitoring
                                           // <30: No fall detected
```

!!! warning "Threshold Discrepancies"
    Config.h values differ from specification:
    - Code: 76, 67, 48, 29
    - Spec: 80, 70, 50, 30

    **Use Code values above (authoritative)**

### Inactivity Detection

```cpp
#define INACTIVITY_THRESHOLD_MS    2000    // Minimum time motionless after impact (ms)
                                           // Typical: 2000ms (2 seconds)
                                           // Longer = require extended stillness
```

## Power Management

```cpp
#define BATTERY_LOW_THRESHOLD      3.3f    // Low battery warning voltage (V)
                                           // 3.3V on 3.7V LiPo = ~10% capacity

// Experimental features (not yet implemented)
#define ENABLE_DEEP_SLEEP          false   // Deep sleep mode between detection cycles
#define WIFI_POWER_SAVE_ENABLED    false   // Reduce WiFi transmission frequency
```

## Sensor Reliability Configuration

### I2C Sensor Retry Parameters

```cpp
// I2C Communication Retries (MPU6050, BMP280, MAX30102)
#define I2C_SENSOR_MAX_RETRIES     10      // Retry failed I2C reads 10 times
#define I2C_SENSOR_RETRY_DELAY_MS  10      // Wait 10ms between I2C retries
```

### Stale Data Detection

```cpp
// Stale Reading Thresholds
#define SENSOR_STALE_THRESHOLD     3       // Reset sensor after 3 consecutive identical reads
                                           // Applies to: MPU6050, MAX30102
                                           // Detects sensor hang/communication errors

#define BMP280_STALE_THRESHOLD     100     // BMP280 measurement cycle ~38ms
                                           // At 10ms read rate, 4 identical reads/cycle is normal
                                           // Set higher to account for measurement timing
```

### Force Sensor (FSR) Configuration

```cpp
// FSR Retry and Sensitivity
#define FSR_MAX_RETRIES            10      // Retry failed FSR reads 10 times
#define FSR_RETRY_DELAY_MS         5       // Wait 5ms between FSR retries
#define FSR_IMPACT_THRESHOLD       500     // ADC counts for impact detection
                                           // Higher = require stronger pressure
                                           // Typical: 500 counts on 12-bit ADC
```

### Audio System Reliability

```cpp
// Audio Output Retries
#define AUDIO_MAX_RETRIES          3       // Retry failed audio alert transmissions
#define AUDIO_RETRY_DELAY_MS       100     // Wait 100ms between retry attempts
```

## WiFi Configuration

```cpp
// Network Credentials
#define WIFI_SSID                  "Mohammed network"     // Your network name
#define WIFI_PASSWORD              "87654321"             // Your network password

// Connection Timing
#define WIFI_TIMEOUT_MS            10000   // 10-second connection timeout
#define WIFI_RECONNECT_INTERVAL_MS 30000   // Try reconnect every 30 seconds
#define WIFI_MAX_RECONNECT_ATTEMPTS 5      // Switch to long interval after this many failures
#define WIFI_RECONNECT_LONG_INTERVAL_MS 300000 // 5-minute backoff after max reconnection attempts

// WiFi Connection Retry Configuration
#define WIFI_CONNECT_MAX_RETRIES    3      // Retry connection attempts 3 times
#define WIFI_CONNECT_RETRY_DELAY_MS 1000   // Wait 1 second between retries

// WiFi HTTP Request Retry Configuration
#define WIFI_HTTP_MAX_RETRIES       3      // Retry failed HTTP requests 3 times
#define WIFI_HTTP_RETRY_DELAY_MS    500    // Wait 500ms between HTTP retries
#define WIFI_HTTP_CONNECT_TIMEOUT_MS 5000  // 5-second HTTP connection timeout

// Server Configuration
#define SERVER_BASE_URL            "https://smartfall.vercel.app"  // Production server
#define SERVER_URL                 SERVER_BASE_URL                 // Base URL (paths appended)
#define SERVER_PORT                443     // HTTPS port

// Alert Transmission
#define EMERGENCY_MAX_RETRIES      3       // Retry failed transmissions 3 times
#define EMERGENCY_RETRY_INTERVAL_MS 5000   // Wait 5 seconds between retries
```

### Two-Tier Reconnection Strategy

After **5 failed reconnection attempts** (default `WIFI_MAX_RECONNECT_ATTEMPTS`):
- **Normal interval**: 30 seconds (retry frequently while active)
- **Long interval**: 5 minutes (preserve battery during extended outage)

This reduces power consumption when WiFi is unavailable while maintaining responsiveness when connectivity might be restored.

### Updating WiFi Credentials

```cpp
// Example: Home network
#define WIFI_SSID      "MyHomeNetwork"
#define WIFI_PASSWORD  "MySecurePassword"
#define SERVER_URL     "http://192.168.1.100:3000"  // Local server IP

// Example: Work network
#define WIFI_SSID      "CompanyWiFi"
#define WIFI_PASSWORD  "work_password_123"
#define SERVER_URL     "https://smartfall.example.com:443"  // HTTPS for security
```

!!! warning "Security Note"
    If using HTTPS, ensure your server certificate is properly configured. For development, HTTP is acceptable. For production, always use HTTPS with valid certificates.

## BLE Configuration

```cpp
// Device Identity
#define BLE_DEVICE_NAME            "SmartFall"  // Device name shown in scanner apps
                                                 // Change for multiple devices:
                                                 // "SmartFall_1", "SmartFall_Mom", etc.

// Streaming Configuration
#define BLE_STREAMING_INTERVAL_MS  1000  // Send sensor data every 1 second when streaming
                                          // Lower = more frequent updates, higher power use
                                          // Typical: 1000ms (1 Hz)
                                          // Range: 100ms (10Hz) to 5000ms (0.2Hz)
```

## Audio Configuration

### PAM8302 Amplifier Setup

```cpp
// Audio Output Settings
#define AUDIO_DEFAULT_VOLUME       80        // Default volume (0-100%)
                                             // 80 = good balance of loudness and power
                                             // For deaf/hearing impaired: 100
                                             // For quiet environments: 50-70

// PWM Signal Configuration
#define AUDIO_PWM_CHANNEL          0         // ESP32 PWM channel (0-15)
#define AUDIO_PWM_FREQUENCY        5000      // 5 kHz PWM frequency
                                             // Standard for audio synthesis
                                             // Avoid: 1-10 kHz interference with human hearing

#define AUDIO_PWM_RESOLUTION       8         // 8-bit resolution (0-255)
                                             // Provides 256 volume levels

// Feature Flags
#define AUDIO_ENABLE_VOICE_ALERTS  true      // Enable voice-like tone sequences
                                             // false = save CPU/memory
                                             // true = more user-friendly
```

## Remote Logging Configuration

```cpp
// Log Manager Parameters
#define LOG_BATCH_INTERVAL_MS      30000  // Send log batch every 30 seconds
                                          // Longer = fewer network requests, more buffering
                                          // Shorter = more frequent updates, higher overhead

#define LOG_BUFFER_SIZE            30     // Ring buffer capacity (entries)
                                          // Logs are discarded when buffer fills
                                          // Each entry ~128 bytes

#define ENABLE_REMOTE_LOGGING      true   // Enable/disable remote log uploads
                                          // false = logs to Serial only
                                          // true = logs batched to /api/device/logs
```

### Log Levels and Categories

**Log Levels**: DEBUG, INFO, WARN, ERROR

**Log Categories**: SYSTEM, FALL_DETECTION, SENSOR, WIFI, EMERGENCY

Logs include optional numeric context (value, threshold) for algorithmic values.

## Buffer and Memory Configuration

```cpp
// Data Buffers
#define SENSOR_HISTORY_SIZE        100  // Maintain 100 sensor samples
                                        // At 100Hz = 1 second of history
                                        // Increase for longer post-fall analysis
                                        // Decrease to save memory

#define DEVICE_ID_SIZE             32   // Device identifier length (bytes)
#define MESSAGE_BUFFER_SIZE        256  // JSON message buffer size
                                        // Increase if server messages get cut off
```

## Debug Settings

```cpp
// Debug Output Control
#define DEBUG_SENSOR_DATA          false  // Print raw sensor values every 100ms
                                          // true = verbose output, slower performance
                                          // false = production mode

#define DEBUG_ALGORITHM_STEPS      true   // Print detection stage progression
                                          // true = useful for algorithm tuning
                                          // false = cleaner console output

#define DEBUG_COMMUNICATION        true   // Print WiFi/BLE transmission details
                                          // true = diagnose connectivity issues
                                          // false = reduce serial traffic

// Test Mode
#define ENABLE_TEST_SERIAL_OUTPUT  false  // Redirect logs to file instead of serial
                                          // false = output to Serial Monitor
                                          // true = experimental feature
```

### Enabling Debug Output

To see algorithm details in Serial Monitor:

```cpp
#define DEBUG_SENSOR_DATA      true
#define DEBUG_ALGORITHM_STEPS  true
#define DEBUG_COMMUNICATION    true
```

Sample output:
```
[00001234] Sensor: ax=-0.2, ay=0.1, az=-9.5, gx=125, gy=80, gz=-45
[00001254] Stage 1: Free fall detected (duration: 350ms, accel: 0.15g)
[00001405] Stage 2: Impact detected (accel: 4.5g, timing: 0.3s)
[00001500] Confidence Score: 68 pts → CONFIRMED_FALL
[00001510] Sending WiFi alert to 10.129.112.75:3000
[00001600] Alert transmitted successfully
```

## Compilation with Custom Config

### Using Custom Settings Without Editing

If you prefer not to edit the header file directly:

```bash
# Arduino CLI with custom define
arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32_v2 \
  -D DEBUG_ALGORITHM_STEPS=1 \
  -D AUDIO_DEFAULT_VOLUME=60 .

# PlatformIO with build flags in platformio.ini
[env:feather_esp32_v2]
build_flags =
    -D DEBUG_ALGORITHM_STEPS=1
    -D AUDIO_DEFAULT_VOLUME=60
```

## Common Adjustments

### For Elderly Users (More Sensitive)

```cpp
#define FREEFALL_THRESHOLD_G       0.6f   // Increased (catches more false positives)
#define ROTATION_THRESHOLD_DPS     120.0f // Lowered (detect slower falls)
#define HIGH_CONFIDENCE_THRESHOLD  70     // Lowered (faster alerting)
```

### For Young/Athletic Users (Less Sensitive)

```cpp
#define FREEFALL_THRESHOLD_G       0.4f   // Decreased (must be clear free fall)
#define ROTATION_THRESHOLD_DPS     180.0f // Increased (ignore exercise rotation)
#define HIGH_CONFIDENCE_THRESHOLD  85     // Raised (require stronger evidence)
```

### For Outdoor/High Activity Environments

```cpp
#define POTENTIAL_THRESHOLD        35     // Lowered (reduce false alarms during activity)
#define DEBUG_SENSOR_DATA          true   // Monitor what's happening
```

### For Night Monitoring (Maximize Battery Life)

```cpp
#define SENSOR_SAMPLE_RATE_HZ      50     // Reduce sampling (50Hz acceptable)
#define AUDIO_DEFAULT_VOLUME       60     // Lower volume at night
#define WIFI_RECONNECT_INTERVAL_MS 60000  // Try WiFi less frequently
```

## Configuration Backup

Always backup working configurations:

```bash
# Linux/macOS
cp SmartFall/Config.h SmartFall/Config.h.backup

# Windows PowerShell
Copy-Item SmartFall/Config.h SmartFall/Config.h.backup
```

## Next Steps

1. **Sensor Setup**: See [Sensors Documentation](../firmware/sensors.md)
2. **Wiring**: See [Wiring Guide](../hardware/wiring.md)
3. **Testing**: See [Component Tests](../testing/component-tests.md)
4. **Troubleshooting**: See [Troubleshooting Guide](../troubleshooting.md)

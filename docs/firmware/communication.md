# Communication System

WiFi and Bluetooth Low Energy integration for emergency alerts and status updates.

## Architecture Overview

```mermaid
graph LR
    A["Emergency Event"] --> B["Alert Payload"]
    B --> C{Dual Protocol}
    C --> D["WiFi Manager"]
    C --> E["BLE Server"]
    D --> F["HTTP POST<br/>to Server"]
    E --> G["Mobile App<br/>Notification"]
    F --> H["Cloud Alert<br/>Dashboard"]
    G --> H
```

## WiFi Communication

### WiFiManager Class

Handles WiFi connectivity and HTTP emergency alert transmission.

#### Initialization

```cpp
#include "WiFi_Manager.h"

WiFiManager wifi_manager;

// Configure WiFi credentials
wifi_manager.setSSID("YourNetworkName");
wifi_manager.setPassword("YourPassword");

// Initialize
bool success = wifi_manager.begin();

if (success) {
    Serial.println("WiFi connected");
} else {
    Serial.println("WiFi connection failed");
}
```

#### Configuration (Config.h)

```cpp
#define WIFI_SSID                 "YourNetworkName"
#define WIFI_PASSWORD             "YourPassword"
#define WIFI_TIMEOUT_MS           10000      // 10 second connection timeout
#define WIFI_RECONNECT_INTERVAL_MS 30000     // Try reconnect every 30s
#define WIFI_MAX_RECONNECT_ATTEMPTS 5        // After 5 failures, switch to long backoff
#define WIFI_RECONNECT_LONG_INTERVAL_MS 300000 // 5-minute backoff interval

// HTTP Connection & Retry Configuration
#define WIFI_CONNECT_MAX_RETRIES    3        // Retry connection attempts 3 times
#define WIFI_CONNECT_RETRY_DELAY_MS 1000     // Wait 1s between retries
#define WIFI_HTTP_MAX_RETRIES       3        // Retry HTTP requests 3 times
#define WIFI_HTTP_RETRY_DELAY_MS    500      // Wait 500ms between HTTP retries
#define WIFI_HTTP_CONNECT_TIMEOUT_MS 5000    // 5-second HTTP connect timeout

// Production server (both http:// and https:// supported)
#define SERVER_BASE_URL           "https://smartfall.vercel.app"
#define SERVER_URL                SERVER_BASE_URL
#define SERVER_PORT               443

#define EMERGENCY_MAX_RETRIES     3
#define EMERGENCY_RETRY_INTERVAL_MS 5000
```

#### WiFi Reconnection Strategy

SmartFall implements a **two-tier reconnection backoff** to balance responsiveness with battery life:

1. **Normal phase** (first 5 failures): Retry every **30 seconds**
   - Device actively trying to reconnect
   - Suitable when WiFi may be intermittently available

2. **Long backoff phase** (after 5 failures): Retry every **5 minutes**
   - Reduces power consumption during extended outages
   - Suitable when WiFi is unavailable for prolonged periods

This strategy automatically engages after `WIFI_MAX_RECONNECT_ATTEMPTS` failures, then resets to normal mode when WiFi is restored.

#### Emergency Alert Transmission

```cpp
// Populated by fall detection pipeline
EmergencyData_t emergency;
emergency.confidence_score = confidenceScorer.getTotalScore();   // 0-100
emergency.confidence          = confidenceScorer.getConfidenceLevel(); // enum
emergency.battery_level       = getBatteryPercentage();
emergency.sos_triggered       = (digitalRead(SOS_BUTTON_PIN) == LOW);
strncpy(emergency.device_id, deviceID, sizeof(emergency.device_id));

// Emergency_Comms maps enum → "HIGH" / "CONFIRMED" etc. and POSTs to /api/falls
bool success = emergencyComms.sendEmergencyAlert(emergency);

if (!success) {
    Serial.println("Emergency transmission failed, retrying...");
    // Automatic retry logic in Emergency_Comms
}
```

#### Connection Management

```cpp
// Check WiFi status
if (wifi_manager.isConnected()) {
    Serial.println("Connected to WiFi");
} else {
    Serial.println("WiFi disconnected");
}

// Get signal strength
int8_t rssi = wifi_manager.getSignalStrength();  // dBm
// -30 to -90 dBm (higher is better)

// Reconnect manually
wifi_manager.reconnect();

// Disconnect
wifi_manager.disconnect();
```

### HTTP Endpoints

Three separate endpoints receive data from the device. All timestamps are generated
server-side — no timestamp field is sent from the hardware.

#### POST /api/falls

Fall alert payload:

```
POST /api/falls HTTP/1.1
Host: smartfall.vercel.app
Content-Type: application/json
```

```json
{
  "device_id":        "SF-AABBCCDDEEFF",
  "timestamp":        1708809600000,
  "confidence_score": 85,
  "confidence_level": "HIGH",
  "sos_triggered":    false,
  "battery_level":    78.5,
  "sensor_data": {
    "accel_x":  -0.42,
    "accel_y":   0.15,
    "accel_z":   4.82,
    "gyro_x":    125.3,
    "gyro_y":    -98.7,
    "gyro_z":    45.2
  }
}
```

**Fields:**
- `confidence_level`: String (`NO_FALL`, `SUSPICIOUS`, `POTENTIAL`, `CONFIRMED`, `HIGH`)
- `sensor_data`: Snapshot of sensor readings at moment of detection

**Response:**
```json
{ "success": true }
```

#### POST /api/device/status

Periodic heartbeat (every 60 seconds):

```json
{
  "device_id":     "SF-AABBCCDDEEFF",
  "battery_level": 78.5,
  "system_health": true,
  "uptime":        123456
}
```

#### POST /api/device/sensor-stream

Sensor readings sent every 5 seconds:

```json
{
  "device_id":  "SF-AABBCCDDEEFF",
  "accel_x":    -0.02,
  "accel_y":     0.01,
  "accel_z":     1.00,
  "gyro_x":      1.20,
  "gyro_y":     -0.50,
  "gyro_z":      0.80,
  "pressure":    101325.00,
  "fsr":         350,
  "heart_rate":  72,
  "spo2":        98
}
```

**Fields:**
- `accel_*`, `gyro_*`: IMU readings (g, °/s)
- `pressure`: Barometric pressure (Pa)
- `fsr`: Force sensor reading (ADC counts)
- `heart_rate`: Pulse rate (BPM, from MAX30102)
- `spo2`: Blood oxygen saturation (%, from MAX30102)

Unknown devices are auto-registered on first sensor-stream POST.

## Bluetooth Low Energy (BLE)

### BLE Server Architecture

```mermaid
graph TD
    A["SmartFall BLE Service<br/>UUID: 4fafc201-1fb5-459e-8fcc-c5c9c331914b"]

    B["Emergency Alert<br/>Notify only"]
    C["Sensor Data<br/>Notify only"]
    D["Status<br/>Read + Notify"]
    E["Command<br/>Write only"]
    F["Config<br/>Read + Write"]

    A --> B
    A --> C
    A --> D
    A --> E
    A --> F

    style A fill:#2563eb,color:#fff
    style B fill:#dc2626,color:#fff
    style C fill:#3b82f6,color:#fff
    style D fill:#8b5cf6,color:#fff
    style E fill:#f97316,color:#fff
    style F fill:#d946ef,color:#fff
```

### SmartFallBLEServer Class

```cpp
#include "BLE_Server.h"

SmartFallBLEServer ble_server;

// Initialize with device name
bool success = ble_server.begin("SmartFall");

// Register callbacks
ble_server.onConnect([]() {
    Serial.println("BLE: Device connected");
});

ble_server.onDisconnect([]() {
    Serial.println("BLE: Device disconnected");
});

ble_server.onCommand([](uint8_t cmd, uint8_t *data, size_t len) {
    Serial.print("BLE Command received: 0x");
    Serial.println(cmd, HEX);
});
```

### BLE Characteristics

| Name | UUID | Properties | Purpose |
|------|------|-----------|---------|
| **Emergency Alert** | beb5483e-... | Notify | Emergency notifications |
| **Sensor Data** | beb5483f-... | Notify | Real-time sensor streaming |
| **Status** | beb54840-... | Read, Notify | Device status |
| **Command** | beb54841-... | Write | App commands to device |
| **Config** | beb54842-... | Read, Write | Settings configuration |

### Sending BLE Notifications

```cpp
// Send emergency alert
EmergencyData_t emergency;
// ... populate emergency data ...
ble_server.sendEmergencyAlert(emergency);

// Send sensor data
SensorData_t sensor_data;
// ... populate sensor data ...
ble_server.sendSensorData(sensor_data);

// Send status update
SystemStatus_t status;
status.battery_level = 75;
status.connection_state = CONNECTED;
ble_server.sendStatusUpdate(status);
```

### BLE Commands

Mobile apps send commands via the Command characteristic:

| Command | Code | Payload | Purpose |
|---------|------|---------|---------|
| Cancel Alert | 0x01 | None | Stop active alert |
| Test Alert | 0x02 | Duration (1 byte) | Test alert system |
| Get Status | 0x03 | None | Request full status |
| Set Config | 0x04 | See below | Update settings |
| Start Streaming | 0x05 | Interval (2 bytes) | Enable sensor stream |
| Stop Streaming | 0x06 | None | Disable sensor stream |

### Streaming Mode

Enable real-time sensor data streaming to mobile app:

```cpp
// Enable streaming
ble_server.enableStreaming(true);
ble_server.setStreamingInterval(1000);  // 1 second updates

// Check if should stream now
if (ble_server.shouldStream()) {
    ble_server.sendSensorData(current_sensor_data);
}

// Disable streaming
ble_server.enableStreaming(false);
```

### Connection Management

```cpp
// Check connection status
if (ble_server.isConnected()) {
    Serial.println("BLE client connected");

    // Send notifications
    ble_server.sendStatusUpdate(status);
} else {
    Serial.println("No BLE client connected");
}

// Start/stop advertising
ble_server.startAdvertising();
// ...
ble_server.stopAdvertising();

// Get device name
String name = ble_server.getDeviceName();
```

## Log Manager

Remote logging system for debugging and monitoring device behavior.

### Overview

The `Log_Manager` class maintains a ring buffer of system events and periodically uploads them to the server. Useful for:
- Diagnosing sensor failures
- Tracking fall detection pipeline performance
- Monitoring WiFi connectivity issues
- Analyzing algorithm behavior in the field

### Initialization

```cpp
#include "Log_Manager.h"

extern Log_Manager logManager;  // Global singleton

// In setup()
logManager.begin(&wifi_manager, device_id);
```

### API

```cpp
// Log a message with optional numeric context
void log(LogLevel_t level, LogCategory_t category, const char* message,
         float value = 0.0f, float threshold = 0.0f);

// Send buffered logs if interval has elapsed
void flush();

// Force immediate send (called on fall detection)
void flushImmediate();

// Check initialization status
bool isReady();
```

### Log Levels and Categories

**Levels:** `LOG_LEVEL_DEBUG`, `LOG_LEVEL_INFO`, `LOG_LEVEL_WARN`, `LOG_LEVEL_ERROR`

**Categories:** `LOG_CAT_SYSTEM`, `LOG_CAT_FALL_DETECTION`, `LOG_CAT_SENSOR`, `LOG_CAT_WIFI`, `LOG_CAT_EMERGENCY`

### Usage Examples

```cpp
// Log sensor errors
logManager.log(LOG_LEVEL_WARN, LOG_CAT_SENSOR,
               "MPU6050 read failed, retrying", 0, 0);

// Log algorithm decisions with values
logManager.log(LOG_LEVEL_INFO, LOG_CAT_FALL_DETECTION,
               "Confidence score borderline",
               68.0,          // actual score
               67.0);         // threshold

// Emergency logging (triggers immediate upload)
logManager.log(LOG_LEVEL_ERROR, LOG_CAT_EMERGENCY, "FALL DETECTED");
logManager.flushImmediate();
```

### Configuration

```cpp
#define LOG_BATCH_INTERVAL_MS  30000  // Send every 30 seconds
#define LOG_BUFFER_SIZE        30     // Ring buffer capacity
#define ENABLE_REMOTE_LOGGING  true   // Enable/disable uploads
```

### Ring Buffer Behavior

- Capacity: **30 entries** (adjustable via `LOG_BUFFER_SIZE`)
- Each entry: ~200 bytes
- When full: Oldest entry discarded, new entry added
- Entries auto-printed to Serial for real-time debugging

### Batch Upload

```cpp
// Called periodically in main loop
logManager.flush();

// Called immediately on fall detection
emergencyComms.sendEmergencyAlert(...);
logManager.flushImmediate();  // Force send buffered logs
```

Uploads JSON to `/api/device/logs` (see WiFi Endpoints section).

## Emergency Communications Class

Coordinates WiFi and BLE transmission:

```cpp
#include "Emergency_Comms.h"

EmergencyComms emergency_comms;

// Initialize both communication systems
emergency_comms.begin();

// Send alert via both protocols
EmergencyData_t alert;
// ... populate alert data ...
emergency_comms.sendAlert(alert);

// Automatic retry logic:
// 1. Send to WiFi
// 2. If WiFi fails, retry 3 times
// 3. Send to BLE (if connected)
// 4. Queue for retry if both fail

// Check transmission status
EmergencyComms::Status status = emergency_comms.getLastStatus();
if (status == EmergencyComms::TRANSMISSION_SUCCESS) {
    Serial.println("Alert sent successfully");
} else if (status == EmergencyComms::TRANSMISSION_PENDING) {
    Serial.println("Alert queued for retry");
} else {
    Serial.println("Alert transmission failed");
}
```

## Dual-Protocol Alert Flow

```mermaid
graph TD
    A["Emergency Event<br/>Triggered"]

    B["WiFi Path"]
    B1["Emergency_Comms<br/>sendEmergencyAlert()"]
    B2["HTTP POST<br/>/api/falls"]
    B3["Retry up to 3x<br/>5s intervals"]
    B4{Success?}
    B5["Status: Success"]
    B6["Status: Failed"]

    C["BLE Path"]
    C1["BLE_Server<br/>sendEmergencyAlert()"]
    C2["Notify Emergency<br/>Characteristic"]
    C3["Mobile app<br/>receives alert"]
    C4{Connected?}
    C5["Status: Success"]
    C6["Status: Queued"]

    D{Result}
    E["TRANSMITTED<br/>At least one succeeded"]
    F["QUEUED<br/>Both failed, retry on WiFi"]

    A --> B
    A --> C

    B --> B1 --> B2 --> B3 --> B4
    B4 -->|Yes| B5
    B4 -->|No| B6

    C --> C1 --> C2 --> C3 --> C4
    C4 -->|Connected| C5
    C4 -->|Not connected| C6

    B5 --> D
    B6 --> D
    C5 --> D
    C6 --> D

    D -->|Min 1 success| E
    D -->|Both failed| F

    style A fill:#dc2626,color:#fff
    style E fill:#16a34a,color:#fff
    style F fill:#f97316,color:#fff
```

## Power Consumption During Communication

| Operation | Power Draw | Duration |
|-----------|-----------|----------|
| WiFi scan | 80 mA | 1-2 seconds |
| WiFi connect | 80 mA | 5-10 seconds |
| HTTP POST | 80 mA | 1-3 seconds |
| BLE advertise | 10-20 mA | Continuous |
| BLE notify | 30-50 mA | <100ms |

## Testing Communication

### WiFi Testing

```cpp
// In test sketch
#include "WiFi_Manager.h"

WiFiManager wifi;
wifi.setSSID("YourSSID");
wifi.setPassword("YourPassword");

if (wifi.begin()) {
    Serial.println("✓ WiFi connected");

    // Test HTTP request
    EmergencyData_t test_alert;
    test_alert.timestamp = millis();
    test_alert.confidence_score = 50;

    if (wifi.sendEmergency(test_alert)) {
        Serial.println("✓ Emergency transmission succeeded");
    } else {
        Serial.println("✗ Emergency transmission failed");
    }
} else {
    Serial.println("✗ WiFi connection failed");
}
```

### BLE Testing

Use a mobile BLE scanner app:
- **iOS**: LightBlue, BLE Scanner
- **Android**: nRF Connect, BLE Scanner

1. Scan for "SmartFall" device
2. Connect to service `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
3. Subscribe to Emergency Alert characteristic
4. Trigger test alert

## Next Steps

1. **API Reference**: See [WiFi Endpoints](../api/wifi-endpoints.md) and [BLE Protocol](../api/ble-protocol.md)
2. **Audio Alerts**: See [Audio System](audio.md)
3. **Testing**: See [Component Tests](../testing/component-tests.md)
4. **Configuration**: See [Config Reference](../configuration/config-reference.md)

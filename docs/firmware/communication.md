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
#define WIFI_TIMEOUT_MS           30000     // 30 second connection timeout
#define WIFI_RECONNECT_INTERVAL_MS 30000    // Try reconnect every 30s
#define WIFI_MAX_RECONNECT_ATTEMPTS 5

#define SERVER_URL                "http://your-server.com"
#define SERVER_PORT               80

#define EMERGENCY_MAX_RETRIES     3
#define EMERGENCY_RETRY_INTERVAL_MS 5000
```

#### Emergency Alert Transmission

```cpp
// Create emergency payload
EmergencyData_t emergency;
emergency.timestamp = millis();
emergency.confidence_score = 80;
emergency.confidence_level = HIGH_CONFIDENCE_FALL;
emergency.battery_level = battery_monitor.getPercentage();
emergency.sos_triggered = false;
emergency.device_id = getDeviceID();

// Send alert to server
bool success = wifi_manager.sendEmergency(emergency);

if (!success) {
    Serial.println("Emergency transmission failed, retrying...");
    // Automatic retry logic in WiFi_Manager
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

### HTTP Endpoint: POST /api/emergency

Emergency alert payload sent to your web server:

**Request Headers:**
```
POST /api/emergency HTTP/1.1
Host: your-server.com
Content-Type: application/json
Content-Length: [payload_length]
```

**Request Body (JSON):**
```json
{
  "timestamp": 1234567890000,
  "confidence_score": 85,
  "confidence_level": 4,
  "battery_level": 78.5,
  "sos_triggered": false,
  "device_id": "SF-AABBCCDDEEFF",
  "location": {
    "latitude": 40.7128,
    "longitude": -74.0060
  },
  "sensor_history": [
    {
      "timestamp": 1234567880000,
      "accel_x": -0.2,
      "accel_y": 0.1,
      "accel_z": -9.5,
      "gyro_x": 120,
      "gyro_y": 80,
      "gyro_z": -45,
      "heart_rate": 95,
      "spo2": 98
    }
    // ... more samples ...
  ]
}
```

**Expected Response:**
```json
{
  "status": "received",
  "alert_id": "unique-alert-identifier",
  "actions": {
    "contact_emergency_services": true,
    "notify_contacts": true
  }
}
```

### HTTP Endpoint: POST /api/status

Periodic status updates (every 60 seconds):

```json
{
  "timestamp": 1234567890000,
  "device_id": "SF-AABBCCDDEEFF",
  "battery_level": 78.5,
  "battery_voltage": 3.8,
  "wifi_signal": -55,
  "ble_connected": false,
  "system_state": "monitoring",
  "last_activity": 5000
}
```

### Example Node.js Server

```javascript
const express = require('express');
const app = express();
app.use(express.json());

app.post('/api/emergency', (req, res) => {
  console.log('Emergency Alert Received:');
  console.log(JSON.stringify(req.body, null, 2));

  // Send SMS/Email notifications to emergency contacts
  const alert = req.body;
  if (alert.confidence_level >= 3) {
    // sendEmergencyNotification(alert);
  }

  // Log to database
  // logAlert(alert);

  res.json({
    status: 'received',
    alert_id: Date.now().toString()
  });
});

app.post('/api/status', (req, res) => {
  // Log device status
  console.log(`Device ${req.body.device_id}: Battery ${req.body.battery_level}%`);
  res.json({ status: 'acknowledged' });
});

app.listen(3000, () => {
  console.log('SmartFall server listening on port 3000');
});
```

## Bluetooth Low Energy (BLE)

### BLE Server Architecture

```
SmartFall BLE Service
├── Service UUID: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
├── Emergency Alert (Notify)
├── Sensor Data (Notify)
├── Status (Read/Notify)
├── Command (Write)
└── Config (Read/Write)
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

```
Emergency Event Triggered
    │
    ├─→ [WiFi Path]
    │   ├─→ WiFi_Manager::sendEmergency()
    │   ├─→ HTTP POST to /api/emergency
    │   ├─→ Retry up to 3 times (5s intervals)
    │   └─→ Status: Success or Failed
    │
    └─→ [BLE Path]
        ├─→ BLE_Server::sendEmergencyAlert()
        ├─→ Notify Emergency Characteristic
        ├─→ Mobile app receives notification
        └─→ Status: Success (if connected) or Queued

    Final State:
    ├─→ At least one channel succeeded: TRANSMITTED
    ├─→ Both channels failed: QUEUED for retry
    └─→ Queue retry on WiFi reconnect
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

# WiFi REST API Endpoints

Complete specification of HTTP endpoints for SmartFall emergency alerts and status updates.

## Base URL Configuration

```cpp
// In Config.h
#define SERVER_URL    "http://10.129.112.75:3000"
#define SERVER_PORT   3000
```

Set `SERVER_URL` to your backend server address.

## Endpoints Overview

| Endpoint | Method | Purpose | Frequency |
|----------|--------|---------|-----------|
| `/api/emergency` | POST | Emergency fall alert | On fall detection |
| `/api/status` | POST | Periodic status update | Every 60 seconds |
| `/api/sensor` | POST | Real-time sensor data | Optional streaming |

## POST /api/emergency

Emergency alert triggered by high confidence fall detection.

### Request

**Headers:**
```
POST /api/emergency HTTP/1.1
Host: your-server.com:3000
Content-Type: application/json
Content-Length: [payload_size]
Connection: close
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
      "timestamp": 1234567800000,
      "accel_x": -0.2,
      "accel_y": 0.1,
      "accel_z": -9.5,
      "gyro_x": 120,
      "gyro_y": 80,
      "gyro_z": -45,
      "pressure_pa": 101325,
      "altitude_m": 0.0,
      "temperature_c": 21.5,
      "heart_rate": 95,
      "spo2": 98,
      "fsr_value": 2100,
      "battery_voltage": 3.85
    }
    // ... more samples (up to 100) ...
  ]
}
```

### Field Descriptions

| Field | Type | Description |
|-------|------|-------------|
| `timestamp` | uint64 | Unix timestamp (milliseconds) |
| `confidence_score` | uint8 | 0-105 score from algorithm |
| `confidence_level` | uint8 | 1-5 (1=suspicious, 5=high confidence) |
| `battery_level` | float | Battery percentage (0-100%) |
| `sos_triggered` | boolean | True if manual SOS button pressed |
| `device_id` | string | Unique device MAC address |
| `location` | object | Optional GPS coordinates |
| `sensor_history` | array | Last 10 seconds of sensor data |

### Response

**Success (200 OK):**
```json
{
  "status": "received",
  "alert_id": "alert_20240224_123456_abc",
  "actions": {
    "contact_emergency_services": true,
    "notify_contacts": true,
    "log_event": true
  }
}
```

**Error (4xx/5xx):**
```json
{
  "status": "error",
  "message": "Invalid payload",
  "error_code": 400
}
```

### Transmission Logic

```cpp
// Automatic retry on failure
if (!sendToServer(emergency_data)) {
    // Retry up to 3 times, 5 seconds apart
    for (int i = 0; i < 3; i++) {
        delay(5000);
        if (sendToServer(emergency_data)) {
            break;  // Success
        }
    }
}
```

## POST /api/status

Periodic status updates (every 60 seconds during normal operation).

### Request

**Headers:**
```
POST /api/status HTTP/1.1
Host: your-server.com:3000
Content-Type: application/json
```

**Request Body:**
```json
{
  "timestamp": 1234567890000,
  "device_id": "SF-AABBCCDDEEFF",
  "battery_level": 78.5,
  "battery_voltage": 3.85,
  "wifi_signal": -55,
  "ble_connected": false,
  "system_state": "monitoring",
  "uptime_seconds": 3600,
  "last_fall_detection": 1234567200000
}
```

### Field Descriptions

| Field | Type | Description |
|-------|------|-------------|
| `timestamp` | uint64 | Current time (ms) |
| `device_id` | string | Device identifier |
| `battery_level` | float | 0-100% |
| `battery_voltage` | float | Actual voltage in Volts |
| `wifi_signal` | int8 | RSSI signal strength (dBm) |
| `ble_connected` | boolean | Is BLE client connected |
| `system_state` | string | "monitoring", "alert", "testing" |
| `uptime_seconds` | uint32 | Seconds since boot |
| `last_fall_detection` | uint64 | Timestamp of last fall (0 if none) |

### Response

**Success:**
```json
{
  "status": "acknowledged",
  "server_time": 1234567890000
}
```

## POST /api/sensor

Optional real-time sensor data streaming (if enabled on device).

### Request

Sent every 1 second when streaming is enabled:

```json
{
  "device_id": "SF-AABBCCDDEEFF",
  "timestamp": 1234567890000,
  "accel_x": -0.2,
  "accel_y": 0.1,
  "accel_z": -9.5,
  "gyro_x": 120,
  "gyro_y": 80,
  "gyro_z": -45,
  "pressure_pa": 101325,
  "temperature_c": 21.5,
  "heart_rate": 95,
  "spo2": 98,
  "fsr_value": 2100,
  "battery_voltage": 3.85
}
```

### Control via BLE Command

```cpp
// Mobile app sends command:
BLE_CMD_START_STREAMING  (0x05)  // Enable 1Hz sensor streaming
BLE_CMD_STOP_STREAMING   (0x06)  // Disable streaming
```

## Example Node.js/Express Server

Complete backend implementation:

```javascript
const express = require('express');
const app = express();
app.use(express.json());

// Store active alerts and devices
const alerts = {};
const devices = {};

// Emergency Alert Endpoint
app.post('/api/emergency', (req, res) => {
  const alert = req.body;
  const alertId = Date.now().toString();

  console.log('🚨 EMERGENCY ALERT RECEIVED:');
  console.log(`Device: ${alert.device_id}`);
  console.log(`Confidence: ${alert.confidence_score}/105 (Level: ${alert.confidence_level})`);
  console.log(`Battery: ${alert.battery_level}%`);

  // Store alert
  alerts[alertId] = {
    ...alert,
    received_at: new Date(),
    status: 'pending'
  };

  // Send to emergency contacts (example)
  if (alert.confidence_level >= 3) {
    sendEmergencyNotification(alert);
    callEmergencyServices(alert);
  }

  // Log to database
  logAlertToDatabase(alert);

  // Send response
  res.json({
    status: 'received',
    alert_id: alertId,
    actions: {
      contact_emergency_services: alert.confidence_level >= 4,
      notify_contacts: true,
      log_event: true
    }
  });
});

// Status Update Endpoint
app.post('/api/status', (req, res) => {
  const status = req.body;
  devices[status.device_id] = status;

  console.log(`✓ Status from ${status.device_id}: Battery ${status.battery_level}%, Signal ${status.wifi_signal} dBm`);

  // Check battery low condition
  if (status.battery_level < 20) {
    console.log('⚠️  Low battery warning for device ' + status.device_id);
    // Send notification to user
    notifyUserOfLowBattery(status.device_id);
  }

  res.json({
    status: 'acknowledged',
    server_time: Date.now()
  });
});

// Real-time Sensor Streaming
app.post('/api/sensor', (req, res) => {
  const data = req.body;
  // Store in time-series database (InfluxDB, Prometheus, etc.)
  // storeInTimeSeries(data);
  res.json({ status: 'ok' });
});

// Alert Query Endpoint (for mobile app)
app.get('/api/alerts/:device_id', (req, res) => {
  const alerts_for_device = Object.values(alerts).filter(
    a => a.device_id === req.params.device_id
  );
  res.json(alerts_for_device);
});

// Start server
app.listen(3000, () => {
  console.log('SmartFall server running on port 3000');
});

// Helper functions
async function sendEmergencyNotification(alert) {
  // Send SMS/Email to emergency contacts
  console.log(`Sending notifications for alert ${alert.device_id}...`);
}

async function callEmergencyServices(alert) {
  console.log(`Calling emergency services for ${alert.device_id}...`);
}

function logAlertToDatabase(alert) {
  // Save to persistent storage
  console.log(`Logging alert to database...`);
}

function notifyUserOfLowBattery(device_id) {
  console.log(`Notifying user about low battery for ${device_id}...`);
}
```

## Error Handling

If server is unreachable, SmartFall automatically:
1. Retries up to 3 times (5-second intervals)
2. Falls back to BLE transmission
3. Queues alert for retry on WiFi reconnect

## HTTP Status Codes

| Code | Meaning | Action |
|------|---------|--------|
| **200** | OK | Alert received successfully |
| **400** | Bad Request | Invalid JSON format |
| **401** | Unauthorized | Authentication required |
| **403** | Forbidden | Access denied |
| **500** | Server Error | Retry transmission |
| **503** | Unavailable | Retry transmission |

## HTTPS Support

For production, use HTTPS:

```cpp
#define SERVER_URL "https://smartfall.example.com"
```

Requires valid SSL certificate on server.

## Testing with cURL

```bash
# Test emergency endpoint
curl -X POST http://localhost:3000/api/emergency \
  -H "Content-Type: application/json" \
  -d '{
    "timestamp": 1234567890000,
    "confidence_score": 80,
    "confidence_level": 4,
    "battery_level": 85,
    "sos_triggered": false,
    "device_id": "SF-AABBCCDDEEFF",
    "sensor_history": []
  }'

# Test status endpoint
curl -X POST http://localhost:3000/api/status \
  -H "Content-Type: application/json" \
  -d '{
    "timestamp": 1234567890000,
    "device_id": "SF-AABBCCDDEEFF",
    "battery_level": 85,
    "wifi_signal": -45
  }'
```

## Next Steps

1. **BLE Protocol**: See [BLE Protocol](ble-protocol.md)
2. **Server Setup**: Configure your backend server
3. **Testing**: Use curl or Postman to test endpoints
4. **Deployment**: Deploy to cloud (AWS, Azure, Google Cloud)

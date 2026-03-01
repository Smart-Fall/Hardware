# WiFi REST API Endpoints

Complete specification of HTTP endpoints for SmartFall emergency alerts and status updates.

## Base URL Configuration

```cpp
// In Config.h
#define SERVER_URL    "http://10.129.112.75:3000"
#define SERVER_PORT   3000
```

Set `SERVER_URL` to your backend server address. Both `http://` and `https://` schemes are
supported — the firmware auto-selects a plain or TLS-secured connection based on the URL prefix.

## Endpoints Overview

| Endpoint | Method | Purpose | Frequency |
|----------|--------|---------|-----------|
| `/api/falls` | POST | Emergency fall alert | On fall detection |
| `/api/device/status` | POST | Periodic status update | Every 60 seconds |
| `/api/device/sensor-stream` | POST | Real-time sensor data | Every 5 seconds |

## POST /api/falls

Emergency alert triggered by high-confidence fall detection. The server records the
fall event and timestamps it server-side for accurate wall-clock time.

### Request

**Headers:**
```
POST /api/falls HTTP/1.1
Host: your-server.com:3000
Content-Type: application/json
Content-Length: [payload_size]
Connection: close
```

**Request Body (JSON):**
```json
{
  "device_id": "SF-AABBCCDDEEFF",
  "confidence_score": 85,
  "confidence_level": "HIGH",
  "sos_triggered": false,
  "battery_level": 75.0
}
```

### Field Descriptions

| Field | Type | Description |
|-------|------|-------------|
| `device_id` | string | Device identifier in `SF-XXXXXXXXXXXX` format |
| `confidence_score` | uint8 | 0–100 score from algorithm |
| `confidence_level` | string | One of: `NO_FALL`, `SUSPICIOUS`, `POTENTIAL`, `HIGH`, `CONFIRMED` |
| `sos_triggered` | boolean | `true` if user manually pressed the SOS button |
| `battery_level` | float | Battery percentage (0–100) |

> **Note:** No timestamp is sent from the hardware. The server records `new Date()` at
> receipt time, avoiding the inaccurate `millis()`-since-boot value the device would otherwise send.

### Confidence Level Mapping

| `confidence_level` | `confidence_score` range | Meaning |
|---|---|---|
| `NO_FALL` | < 20 | No fall detected |
| `SUSPICIOUS` | 20–39 | Possible unusual motion |
| `POTENTIAL` | 40–59 | Possible fall, needs confirmation |
| `HIGH` | 60–79 | High likelihood fall |
| `CONFIRMED` | 80–100 | Fall confirmed by all stages |

### Response

**Success (200 OK):**
```json
{
  "success": true
}
```

**Error (4xx/5xx):**
```json
{
  "error": "Internal server error"
}
```

### Retry Logic

```cpp
// Automatic retry on failure
for (int i = 0; i < EMERGENCY_MAX_RETRIES; i++) {
    if (wifi_manager.sendJSONToEndpoint("/api/falls", json)) break;
    delay(EMERGENCY_RETRY_INTERVAL_MS);  // default 5 000 ms
}
```

---

## POST /api/device/status

Periodic status updates sent every 60 seconds during normal operation.

### Request

**Request Body:**
```json
{
  "device_id": "SF-AABBCCDDEEFF",
  "battery_level": 75.0,
  "system_health": true,
  "uptime": 123456
}
```

### Field Descriptions

| Field | Type | Description |
|-------|------|-------------|
| `device_id` | string | Device identifier |
| `battery_level` | float | 0–100% |
| `system_health` | boolean | `true` if all sensors initialized correctly |
| `uptime` | uint32 | Milliseconds since boot |

### Response

**Success:**
```json
{
  "success": true
}
```

---

## POST /api/device/sensor-stream

Continuous sensor data sent every 5 seconds. The backend uses this to populate
historical graphs and update device `lastSeen`. Unknown devices are auto-registered.

### Request

```json
{
  "device_id": "SF-AABBCCDDEEFF",
  "accel_x": -0.02,
  "accel_y":  0.01,
  "accel_z":  1.00,
  "gyro_x":   1.20,
  "gyro_y":  -0.50,
  "gyro_z":   0.80,
  "pressure": 101325.00,
  "heart_rate": 72,
  "spo2": 98
}
```

### Field Descriptions

| Field | Type | Unit | Description |
|-------|------|------|-------------|
| `device_id` | string | — | Device identifier |
| `accel_x/y/z` | float | g | Acceleration (gravity-normalised) |
| `gyro_x/y/z` | float | °/s | Angular velocity |
| `pressure` | float | hPa | Barometric pressure |
| `heart_rate` | uint16 | BPM | Heart rate (0 if sensor unavailable) |
| `spo2` | uint8 | % | Blood oxygen (0 if sensor unavailable) |

> **Auto-registration:** If the backend does not recognise `device_id`, it creates a new
> device record automatically. Link the device to a patient via the signup form using the
> MAC address (the system normalises it to the `SF-XXXXXXXXXXXX` format automatically).

---

## Device ID Format

The firmware generates device IDs from the ESP32 MAC address:

```cpp
// SmartFall.ino — generateDeviceID()
snprintf(deviceID, sizeof(deviceID), "SF-%02X%02X%02X%02X%02X%02X",
         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
// Example: "SF-AABBCCDDEEFF"
```

When registering a patient in the web interface, enter the full MAC address
(e.g., `AA:BB:CC:DD:EE:FF` or `AABBCCDDEEFF`) — the backend normalises it to the
`SF-XXXXXXXXXXXX` format automatically.

---

## HTTPS Support

```cpp
// Config.h — production
#define SERVER_URL "https://smartfall.example.com"
```

The firmware detects the `https://` prefix and establishes a TLS connection using
`WiFiClientSecure`. For development, `http://` uses a plain `WiFiClient`.

---

## Testing with cURL

```bash
# Test fall alert endpoint
curl -X POST http://localhost:3000/api/falls \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "SF-AABBCCDDEEFF",
    "confidence_score": 80,
    "confidence_level": "HIGH",
    "sos_triggered": false,
    "battery_level": 85.0
  }'

# Test status endpoint
curl -X POST http://localhost:3000/api/device/status \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "SF-AABBCCDDEEFF",
    "battery_level": 85.0,
    "system_health": true,
    "uptime": 60000
  }'

# Test sensor stream endpoint
curl -X POST http://localhost:3000/api/device/sensor-stream \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "SF-AABBCCDDEEFF",
    "accel_x": 0.01, "accel_y": 0.0, "accel_z": 1.0,
    "gyro_x": 0.0, "gyro_y": 0.0, "gyro_z": 0.0,
    "pressure": 101325.0,
    "heart_rate": 72,
    "spo2": 98
  }'
```

## HTTP Status Codes

| Code | Meaning | Action |
|------|---------|--------|
| **200** | OK | Data received successfully |
| **400** | Bad Request | Invalid JSON or missing required field |
| **401** | Unauthorized | Authentication required |
| **500** | Server Error | Retry transmission |
| **503** | Unavailable | Retry transmission |

## Next Steps

1. **BLE Protocol**: See [BLE Protocol](ble-protocol.md)
2. **Server Setup**: Configure your backend server
3. **Testing**: Use curl or Postman to test endpoints
4. **Deployment**: Deploy to cloud (AWS, Azure, Google Cloud)

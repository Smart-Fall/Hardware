#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <Arduino.h>

// Sensor data structure
typedef struct {
    float accel_x, accel_y, accel_z;          // Acceleration (g)
    float gyro_x, gyro_y, gyro_z;             // Angular velocity (°/s)
    float pressure;                            // Barometric pressure (hPa)
    float heart_rate;                          // Heart rate (BPM)
    uint16_t fsr_value;                        // FSR reading (ADC counts)
    uint32_t timestamp;                        // Timestamp (ms)
    bool valid;                                // Data validity flag
} SensorData_t;

// Fall detection status
typedef enum {
    FALL_STATUS_MONITORING,
    FALL_STATUS_STAGE1_FREEFALL,
    FALL_STATUS_STAGE2_IMPACT,
    FALL_STATUS_STAGE3_ROTATION,
    FALL_STATUS_STAGE4_INACTIVITY,
    FALL_STATUS_POTENTIAL_FALL,
    FALL_STATUS_FALL_DETECTED,
    FALL_STATUS_EMERGENCY_ACTIVE
} FallStatus_t;

// Confidence levels
typedef enum {
    CONFIDENCE_NO_FALL = 0,
    CONFIDENCE_SUSPICIOUS = 1,
    CONFIDENCE_POTENTIAL = 2,
    CONFIDENCE_CONFIRMED = 3,
    CONFIDENCE_HIGH = 4
} FallConfidence_t;

// Emergency data payload
typedef struct {
    uint32_t timestamp;
    FallConfidence_t confidence;
    uint8_t confidence_score;
    SensorData_t sensor_history[100];  // 10-second history at 10Hz
    float battery_level;
    bool sos_triggered;
    char device_id[32];
} EmergencyData_t;

// Detection thresholds structure
typedef struct {
    float freefall_threshold_g;
    float impact_threshold_g;
    float rotation_threshold_dps;
    uint32_t inactivity_threshold_ms;
    float pressure_change_threshold_m;
} DetectionThresholds_t;

// System status structure
typedef struct {
    bool sensors_initialized;
    bool wifi_connected;
    bool bluetooth_connected;
    float battery_percentage;
    FallStatus_t current_status;
    uint32_t uptime_ms;
} SystemStatus_t;

// Voice message types
typedef enum {
    VOICE_FALL_DETECTED,
    VOICE_PRESS_BUTTON,
    VOICE_EMERGENCY_CONFIRMED,
    VOICE_SYSTEM_READY
} VoiceMessage_t;

// Contact list structure
typedef struct {
    char name[32];
    char phone[16];
    char email[64];
    bool enabled;
} Contact_t;

typedef struct {
    Contact_t contacts[5];
    uint8_t count;
} ContactList_t;

// Configuration structure
typedef struct {
    char wifi_ssid[32];
    char wifi_password[64];
    char device_name[32];
    ContactList_t emergency_contacts;
    DetectionThresholds_t thresholds;
    uint8_t alert_volume;
    uint8_t haptic_intensity;
    bool visual_alerts_enabled;
} Config_t;

// Status update data
typedef struct {
    uint32_t timestamp;
    float battery_level;
    bool system_health;
    uint32_t uptime;
    char status_message[64];
} StatusData_t;

#endif // DATA_TYPES_H
#ifndef FALL_DETECTOR_H
#define FALL_DETECTOR_H

#include "Data_Types.h"
#include "Config.h"
#include <Arduino.h>

class FallDetector
{
private:
    // Detection state variables
    FallStatus_t current_status;
    DetectionThresholds_t thresholds;
    bool monitoring_active;

    // Stage timing variables
    uint32_t stage1_start_time;
    uint32_t stage2_start_time;
    uint32_t stage3_start_time;
    uint32_t stage4_start_time;
    uint32_t detection_window_start;
    uint32_t potential_fall_start_time;

    // Stage detection flags
    bool stage1_triggered;
    bool stage2_triggered;
    bool stage3_triggered;
    bool stage4_triggered;
    uint32_t stage1_exit_candidate_start;
    uint32_t stage2_last_above_threshold_time;
    uint32_t stage4_exit_candidate_start;
    uint32_t potential_exit_candidate_start;

    // Sensor data history for analysis
    SensorData_t sensor_history[SENSOR_HISTORY_SIZE];
    uint8_t history_index;
    uint8_t history_count;

    // Pre-fall detection variables
    float freefall_duration;
    float min_acceleration_during_fall;

    // Impact detection variables
    float max_impact_acceleration;
    uint32_t impact_timing;

    // Rotation analysis variables
    float max_angular_velocity;
    float total_orientation_change;
    float min_tilt_angle;
    float max_tilt_angle;

    // Inactivity assessment variables
    uint32_t inactivity_start_time;
    bool position_stable;

public:
    FallDetector();
    ~FallDetector();

    // Core functions
    bool init();
    void processSensorData(SensorData_t &data);
    FallStatus_t getCurrentStatus();
    void resetDetection();
    bool isMonitoring();

    // Configuration functions
    void setThresholds(DetectionThresholds_t &new_thresholds);
    DetectionThresholds_t getThresholds();
    void enableMonitoring();
    void disableMonitoring();

    // Data access functions
    SensorData_t *getSensorHistory();
    uint8_t getHistoryCount();
    float getFreefallDuration();
    float getMinAcceleration();
    float getMaxImpact();
    uint32_t getImpactTiming();
    float getMaxRotation();
    float getOrientationChange();
    float getInactivityDuration();
    bool isPositionStable();

    // Debug functions
    void printStatus();
    void printStageDetails();
    const char *getStatusString(FallStatus_t status);

private:
    // Stage detection functions
    bool checkStage1_FreeFall(SensorData_t &data);
    bool checkStage2_Impact(SensorData_t &data);
    bool checkStage3_Rotation(SensorData_t &data);
    bool checkStage4_Inactivity(SensorData_t &data);
    bool checkPotentialFallPersistence(SensorData_t &data);

    // Analysis helper functions
    float calculateTotalAcceleration(SensorData_t &data);
    float calculateAngularMagnitude(SensorData_t &data);
    float calculateTiltAngle(SensorData_t &data);
    bool isWithinDetectionWindow();
    void addToHistory(SensorData_t &data);
    void resetStageVariables();

    // Timeout and validation functions
    bool checkStageTimeouts();
    void handleDetectionTimeout();
};

#endif // FALL_DETECTOR_H

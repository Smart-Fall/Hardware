#include "Fall_Detector.h"
#include "Log_Manager.h"

FallDetector::FallDetector() : current_status(FALL_STATUS_MONITORING),
                               monitoring_active(false),
                               stage1_start_time(0), stage2_start_time(0),
                               stage3_start_time(0), stage4_start_time(0),
                               detection_window_start(0), potential_fall_start_time(0),
                               stage1_triggered(false), stage2_triggered(false),
                               stage3_triggered(false), stage4_triggered(false),
                               stage1_exit_candidate_start(0), stage2_last_above_threshold_time(0),
                               history_index(0), history_count(0),
                               freefall_duration(0), min_acceleration_during_fall(10.0f),
                               max_impact_acceleration(0), impact_timing(0),
                               max_angular_velocity(0), total_orientation_change(0),
                               min_tilt_angle(-1.0f), max_tilt_angle(-1.0f),
                               inactivity_start_time(0), position_stable(false)
{

    // Initialize thresholds with default values
    thresholds.freefall_threshold_g = FREEFALL_THRESHOLD_G;
    thresholds.impact_threshold_g = IMPACT_THRESHOLD_G;
    thresholds.rotation_threshold_dps = ROTATION_THRESHOLD_DPS;
    thresholds.inactivity_threshold_ms = INACTIVITY_THRESHOLD_MS;
    thresholds.pressure_change_threshold_m = 1.0f;

    // Initialize sensor history
    for (int i = 0; i < SENSOR_HISTORY_SIZE; i++)
    {
        sensor_history[i] = {0};
    }
}

FallDetector::~FallDetector()
{
    // Cleanup if needed
}

bool FallDetector::init()
{
    resetDetection();
    monitoring_active = true;

    Serial.println("Fall Detector initialized successfully");
    Serial.println("=== Detection Thresholds ===");
    Serial.print("Free Fall: < ");
    Serial.print(thresholds.freefall_threshold_g);
    Serial.println(" g");
    Serial.print("Impact: > ");
    Serial.print(thresholds.impact_threshold_g);
    Serial.println(" g");
    Serial.print("Rotation: > ");
    Serial.print(thresholds.rotation_threshold_dps);
    Serial.println(" °/s");
    Serial.print("Inactivity: > ");
    Serial.print(thresholds.inactivity_threshold_ms);
    Serial.println(" ms");
    Serial.println("============================");

    return true;
}

void FallDetector::processSensorData(SensorData_t &data)
{
    if (!monitoring_active || !data.valid)
        return;

    // Add data to history
    addToHistory(data);

    // Check for stage timeouts
    if (checkStageTimeouts())
    {
        return; // Detection reset due to timeout
    }

    // Process detection stages sequentially
    switch (current_status)
    {
    case FALL_STATUS_MONITORING:
        if (checkStage1_FreeFall(data))
        {
            current_status = FALL_STATUS_STAGE1_FREEFALL;
            stage1_start_time = millis();
            detection_window_start = stage1_start_time;
            if (DEBUG_ALGORITHM_STEPS)
            {
                Serial.println("STAGE 1: Free fall detected!");
            }
            if (logManager.isReady())
            {
                logManager.log(LOG_LEVEL_INFO, LOG_CAT_FALL_DETECTION,
                               "Stage 1: Free fall detected",
                               min_acceleration_during_fall,
                               thresholds.freefall_threshold_g);
            }
        }
        // Some real falls do not contain a clean free-fall phase.
        // Allow a direct impact path and still require sustained inactivity
        // before the detector can confirm a fall.
        else if (checkStage2_Impact(data))
        {
            current_status = FALL_STATUS_STAGE2_IMPACT;
            stage2_start_time = millis();
            detection_window_start = stage2_start_time;
            if (DEBUG_ALGORITHM_STEPS)
            {
                Serial.println("STAGE 2: Impact detected (direct path, no free fall)!");
            }
            if (logManager.isReady())
            {
                logManager.log(LOG_LEVEL_INFO, LOG_CAT_FALL_DETECTION,
                               "Stage 2: Impact detected (direct path)",
                               max_impact_acceleration,
                               thresholds.impact_threshold_g);
            }
        }
        break;

    case FALL_STATUS_STAGE1_FREEFALL:
        // Continue monitoring free fall
        checkStage1_FreeFall(data);

        // Check for impact
        if (checkStage2_Impact(data))
        {
            current_status = FALL_STATUS_STAGE2_IMPACT;
            stage2_start_time = millis();
            if (DEBUG_ALGORITHM_STEPS)
            {
                Serial.println("STAGE 2: Impact detected!");
            }
            if (logManager.isReady())
            {
                logManager.log(LOG_LEVEL_INFO, LOG_CAT_FALL_DETECTION,
                               "Stage 2: Impact detected",
                               max_impact_acceleration,
                               thresholds.impact_threshold_g);
            }
        }
        break;

    case FALL_STATUS_STAGE2_IMPACT:
        // Check for rotation during impact
        if (checkStage3_Rotation(data))
        {
            current_status = FALL_STATUS_STAGE3_ROTATION;
            stage3_start_time = millis();
            if (DEBUG_ALGORITHM_STEPS)
            {
                Serial.println("STAGE 3: Rotation detected!");
            }
            if (logManager.isReady())
            {
                logManager.log(LOG_LEVEL_INFO, LOG_CAT_FALL_DETECTION,
                               "Stage 3: Rotation detected",
                               max_angular_velocity,
                               thresholds.rotation_threshold_dps);
            }
        }
        // Stage 3 is optional — a straight drop may produce no rotation.
        // If inactivity is already present, skip directly to Stage 4.
        else if (checkStage4_Inactivity(data))
        {
            current_status = FALL_STATUS_STAGE4_INACTIVITY;
            stage4_start_time = millis();
            inactivity_start_time = stage4_start_time;
            if (DEBUG_ALGORITHM_STEPS)
            {
                Serial.println("STAGE 4: Inactivity detected (rotation skipped)!");
            }
            if (logManager.isReady())
            {
                logManager.log(LOG_LEVEL_INFO, LOG_CAT_FALL_DETECTION,
                               "Stage 4: Inactivity detected (rotation skipped)",
                               0.0f,
                               (float)thresholds.inactivity_threshold_ms);
            }
        }
        break;

    case FALL_STATUS_STAGE3_ROTATION:
        // Continue monitoring rotation
        checkStage3_Rotation(data);

        // Check for inactivity
        if (checkStage4_Inactivity(data))
        {
            current_status = FALL_STATUS_STAGE4_INACTIVITY;
            stage4_start_time = millis();
            inactivity_start_time = stage4_start_time;
            if (DEBUG_ALGORITHM_STEPS)
            {
                Serial.println("STAGE 4: Inactivity detected!");
            }
            if (logManager.isReady())
            {
                logManager.log(LOG_LEVEL_INFO, LOG_CAT_FALL_DETECTION,
                               "Stage 4: Inactivity detected",
                               0.0f,
                               (float)thresholds.inactivity_threshold_ms);
            }
        }
        break;

    case FALL_STATUS_STAGE4_INACTIVITY:
        if (checkStage4_Inactivity(data))
        {
            // Check if inactivity duration is sufficient
            if ((millis() - inactivity_start_time) >= thresholds.inactivity_threshold_ms)
            {
                current_status = FALL_STATUS_POTENTIAL_FALL;
                if (DEBUG_ALGORITHM_STEPS)
                {
                    Serial.println("POTENTIAL FALL: All stages completed!");
                }
                if (logManager.isReady())
                {
                    logManager.log(LOG_LEVEL_WARN, LOG_CAT_FALL_DETECTION,
                                   "Potential fall: all stages completed");
                }
            }
        }
        else
        {
            // User recovered, reset detection
            if (DEBUG_ALGORITHM_STEPS)
            {
                Serial.println("User recovered - resetting detection");
            }
            if (logManager.isReady())
            {
                logManager.log(LOG_LEVEL_INFO, LOG_CAT_FALL_DETECTION,
                               "User recovered - detection reset");
            }
            resetDetection();
        }
        break;

    case FALL_STATUS_POTENTIAL_FALL:
        if (potential_fall_start_time == 0)
        {
            potential_fall_start_time = millis();
        }

        // During potential-fall monitoring window, movement recovery cancels alert.
        if (!checkStage4_Inactivity(data))
        {
            if (DEBUG_ALGORITHM_STEPS)
            {
                Serial.println("Potential fall canceled - recovery movement detected");
            }
            if (logManager.isReady())
            {
                logManager.log(LOG_LEVEL_INFO, LOG_CAT_FALL_DETECTION,
                               "Potential fall canceled due to recovery movement");
            }
            resetDetection();
            break;
        }

        // Promote only after sustained potential-fall monitoring.
        if ((millis() - potential_fall_start_time) >= POTENTIAL_MONITORING_MS)
        {
            current_status = FALL_STATUS_FALL_DETECTED;
            if (DEBUG_ALGORITHM_STEPS)
            {
                Serial.println("FALL DETECTED: potential fall persisted through monitoring window");
            }
            if (logManager.isReady())
            {
                logManager.log(LOG_LEVEL_WARN, LOG_CAT_FALL_DETECTION,
                               "Fall confirmed after potential-fall monitoring window");
            }
        }
        break;

    case FALL_STATUS_FALL_DETECTED:
    case FALL_STATUS_EMERGENCY_ACTIVE:
        // These states are handled by higher-level system
        break;
    }
}

bool FallDetector::checkStage1_FreeFall(SensorData_t &data)
{
    float total_accel = calculateTotalAcceleration(data);

    if (total_accel < thresholds.freefall_threshold_g)
    {
        if (!stage1_triggered)
        {
            stage1_triggered = true;
            stage1_start_time = millis();
            min_acceleration_during_fall = total_accel;
        }

        // Update minimum acceleration during fall
        if (total_accel < min_acceleration_during_fall)
        {
            min_acceleration_during_fall = total_accel;
        }

        // Update fall duration
        freefall_duration = millis() - stage1_start_time;
        stage1_exit_candidate_start = 0;

        return freefall_duration >= FREEFALL_MIN_DURATION_MS;
    }
    else
    {
        // Debounce exits from free fall to avoid noisy one-sample resets.
        if (stage1_triggered)
        {
            if (stage1_exit_candidate_start == 0)
            {
                stage1_exit_candidate_start = millis();
                return false;
            }

            if ((millis() - stage1_exit_candidate_start) < STAGE1_EXIT_DEBOUNCE_MS)
            {
                return false;
            }

            if (freefall_duration >= FREEFALL_MIN_DURATION_MS)
            {
                return true;
            }

            stage1_triggered = false;
            freefall_duration = 0;
            stage1_exit_candidate_start = 0;
        }
    }

    return false;
}

bool FallDetector::checkStage2_Impact(SensorData_t &data)
{
    float total_accel = calculateTotalAcceleration(data);
    bool has_stage1_context = stage1_triggered || (stage1_start_time > 0);

    if (total_accel > thresholds.impact_threshold_g)
    {
        if (!stage2_triggered)
        {
            stage2_triggered = true;
            stage2_start_time = millis();

            // In direct-impact path (no Stage 1), timing constraint does not apply.
            impact_timing = has_stage1_context ? (stage2_start_time - stage1_start_time) : 0;
        }

        stage2_last_above_threshold_time = millis();

        // Update maximum impact acceleration
        if (total_accel > max_impact_acceleration)
        {
            max_impact_acceleration = total_accel;
        }

        // When Stage 1 exists, keep the timing constraint. Otherwise accept direct impact.
        return has_stage1_context ? (impact_timing <= IMPACT_MAX_DELAY_MS) : true;
    }

    // Allow a very short hold to bridge sampling gaps around impact peak.
    if (stage2_triggered && (millis() - stage2_last_above_threshold_time) <= STAGE2_HOLD_MS)
    {
        return has_stage1_context ? (impact_timing <= IMPACT_MAX_DELAY_MS) : true;
    }

    return false;
}

bool FallDetector::checkStage3_Rotation(SensorData_t &data)
{
    float angular_mag = calculateAngularMagnitude(data);

    if (angular_mag > thresholds.rotation_threshold_dps)
    {
        if (!stage3_triggered)
        {
            stage3_triggered = true;
            stage3_start_time = millis();
        }

        // Update maximum angular velocity
        if (angular_mag > max_angular_velocity)
        {
            max_angular_velocity = angular_mag;
        }

        float tilt = calculateTiltAngle(data);
        if (min_tilt_angle < 0.0f || max_tilt_angle < 0.0f)
        {
            min_tilt_angle = tilt;
            max_tilt_angle = tilt;
        }
        else
        {
            if (tilt < min_tilt_angle)
                min_tilt_angle = tilt;
            if (tilt > max_tilt_angle)
                max_tilt_angle = tilt;
        }
        total_orientation_change = max_tilt_angle - min_tilt_angle;

        return true;
    }

    return stage3_triggered; // Return true if already triggered
}

bool FallDetector::checkStage4_Inactivity(SensorData_t &data)
{
    float total_accel = calculateTotalAcceleration(data);
    float angular_mag = calculateAngularMagnitude(data);

    // Check for inactivity: low acceleration (near 1g) and low angular velocity
    bool is_inactive = (total_accel > 0.8f && total_accel < 1.2f) &&
                       (angular_mag < 50.0f);

    if (is_inactive)
    {
        if (!stage4_triggered)
        {
            stage4_triggered = true;
            inactivity_start_time = millis();
        }
        position_stable = true;
        return true;
    }

    if (stage4_triggered)
    {
        // Any significant movement after inactivity indicates recovery.
        if ((total_accel > INACTIVITY_RECOVERY_ACCEL_G) ||
            (angular_mag > INACTIVITY_RECOVERY_ANGULAR_DPS))
        {
            stage4_triggered = false;
            position_stable = false;
            return false;
        }

        // Do not keep Stage 4 latched when inactivity condition is broken.
        stage4_triggered = false;
        position_stable = false;
        return false;
    }

    return false;
}

float FallDetector::calculateTotalAcceleration(SensorData_t &data)
{
    return sqrt(data.accel_x * data.accel_x +
                data.accel_y * data.accel_y +
                data.accel_z * data.accel_z);
}

float FallDetector::calculateAngularMagnitude(SensorData_t &data)
{
    return sqrt(data.gyro_x * data.gyro_x +
                data.gyro_y * data.gyro_y +
                data.gyro_z * data.gyro_z);
}

float FallDetector::calculateTiltAngle(SensorData_t &data)
{
    float xy = sqrt(data.accel_x * data.accel_x + data.accel_y * data.accel_y);
    return atan2f(xy, fabsf(data.accel_z)) * (180.0f / PI);
}

bool FallDetector::isWithinDetectionWindow()
{
    uint32_t allowed_window = DETECTION_WINDOW_MS;
    if (current_status >= FALL_STATUS_STAGE4_INACTIVITY)
    {
        allowed_window += POTENTIAL_MONITORING_MS;
    }
    return (millis() - detection_window_start) <= allowed_window;
}

void FallDetector::addToHistory(SensorData_t &data)
{
    sensor_history[history_index] = data;
    history_index = (history_index + 1) % SENSOR_HISTORY_SIZE;

    if (history_count < SENSOR_HISTORY_SIZE)
    {
        history_count++;
    }
}

void FallDetector::resetStageVariables()
{
    stage1_triggered = false;
    stage2_triggered = false;
    stage3_triggered = false;
    stage4_triggered = false;
    stage1_exit_candidate_start = 0;
    stage2_last_above_threshold_time = 0;
    potential_fall_start_time = 0;

    freefall_duration = 0;
    min_acceleration_during_fall = 10.0f;
    max_impact_acceleration = 0;
    impact_timing = 0;
    max_angular_velocity = 0;
    total_orientation_change = 0;
    min_tilt_angle = -1.0f;
    max_tilt_angle = -1.0f;
    position_stable = false;
    inactivity_start_time = 0;
}

bool FallDetector::checkStageTimeouts()
{
    if (current_status == FALL_STATUS_MONITORING)
        return false;

    if (!isWithinDetectionWindow())
    {
        handleDetectionTimeout();
        return true;
    }

    return false;
}

void FallDetector::handleDetectionTimeout()
{
    Serial.println("Detection timeout - resetting to monitoring");
    if (logManager.isReady())
    {
        logManager.log(LOG_LEVEL_INFO, LOG_CAT_FALL_DETECTION,
                       "Detection window timeout - resetting to monitoring");
    }
    resetDetection();
}

FallStatus_t FallDetector::getCurrentStatus()
{
    return current_status;
}

void FallDetector::resetDetection()
{
    current_status = FALL_STATUS_MONITORING;
    detection_window_start = 0;
    resetStageVariables();
}

bool FallDetector::isMonitoring()
{
    return monitoring_active;
}

void FallDetector::setThresholds(DetectionThresholds_t &new_thresholds)
{
    thresholds = new_thresholds;
    Serial.println("Detection thresholds updated");
}

DetectionThresholds_t FallDetector::getThresholds()
{
    return thresholds;
}

void FallDetector::enableMonitoring()
{
    monitoring_active = true;
    Serial.println("Fall detection monitoring enabled");
}

void FallDetector::disableMonitoring()
{
    monitoring_active = false;
    resetDetection();
    Serial.println("Fall detection monitoring disabled");
}

SensorData_t *FallDetector::getSensorHistory()
{
    return sensor_history;
}

uint8_t FallDetector::getHistoryCount()
{
    return history_count;
}

float FallDetector::getFreefallDuration()
{
    return freefall_duration;
}

float FallDetector::getMinAcceleration()
{
    return min_acceleration_during_fall;
}

float FallDetector::getMaxImpact()
{
    return max_impact_acceleration;
}

uint32_t FallDetector::getImpactTiming()
{
    return impact_timing;
}

float FallDetector::getMaxRotation()
{
    return max_angular_velocity;
}

float FallDetector::getOrientationChange()
{
    return total_orientation_change;
}

float FallDetector::getInactivityDuration()
{
    if (inactivity_start_time == 0)
        return 0;
    return (float)(millis() - inactivity_start_time);
}

bool FallDetector::isPositionStable()
{
    return position_stable;
}

const char *FallDetector::getStatusString(FallStatus_t status)
{
    switch (status)
    {
    case FALL_STATUS_MONITORING:
        return "MONITORING";
    case FALL_STATUS_STAGE1_FREEFALL:
        return "STAGE1_FREEFALL";
    case FALL_STATUS_STAGE2_IMPACT:
        return "STAGE2_IMPACT";
    case FALL_STATUS_STAGE3_ROTATION:
        return "STAGE3_ROTATION";
    case FALL_STATUS_STAGE4_INACTIVITY:
        return "STAGE4_INACTIVITY";
    case FALL_STATUS_POTENTIAL_FALL:
        return "POTENTIAL_FALL";
    case FALL_STATUS_FALL_DETECTED:
        return "FALL_DETECTED";
    case FALL_STATUS_EMERGENCY_ACTIVE:
        return "EMERGENCY_ACTIVE";
    default:
        return "UNKNOWN";
    }
}

void FallDetector::printStatus()
{
    Serial.print("Fall Detector Status: ");
    Serial.println(getStatusString(current_status));
}

void FallDetector::printStageDetails()
{
    Serial.println("=== Fall Detection Stage Details ===");
    Serial.print("Current Status: ");
    Serial.println(getStatusString(current_status));

    if (freefall_duration > 0)
    {
        Serial.print("Free Fall Duration: ");
        Serial.print(freefall_duration);
        Serial.println(" ms");
    }

    if (max_impact_acceleration > 0)
    {
        Serial.print("Max Impact: ");
        Serial.print(max_impact_acceleration);
        Serial.println(" g");
    }

    if (max_angular_velocity > 0)
    {
        Serial.print("Max Rotation: ");
        Serial.print(max_angular_velocity);
        Serial.println(" °/s");
    }

    Serial.println("=====================================");
}

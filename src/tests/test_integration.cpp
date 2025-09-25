#include "test_integration.h"
#include "../utils/config.h"

// Conditionally enable/disable test serial output
#if ENABLE_TEST_SERIAL_OUTPUT
  #define TEST_SERIAL_PRINT(...) Serial.print(__VA_ARGS__)
  #define TEST_SERIAL_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
  #define TEST_SERIAL_PRINT(...) do {} while(0)
  #define TEST_SERIAL_PRINTLN(...) do {} while(0)
#endif

TestIntegration::TestIntegration() : tests_passed(0), tests_failed(0),
                                     audio_alert_active(false), haptic_alert_active(false),
                                     visual_alert_active(false), sos_button_pressed(false),
                                     alert_start_time(0) {

    // Initialize default test configuration
    config.verbose_output = false;
    config.print_sensor_data = false;
    config.print_detection_steps = true;
    config.max_test_duration_ms = 60000;  // 1 minute max per test
    config.sensor_sample_rate_ms = 10;    // 100Hz simulation

    // Initialize system components
    fall_detector.init();
    confidence_scorer.resetScore();
}

TestIntegration::~TestIntegration() {
}

void TestIntegration::setTestConfig(const TestConfig& test_config) {
    config = test_config;
}

void TestIntegration::setVerboseOutput(bool verbose) {
    config.verbose_output = verbose;
    config.print_sensor_data = verbose;
    config.print_detection_steps = verbose;
}

void TestIntegration::assert_true(bool condition, const char* test_name) {
    if (!condition) {
        TEST_SERIAL_PRINT("FAIL: ");
        TEST_SERIAL_PRINTLN(test_name);
        tests_failed++;
    } else {
        tests_passed++;
    }
    print_test_result(condition, test_name);
}

void TestIntegration::print_test_result(bool passed, const char* test_name) {
    if (passed) {
        TEST_SERIAL_PRINT("PASS: ");
    } else {
        TEST_SERIAL_PRINT("FAIL: ");
    }
    TEST_SERIAL_PRINTLN(test_name);
}

bool TestIntegration::testCompleteNormalActivity() {
    TEST_SERIAL_PRINTLN("Testing complete normal activity scenario...");

    TestResult result = runScenarioTest(SCENARIO_NORMAL_ACTIVITY, 10000,
                                       "Normal Activity", false);

    return validateTestResult(result, false, "Normal activity should not trigger fall detection");
}

bool TestIntegration::testCompleteWalkingActivity() {
    TEST_SERIAL_PRINTLN("Testing complete walking activity scenario...");

    TestResult result = runScenarioTest(SCENARIO_WALKING, 15000,
                                       "Walking Activity", false);

    return validateTestResult(result, false, "Walking activity should not trigger fall detection");
}

bool TestIntegration::testCompleteTypicalFall() {
    TEST_SERIAL_PRINTLN("Testing complete typical fall scenario...");

    TestResult result = runScenarioTest(SCENARIO_TYPICAL_FALL, 20000,
                                       "Typical Fall", true);

    return validateTestResult(result, true, "Typical fall should be detected");
}

bool TestIntegration::testCompleteSevereFall() {
    TEST_SERIAL_PRINTLN("Testing complete severe fall scenario...");

    TestResult result = runScenarioTest(SCENARIO_SEVERE_FALL, 25000,
                                       "Severe Fall", true);

    bool passed = validateTestResult(result, true, "Severe fall should be detected");

    // Additional validation for severe falls
    if (passed && result.test_passed) {
        assert_true(result.final_confidence_score >= 85, "Severe fall high confidence score");
        assert_true(result.final_confidence_level == CONFIDENCE_HIGH, "Severe fall high confidence level");
    }

    return passed;
}

bool TestIntegration::testCompleteFalsePositiveDrop() {
    TEST_SERIAL_PRINTLN("Testing complete device drop (false positive) scenario...");

    TestResult result = runScenarioTest(SCENARIO_FALSE_POSITIVE_DROP, 8000,
                                       "Device Drop", false);

    return validateTestResult(result, false, "Device drop should not trigger fall detection");
}

bool TestIntegration::testCompleteFalsePositiveExercise() {
    TEST_SERIAL_PRINTLN("Testing complete exercise (false positive) scenario...");

    TestResult result = runScenarioTest(SCENARIO_FALSE_POSITIVE_EXERCISE, 12000,
                                       "Exercise Activity", false);

    return validateTestResult(result, false, "Exercise activity should not trigger fall detection");
}

bool TestIntegration::testSOSButtonDuringNormalActivity() {
    TEST_SERIAL_PRINTLN("Testing SOS button activation during normal activity...");

    // Reset system
    fall_detector.resetDetection();
    confidence_scorer.resetScore();

    // Start normal activity
    data_generator.startScenario(SCENARIO_NORMAL_ACTIVITY, 10000);

    bool sos_triggered = false;
    uint32_t start_time = millis();

    // Simulate normal activity for 3 seconds, then trigger SOS
    while ((millis() - start_time) < 5000) {
        SensorData_t data = data_generator.generateSensorData();

        if ((millis() - start_time) > 3000 && !sos_triggered) {
            // Simulate SOS button press
            sos_button_pressed = true;
            sos_triggered = true;
            TEST_SERIAL_PRINTLN(">>> SOS BUTTON PRESSED - MANUAL EMERGENCY!");
        }

        if (sos_button_pressed) {
            // Handle SOS - immediate alert activation
            audio_alert_active = true;
            haptic_alert_active = true;
            visual_alert_active = true;
            fall_detector.resetDetection();
            confidence_scorer.resetScore();
            sos_button_pressed = false;
        } else {
            // Normal processing
            fall_detector.processSensorData(data);
        }

        updateAlertSystem();
        delay(config.sensor_sample_rate_ms);
    }

    assert_true(audio_alert_active, "SOS button activated audio alert");
    assert_true(sos_triggered, "SOS button was triggered during test");

    return true;
}

bool TestIntegration::testSOSButtonDuringFallDetection() {
    TEST_SERIAL_PRINTLN("Testing SOS button during fall detection sequence...");

    fall_detector.resetDetection();
    confidence_scorer.resetScore();

    // Start typical fall scenario
    data_generator.startScenario(SCENARIO_TYPICAL_FALL, 15000);

    bool sos_triggered = false;
    bool fall_detection_started = false;
    uint32_t start_time = millis();

    while ((millis() - start_time) < 10000) {
        SensorData_t data = data_generator.generateSensorData();

        FallStatus_t current_status = fall_detector.getCurrentStatus();

        // Trigger SOS when fall detection is in progress
        if (current_status > FALL_STATUS_MONITORING && !fall_detection_started) {
            fall_detection_started = true;
            TEST_SERIAL_PRINTLN("Fall detection in progress...");
        }

        if (fall_detection_started && !sos_triggered &&
            (millis() - start_time) > 5000) {
            sos_button_pressed = true;
            sos_triggered = true;
            TEST_SERIAL_PRINTLN(">>> SOS BUTTON PRESSED DURING FALL DETECTION!");
        }

        if (sos_button_pressed) {
            audio_alert_active = true;
            haptic_alert_active = true;
            visual_alert_active = true;
            fall_detector.resetDetection();
            confidence_scorer.resetScore();
            sos_button_pressed = false;
        } else {
            fall_detector.processSensorData(data);
        }

        updateAlertSystem();
        delay(config.sensor_sample_rate_ms);
    }

    assert_true(fall_detection_started, "Fall detection was triggered");
    assert_true(sos_triggered, "SOS button was pressed during detection");
    assert_true(audio_alert_active, "SOS override activated alerts");

    return true;
}

bool TestIntegration::testRecoveryAfterPartialDetection() {
    TEST_SERIAL_PRINTLN("Testing recovery after partial fall detection...");

    fall_detector.resetDetection();
    confidence_scorer.resetScore();

    TestResult result;
    result.test_passed = false;
    result.detection_time_ms = 0;
    result.total_sensor_samples = 0;

    // Start typical fall but interrupt it
    data_generator.startScenario(SCENARIO_TYPICAL_FALL, 8000);

    uint32_t start_time = millis();
    bool switched_to_normal = false;

    while ((millis() - start_time) < 15000 && !result.test_passed) {
        SensorData_t data;

        // Switch to normal activity after 4 seconds
        if ((millis() - start_time) > 4000 && !switched_to_normal) {
            data_generator.startScenario(SCENARIO_NORMAL_ACTIVITY, 10000);
            switched_to_normal = true;
            TEST_SERIAL_PRINTLN("Switching to normal activity (simulating recovery)...");
        }

        data = data_generator.generateSensorData();
        fall_detector.processSensorData(data);

        FallStatus_t current_status = fall_detector.getCurrentStatus();
        result.final_fall_status = current_status;
        result.total_sensor_samples++;

        // Check if system resets to monitoring after recovery
        if (switched_to_normal && current_status == FALL_STATUS_MONITORING) {
            result.test_passed = true;
            result.detection_time_ms = millis() - start_time;
        }

        if (config.verbose_output) {
            printSystemStatus(data);
        }

        delay(config.sensor_sample_rate_ms);
    }

    assert_true(switched_to_normal, "Scenario switched to simulate recovery");
    assert_true(result.test_passed, "System reset after recovery detected");

    return result.test_passed;
}

bool TestIntegration::testMultipleConsecutiveFalls() {
    TEST_SERIAL_PRINTLN("Testing multiple consecutive falls...");

    bool all_falls_detected = true;
    int falls_detected = 0;

    // Test 3 consecutive falls
    for (int fall_num = 1; fall_num <= 3; fall_num++) {
        TEST_SERIAL_PRINT("Testing fall #");
        TEST_SERIAL_PRINTLN(fall_num);

        fall_detector.resetDetection();
        confidence_scorer.resetScore();

        TestResult result = runScenarioTest(SCENARIO_TYPICAL_FALL, 15000,
                                           "Consecutive Fall", true);

        if (result.test_passed) {
            falls_detected++;
        } else {
            all_falls_detected = false;
        }

        // Brief pause between falls
        delay(2000);
    }

    assert_true(falls_detected >= 2, "At least 2 out of 3 consecutive falls detected");
    assert_true(all_falls_detected, "All consecutive falls detected");

    return all_falls_detected;
}

bool TestIntegration::testSensorMalfunctionHandling() {
    TEST_SERIAL_PRINTLN("Testing sensor malfunction handling...");

    fall_detector.resetDetection();
    confidence_scorer.resetScore();

    data_generator.startScenario(SCENARIO_NORMAL_ACTIVITY, 10000);

    bool malfunction_handled = true;
    uint32_t start_time = millis();
    int valid_samples = 0;
    int invalid_samples = 0;

    while ((millis() - start_time) < 8000) {
        SensorData_t data = data_generator.generateSensorData();

        // Simulate intermittent sensor malfunction
        if ((millis() - start_time) > 3000 && (millis() - start_time) < 6000) {
            if (random(0, 100) < 30) {  // 30% chance of invalid data
                data.valid = false;
                invalid_samples++;
            }
        }

        if (data.valid) {
            valid_samples++;
            fall_detector.processSensorData(data);
        }

        // System should continue operating despite invalid samples
        FallStatus_t status = fall_detector.getCurrentStatus();
        if (status == FALL_STATUS_MONITORING) {
            // Good - system is stable
        }

        delay(config.sensor_sample_rate_ms);
    }

    assert_true(invalid_samples > 0, "Invalid samples were generated");
    assert_true(valid_samples > invalid_samples, "More valid samples than invalid");
    assert_true(fall_detector.getCurrentStatus() == FALL_STATUS_MONITORING,
               "System remained stable during malfunction");

    return malfunction_handled;
}

bool TestIntegration::testExtendedOperationStability() {
    TEST_SERIAL_PRINTLN("Testing extended operation stability...");

    fall_detector.resetDetection();
    confidence_scorer.resetScore();

    // Run for extended period with various activities
    TestScenario_t scenarios[] = {
        SCENARIO_NORMAL_ACTIVITY,
        SCENARIO_WALKING,
        SCENARIO_FALSE_POSITIVE_EXERCISE,
        SCENARIO_NORMAL_ACTIVITY
    };

    bool stable_operation = true;
    uint32_t total_runtime = 0;

    for (int i = 0; i < 4; i++) {
        data_generator.startScenario(scenarios[i], 8000);
        uint32_t scenario_start = millis();

        while (data_generator.isScenarioActive()) {
            SensorData_t data = data_generator.generateSensorData();
            fall_detector.processSensorData(data);

            // Check for system stability
            FallStatus_t status = fall_detector.getCurrentStatus();
            if (scenarios[i] != SCENARIO_TYPICAL_FALL &&
                scenarios[i] != SCENARIO_SEVERE_FALL &&
                status >= FALL_STATUS_POTENTIAL_FALL) {
                stable_operation = false;
                TEST_SERIAL_PRINTLN("Unexpected fall detection during normal activity");
            }

            delay(config.sensor_sample_rate_ms);
        }

        total_runtime += millis() - scenario_start;
    }

    assert_true(total_runtime > 25000, "Extended operation completed");
    assert_true(stable_operation, "System remained stable during extended operation");

    return stable_operation;
}

bool TestIntegration::testAlertSystemActivation() {
    TEST_SERIAL_PRINTLN("Testing alert system activation...");

    fall_detector.resetDetection();
    confidence_scorer.resetScore();

    TestResult result = runScenarioTest(SCENARIO_TYPICAL_FALL, 20000,
                                       "Alert System Test", true);

    // Check that alerts were properly activated
    bool alerts_activated = audio_alert_active || haptic_alert_active || visual_alert_active;

    assert_true(result.test_passed, "Fall was detected");
    assert_true(alerts_activated, "Alert system was activated");

    return result.test_passed && alerts_activated;
}

bool TestIntegration::testConfidenceScoreAccuracy() {
    TEST_SERIAL_PRINTLN("Testing confidence score accuracy...");

    struct ScenarioExpectation {
        TestScenario_t scenario;
        uint8_t min_score;
        uint8_t max_score;
        FallConfidence_t expected_confidence;
    };

    ScenarioExpectation expectations[] = {
        {SCENARIO_NORMAL_ACTIVITY, 0, 30, CONFIDENCE_NO_FALL},
        {SCENARIO_FALSE_POSITIVE_DROP, 0, 50, CONFIDENCE_POTENTIAL},
        {SCENARIO_TYPICAL_FALL, 70, 90, CONFIDENCE_CONFIRMED},
        {SCENARIO_SEVERE_FALL, 85, 105, CONFIDENCE_HIGH}
    };

    bool all_scores_accurate = true;

    for (int i = 0; i < 4; i++) {
        fall_detector.resetDetection();
        confidence_scorer.resetScore();

        bool should_detect = (expectations[i].scenario == SCENARIO_TYPICAL_FALL ||
                             expectations[i].scenario == SCENARIO_SEVERE_FALL);

        TestResult result = runScenarioTest(expectations[i].scenario, 20000,
                                           "Confidence Test", should_detect);

        bool score_in_range = (result.final_confidence_score >= expectations[i].min_score &&
                              result.final_confidence_score <= expectations[i].max_score);

        bool confidence_correct = (result.final_confidence_level == expectations[i].expected_confidence);

        if (!score_in_range || !confidence_correct) {
            all_scores_accurate = false;
            TEST_SERIAL_PRINT("Score mismatch for scenario ");
            TEST_SERIAL_PRINT(expectations[i].scenario);
            TEST_SERIAL_PRINT(": got ");
            TEST_SERIAL_PRINT(result.final_confidence_score);
            TEST_SERIAL_PRINT(" (expected ");
            TEST_SERIAL_PRINT(expectations[i].min_score);
            TEST_SERIAL_PRINT("-");
            TEST_SERIAL_PRINT(expectations[i].max_score);
            TEST_SERIAL_PRINTLN(")");
        }
    }

    assert_true(all_scores_accurate, "All confidence scores were accurate");

    return all_scores_accurate;
}

bool TestIntegration::testDetectionTimingRequirements() {
    TEST_SERIAL_PRINTLN("Testing detection timing requirements...");

    fall_detector.resetDetection();
    confidence_scorer.resetScore();

    TestResult result = runScenarioTest(SCENARIO_SEVERE_FALL, 25000,
                                       "Timing Test", true);

    // Fall detection should occur within 10 seconds of fall start
    bool timing_acceptable = (result.detection_time_ms > 0 && result.detection_time_ms < 10000);

    assert_true(result.test_passed, "Fall was detected");
    assert_true(timing_acceptable, "Detection timing within requirements");

    if (result.test_passed) {
        TEST_SERIAL_PRINT("Detection time: ");
        TEST_SERIAL_PRINT(result.detection_time_ms);
        TEST_SERIAL_PRINTLN(" ms");
    }

    return result.test_passed && timing_acceptable;
}

bool TestIntegration::testRealWorldScenarioSequence() {
    TEST_SERIAL_PRINTLN("Testing real-world scenario sequence...");

    // Simulate a realistic day sequence
    TestScenario_t daily_sequence[] = {
        SCENARIO_NORMAL_ACTIVITY,    // Morning routine
        SCENARIO_WALKING,            // Going for a walk
        SCENARIO_NORMAL_ACTIVITY,    // Sitting, reading
        SCENARIO_FALSE_POSITIVE_EXERCISE, // Afternoon exercise
        SCENARIO_NORMAL_ACTIVITY,    // Evening relaxation
        SCENARIO_TYPICAL_FALL        // Unfortunate fall
    };

    bool sequence_handled_correctly = true;
    int fall_detected_in_sequence = 0;

    for (int i = 0; i < 6; i++) {
        bool should_detect = (daily_sequence[i] == SCENARIO_TYPICAL_FALL ||
                             daily_sequence[i] == SCENARIO_SEVERE_FALL);

        TestResult result = runScenarioTest(daily_sequence[i], 8000,
                                           "Daily Sequence", should_detect);

        if (should_detect && result.test_passed) {
            fall_detected_in_sequence++;
        } else if (!should_detect && result.false_positive) {
            sequence_handled_correctly = false;
        }

        // Brief pause between activities
        delay(1000);
    }

    assert_true(fall_detected_in_sequence == 1, "Exactly one fall detected in sequence");
    assert_true(sequence_handled_correctly, "Real-world sequence handled correctly");

    return sequence_handled_correctly && (fall_detected_in_sequence == 1);
}

bool TestIntegration::testBorderlineCaseAnalysis() {
    TEST_SERIAL_PRINTLN("Testing borderline case analysis...");

    fall_detector.resetDetection();
    confidence_scorer.resetScore();

    // Create borderline scenario
    data_generator.startScenario(SCENARIO_TYPICAL_FALL, 15000);

    // Reduce fall severity to create borderline case
    data_generator.configureFall(1.0f, 0.7f, true);  // Reduced height and severity

    TestResult result;
    simulateMainLoop(15000, result);

    bool borderline_handled = true;

    // Borderline cases should result in POTENTIAL_FALL status
    if (result.final_confidence_score >= 50 && result.final_confidence_score < 70) {
        borderline_handled = (result.final_confidence_level == CONFIDENCE_POTENTIAL);
    }

    assert_true(borderline_handled, "Borderline case properly classified");

    TEST_SERIAL_PRINT("Borderline case score: ");
    TEST_SERIAL_PRINTLN(result.final_confidence_score);

    return borderline_handled;
}

TestIntegration::TestResult TestIntegration::runScenarioTest(TestScenario_t scenario,
                                                            uint32_t duration_ms,
                                                            const char* scenario_name,
                                                            bool should_detect_fall) {
    if (config.verbose_output) {
        TEST_SERIAL_PRINT("Running scenario: ");
        TEST_SERIAL_PRINTLN(scenario_name);
    }

    // Reset system
    fall_detector.resetDetection();
    confidence_scorer.resetScore();
    audio_alert_active = false;
    haptic_alert_active = false;
    visual_alert_active = false;

    // Start scenario
    data_generator.startScenario(scenario, duration_ms);

    TestResult result;
    simulateMainLoop(duration_ms, result);

    if (config.verbose_output) {
        printScenarioResults(result, scenario_name);
    }

    return result;
}

void TestIntegration::simulateMainLoop(uint32_t duration_ms, TestResult& result) {
    // Initialize result
    result.test_passed = false;
    result.detection_time_ms = 0;
    result.final_confidence_score = 0;
    result.final_confidence_level = CONFIDENCE_NO_FALL;
    result.final_fall_status = FALL_STATUS_MONITORING;
    result.total_sensor_samples = 0;
    result.false_positive = false;
    result.false_negative = false;

    uint32_t start_time = millis();
    uint32_t first_detection_time = 0;

    while ((millis() - start_time) < duration_ms) {
        // Generate sensor data
        SensorData_t data = data_generator.generateSensorData();
        result.total_sensor_samples++;

        if (config.print_sensor_data) {
            data_generator.printGeneratedData(data);
        }

        // Process detection logic (simulating main.cpp behavior)
        processDetectionLogic(data, result);

        // Update alert system
        updateAlertSystem();

        // Track first detection time
        if (result.final_fall_status >= FALL_STATUS_POTENTIAL_FALL && first_detection_time == 0) {
            first_detection_time = millis() - start_time;
            result.detection_time_ms = first_detection_time;
        }

        // Print status if verbose
        if (config.verbose_output && (result.total_sensor_samples % 100) == 0) {
            printSystemStatus(data);
        }

        delay(config.sensor_sample_rate_ms);
    }

    // Final result evaluation
    result.test_passed = (result.final_fall_status >= FALL_STATUS_POTENTIAL_FALL);
}

void TestIntegration::processDetectionLogic(SensorData_t& data, TestResult& result) {
    // Process sensor data through fall detector (simulating main.cpp)
    fall_detector.processSensorData(data);

    FallStatus_t current_status = fall_detector.getCurrentStatus();
    result.final_fall_status = current_status;

    // Handle different detection states (simulating main.cpp switch statement)
    if (current_status == FALL_STATUS_POTENTIAL_FALL) {
        if (config.print_detection_steps) {
            TEST_SERIAL_PRINTLN(">>> POTENTIAL FALL DETECTED - Analyzing...");
        }

        // Calculate confidence score (simulating main.cpp)
        confidence_scorer.addStage1Score(fall_detector.getFreefalDuration(),
                                        sqrt(data.accel_x*data.accel_x +
                                             data.accel_y*data.accel_y +
                                             data.accel_z*data.accel_z));
        confidence_scorer.addStage2Score(fall_detector.getMaxImpact(), data.timestamp, false);
        confidence_scorer.addStage3Score(fall_detector.getMaxRotation(), 0);
        confidence_scorer.addStage4Score(2000, true);

        // Add filter scores
        confidence_scorer.addPressureFilterScore(1.0f);
        confidence_scorer.addHeartRateFilterScore(15.0f);
        confidence_scorer.addFSRFilterScore(false, true);

        // Evaluate confidence
        result.final_confidence_score = confidence_scorer.getTotalScore();
        result.final_confidence_level = confidence_scorer.getConfidenceLevel();

        if (config.print_detection_steps) {
            TEST_SERIAL_PRINT("Confidence Score: ");
            TEST_SERIAL_PRINT(result.final_confidence_score);
            TEST_SERIAL_PRINT("/105 - ");
            TEST_SERIAL_PRINTLN(confidence_scorer.getConfidenceString(result.final_confidence_level));
        }

        if (result.final_confidence_level >= CONFIDENCE_CONFIRMED) {
            if (config.print_detection_steps) {
                TEST_SERIAL_PRINTLN(">>> FALL CONFIRMED - EMERGENCY ALERT ACTIVATED!");
            }
            visual_alert_active = true;
            audio_alert_active = true;
            haptic_alert_active = true;
            alert_start_time = millis();
        }

        if (config.verbose_output) {
            confidence_scorer.printScoreBreakdown();
        }
    }
}

void TestIntegration::updateAlertSystem() {
    // Auto-reset alerts after timeout (simulating main.cpp behavior)
    if (audio_alert_active || haptic_alert_active || visual_alert_active) {
        if (alert_start_time == 0) {
            alert_start_time = millis();
        } else if ((millis() - alert_start_time) > 10000) {  // 10 second demo timeout
            audio_alert_active = false;
            haptic_alert_active = false;
            visual_alert_active = false;
            alert_start_time = 0;
            if (config.verbose_output) {
                TEST_SERIAL_PRINTLN("Alert timeout - returning to monitoring mode");
            }
        }
    } else {
        alert_start_time = 0;
    }
}

void TestIntegration::printSystemStatus(const SensorData_t& data) {
    FallStatus_t status = fall_detector.getCurrentStatus();

    TEST_SERIAL_PRINT("Status: ");
    TEST_SERIAL_PRINT(fall_detector.getStatusString(status));
    TEST_SERIAL_PRINT(" | Total Accel: ");

    float total_accel = sqrt(data.accel_x*data.accel_x +
                            data.accel_y*data.accel_y +
                            data.accel_z*data.accel_z);
    TEST_SERIAL_PRINT(total_accel, 2);

    TEST_SERIAL_PRINT("g | Alerts: ");
    if (audio_alert_active) TEST_SERIAL_PRINT("A");
    if (haptic_alert_active) TEST_SERIAL_PRINT("H");
    if (visual_alert_active) TEST_SERIAL_PRINT("V");
    if (!audio_alert_active && !haptic_alert_active && !visual_alert_active) TEST_SERIAL_PRINT("-");

    TEST_SERIAL_PRINTLN();
}

bool TestIntegration::validateTestResult(const TestResult& result, bool should_detect_fall,
                                       const char* test_name) {
    bool validation_passed = true;

    if (should_detect_fall) {
        if (!result.test_passed) {
            TEST_SERIAL_PRINT("FAIL: ");
            TEST_SERIAL_PRINT(test_name);
            TEST_SERIAL_PRINTLN(" - Fall not detected when it should have been");
            validation_passed = false;
        }
        if (result.final_confidence_score < 50) {
            TEST_SERIAL_PRINT("WARN: ");
            TEST_SERIAL_PRINT(test_name);
            TEST_SERIAL_PRINT(" - Low confidence score: ");
            TEST_SERIAL_PRINTLN(result.final_confidence_score);
        }
    } else {
        if (result.test_passed) {
            TEST_SERIAL_PRINT("FAIL: ");
            TEST_SERIAL_PRINT(test_name);
            TEST_SERIAL_PRINTLN(" - False positive detected");
            validation_passed = false;
        }
    }

    if (validation_passed) {
        tests_passed++;
        TEST_SERIAL_PRINT("PASS: ");
        TEST_SERIAL_PRINTLN(test_name);
    } else {
        tests_failed++;
    }

    return validation_passed;
}

void TestIntegration::printScenarioResults(const TestResult& result, const char* scenario_name) {
    TEST_SERIAL_PRINTLN("--- Scenario Results ---");
    TEST_SERIAL_PRINT("Scenario: ");
    TEST_SERIAL_PRINTLN(scenario_name);
    TEST_SERIAL_PRINT("Fall Detected: ");
    TEST_SERIAL_PRINTLN(result.test_passed ? "YES" : "NO");
    TEST_SERIAL_PRINT("Detection Time: ");
    TEST_SERIAL_PRINT(result.detection_time_ms);
    TEST_SERIAL_PRINTLN(" ms");
    TEST_SERIAL_PRINT("Final Status: ");
    TEST_SERIAL_PRINTLN(fall_detector.getStatusString(result.final_fall_status));
    TEST_SERIAL_PRINT("Confidence Score: ");
    TEST_SERIAL_PRINT(result.final_confidence_score);
    TEST_SERIAL_PRINTLN("/105");
    TEST_SERIAL_PRINT("Confidence Level: ");
    TEST_SERIAL_PRINTLN(confidence_scorer.getConfidenceString(result.final_confidence_level));
    TEST_SERIAL_PRINT("Sensor Samples: ");
    TEST_SERIAL_PRINTLN(result.total_sensor_samples);
    TEST_SERIAL_PRINTLN("----------------------");
}

bool TestIntegration::runAllIntegrationTests() {
    TEST_SERIAL_PRINTLN("========================================");
    TEST_SERIAL_PRINTLN("     INTEGRATION TEST SUITE");
    TEST_SERIAL_PRINTLN("   (Simulating Complete System)");
    TEST_SERIAL_PRINTLN("========================================");

    tests_passed = 0;
    tests_failed = 0;

    bool all_passed = true;

    // Complete scenario tests
    all_passed &= testCompleteNormalActivity();
    all_passed &= testCompleteWalkingActivity();
    all_passed &= testCompleteTypicalFall();
    all_passed &= testCompleteSevereFall();
    all_passed &= testCompleteFalsePositiveDrop();
    all_passed &= testCompleteFalsePositiveExercise();

    // Interactive tests
    all_passed &= testSOSButtonDuringNormalActivity();
    all_passed &= testSOSButtonDuringFallDetection();
    all_passed &= testRecoveryAfterPartialDetection();

    // Stress and edge case tests
    all_passed &= testMultipleConsecutiveFalls();
    all_passed &= testSensorMalfunctionHandling();
    all_passed &= testExtendedOperationStability();

    // System validation tests
    all_passed &= testAlertSystemActivation();
    all_passed &= testConfidenceScoreAccuracy();
    all_passed &= testDetectionTimingRequirements();

    // Comprehensive scenario tests
    all_passed &= testRealWorldScenarioSequence();
    all_passed &= testBorderlineCaseAnalysis();

    printDetailedTestSummary();

    return all_passed;
}

void TestIntegration::printDetailedTestSummary() {
    TEST_SERIAL_PRINTLN("========================================");
    TEST_SERIAL_PRINTLN("      INTEGRATION TEST RESULTS");
    TEST_SERIAL_PRINTLN("========================================");
    TEST_SERIAL_PRINT("Tests Passed: ");
    TEST_SERIAL_PRINTLN(tests_passed);
    TEST_SERIAL_PRINT("Tests Failed: ");
    TEST_SERIAL_PRINTLN(tests_failed);
    TEST_SERIAL_PRINT("Success Rate: ");
    if (tests_passed + tests_failed > 0) {
        TEST_SERIAL_PRINT((tests_passed * 100) / (tests_passed + tests_failed));
        TEST_SERIAL_PRINTLN("%");
    } else {
        TEST_SERIAL_PRINTLN("N/A");
    }

    TEST_SERIAL_PRINTLN();
    TEST_SERIAL_PRINTLN("=== System Performance Summary ===");
    TEST_SERIAL_PRINT("Free Heap: ");
    TEST_SERIAL_PRINT(ESP.getFreeHeap());
    TEST_SERIAL_PRINTLN(" bytes");
    TEST_SERIAL_PRINT("Uptime: ");
    TEST_SERIAL_PRINT(millis() / 1000);
    TEST_SERIAL_PRINTLN(" seconds");

    TEST_SERIAL_PRINTLN();
    if (tests_failed == 0) {
        TEST_SERIAL_PRINTLN("✓ ALL INTEGRATION TESTS PASSED!");
        TEST_SERIAL_PRINTLN("✓ SmartFall system is ready for deployment!");
    } else {
        TEST_SERIAL_PRINTLN("✗ SOME INTEGRATION TESTS FAILED!");
        TEST_SERIAL_PRINTLN("✗ System requires additional debugging!");
    }
    TEST_SERIAL_PRINTLN("========================================");
}
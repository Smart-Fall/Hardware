#include "test_fall_detector.h"
#include "../utils/config.h"

// Conditionally enable/disable test serial output
#if ENABLE_TEST_SERIAL_OUTPUT
  #define TEST_SERIAL_PRINT(...) Serial.print(__VA_ARGS__)
  #define TEST_SERIAL_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
  #define TEST_SERIAL_PRINT(...) do {} while(0)
  #define TEST_SERIAL_PRINTLN(...) do {} while(0)
#endif

TestFallDetector::TestFallDetector() : tests_passed(0), tests_failed(0) {
    detector.init();
}

TestFallDetector::~TestFallDetector() {
}

void TestFallDetector::assert_equal(FallStatus_t expected, FallStatus_t actual, const char* test_name) {
    bool passed = (expected == actual);
    if (!passed) {
        TEST_SERIAL_PRINT("FAIL: ");
        TEST_SERIAL_PRINT(test_name);
        TEST_SERIAL_PRINT(" - Expected status: ");
        TEST_SERIAL_PRINT(expected);
        TEST_SERIAL_PRINT(", Got: ");
        TEST_SERIAL_PRINTLN(actual);
        tests_failed++;
    } else {
        tests_passed++;
    }
    print_test_result(passed, test_name);
}

void TestFallDetector::assert_true(bool condition, const char* test_name) {
    if (!condition) {
        TEST_SERIAL_PRINT("FAIL: ");
        TEST_SERIAL_PRINTLN(test_name);
        tests_failed++;
    } else {
        tests_passed++;
    }
    print_test_result(condition, test_name);
}

void TestFallDetector::print_test_result(bool passed, const char* test_name) {
    if (passed) {
        TEST_SERIAL_PRINT("PASS: ");
    } else {
        TEST_SERIAL_PRINT("FAIL: ");
    }
    TEST_SERIAL_PRINTLN(test_name);
}

SensorData_t TestFallDetector::createTestData(float ax, float ay, float az, float gx, float gy, float gz) {
    SensorData_t data = {0};
    data.accel_x = ax;
    data.accel_y = ay;
    data.accel_z = az;
    data.gyro_x = gx;
    data.gyro_y = gy;
    data.gyro_z = gz;
    data.pressure = 1013.25f;
    data.heart_rate = 70.0f;
    data.fsr_value = 100;
    data.timestamp = millis();
    data.valid = true;
    return data;
}

void TestFallDetector::feedDataSequence(SensorData_t* data_array, int count, int delay_ms) {
    for (int i = 0; i < count; i++) {
        data_array[i].timestamp = millis();
        detector.processSensorData(data_array[i]);
        if (delay_ms > 0) {
            delay(delay_ms);
        }
    }
}

bool TestFallDetector::testInitialization() {
    TEST_SERIAL_PRINTLN("Testing fall detector initialization...");

    detector.resetDetection();

    assert_equal(FALL_STATUS_MONITORING, detector.getCurrentStatus(), "Initial status");
    assert_true(detector.isMonitoring(), "Monitoring enabled after init");

    return true;
}

bool TestFallDetector::testResetDetection() {
    TEST_SERIAL_PRINTLN("Testing detection reset functionality...");

    // Put detector in some advanced state
    SensorData_t free_fall_data = createTestData(0.1f, 0.1f, 0.2f, 0, 0, 0);
    for (int i = 0; i < 30; i++) {  // Feed free fall data for 300ms
        detector.processSensorData(free_fall_data);
        delay(10);
    }

    // Should be in free fall detection state
    FallStatus_t status_before_reset = detector.getCurrentStatus();
    assert_true(status_before_reset != FALL_STATUS_MONITORING, "Detector advanced beyond monitoring");

    // Reset should return to monitoring
    detector.resetDetection();
    assert_equal(FALL_STATUS_MONITORING, detector.getCurrentStatus(), "Status after reset");

    return true;
}

bool TestFallDetector::testThresholdConfiguration() {
    TEST_SERIAL_PRINTLN("Testing threshold configuration...");

    DetectionThresholds_t original_thresholds = detector.getThresholds();

    // Modify thresholds
    DetectionThresholds_t new_thresholds = original_thresholds;
    new_thresholds.freefall_threshold_g = 0.3f;
    new_thresholds.impact_threshold_g = 4.0f;

    detector.setThresholds(new_thresholds);
    DetectionThresholds_t retrieved_thresholds = detector.getThresholds();

    assert_true(abs(retrieved_thresholds.freefall_threshold_g - 0.3f) < 0.01f, "Free fall threshold updated");
    assert_true(abs(retrieved_thresholds.impact_threshold_g - 4.0f) < 0.01f, "Impact threshold updated");

    // Restore original thresholds
    detector.setThresholds(original_thresholds);

    return true;
}

bool TestFallDetector::testStage1FreeFallDetection() {
    TEST_SERIAL_PRINTLN("Testing Stage 1 - Free Fall Detection...");

    detector.resetDetection();

    // Feed normal data first
    SensorData_t normal_data = createTestData(0.0f, 0.0f, 1.0f, 0, 0, 0);
    for (int i = 0; i < 10; i++) {
        detector.processSensorData(normal_data);
        delay(10);
    }
    assert_equal(FALL_STATUS_MONITORING, detector.getCurrentStatus(), "Normal activity state");

    // Feed free fall data (below threshold for sufficient duration)
    SensorData_t free_fall_data = createTestData(0.1f, 0.1f, 0.3f, 0, 0, 0);
    for (int i = 0; i < 25; i++) {  // 250ms of free fall
        detector.processSensorData(free_fall_data);
        delay(10);
    }

    FallStatus_t status = detector.getCurrentStatus();
    assert_true(status == FALL_STATUS_STAGE1_FREEFALL, "Free fall detected");

    return true;
}

bool TestFallDetector::testStage2ImpactDetection() {
    TEST_SERIAL_PRINTLN("Testing Stage 2 - Impact Detection...");

    detector.resetDetection();

    // First trigger Stage 1
    SensorData_t free_fall_data = createTestData(0.1f, 0.1f, 0.3f, 0, 0, 0);
    for (int i = 0; i < 25; i++) {
        detector.processSensorData(free_fall_data);
        delay(10);
    }

    // Then trigger impact
    SensorData_t impact_data = createTestData(1.0f, 2.0f, 4.5f, 50, 60, 40);
    for (int i = 0; i < 5; i++) {  // Brief impact
        detector.processSensorData(impact_data);
        delay(10);
    }

    FallStatus_t status = detector.getCurrentStatus();
    assert_true(status == FALL_STATUS_STAGE2_IMPACT, "Impact detected after free fall");

    return true;
}

bool TestFallDetector::testStage3RotationDetection() {
    TEST_SERIAL_PRINTLN("Testing Stage 3 - Rotation Detection...");

    detector.resetDetection();

    // Progress through stages 1 and 2
    SensorData_t free_fall_data = createTestData(0.1f, 0.1f, 0.3f, 0, 0, 0);
    for (int i = 0; i < 25; i++) {
        detector.processSensorData(free_fall_data);
        delay(10);
    }

    SensorData_t impact_data = createTestData(1.0f, 2.0f, 4.5f, 50, 60, 40);
    for (int i = 0; i < 5; i++) {
        detector.processSensorData(impact_data);
        delay(10);
    }

    // Add rotation data
    SensorData_t rotation_data = createTestData(0.5f, -0.3f, 1.2f, 280, 320, 150);
    for (int i = 0; i < 10; i++) {
        detector.processSensorData(rotation_data);
        delay(10);
    }

    FallStatus_t status = detector.getCurrentStatus();
    assert_true(status == FALL_STATUS_STAGE3_ROTATION, "Rotation detected after impact");

    return true;
}

bool TestFallDetector::testStage4InactivityDetection() {
    TEST_SERIAL_PRINTLN("Testing Stage 4 - Inactivity Detection...");

    detector.resetDetection();

    // Progress through all previous stages quickly
    SensorData_t free_fall_data = createTestData(0.1f, 0.1f, 0.3f, 20, 15, 10);
    for (int i = 0; i < 25; i++) {
        detector.processSensorData(free_fall_data);
        delay(10);
    }

    SensorData_t impact_data = createTestData(1.0f, 2.0f, 4.5f, 300, 250, 180);
    for (int i = 0; i < 5; i++) {
        detector.processSensorData(impact_data);
        delay(10);
    }

    // Simulate inactivity (stable, low motion)
    SensorData_t inactive_data = createTestData(0.1f, -0.05f, 0.95f, 5, 8, 3);
    for (int i = 0; i < 250; i++) {  // 2.5 seconds of inactivity
        detector.processSensorData(inactive_data);
        delay(10);
    }

    FallStatus_t status = detector.getCurrentStatus();
    assert_true(status >= FALL_STATUS_STAGE4_INACTIVITY, "Inactivity detected");

    return true;
}

bool TestFallDetector::testTypicalFallSequence() {
    TEST_SERIAL_PRINTLN("Testing complete typical fall sequence...");

    detector.resetDetection();

    // Use fake data generator for realistic fall sequence
    data_generator.startScenario(SCENARIO_TYPICAL_FALL, 15000);
    TestDataSets::setupTypicalFall(data_generator);

    FallStatus_t final_status = FALL_STATUS_MONITORING;
    int data_points = 0;

    while (data_generator.isScenarioActive() && data_points < 1000) {
        SensorData_t data = data_generator.generateSensorData();
        detector.processSensorData(data);
        final_status = detector.getCurrentStatus();

        // Stop if we reach potential fall detection
        if (final_status == FALL_STATUS_POTENTIAL_FALL) {
            break;
        }

        delay(10);
        data_points++;
    }

    assert_true(final_status >= FALL_STATUS_STAGE4_INACTIVITY, "Typical fall progressed to advanced stages");

    return true;
}

bool TestFallDetector::testSevereFallSequence() {
    TEST_SERIAL_PRINTLN("Testing severe fall sequence...");

    detector.resetDetection();

    data_generator.startScenario(SCENARIO_SEVERE_FALL, 20000);
    TestDataSets::setupSevereFall(data_generator);

    FallStatus_t final_status = FALL_STATUS_MONITORING;
    int data_points = 0;

    while (data_generator.isScenarioActive() && data_points < 1500) {
        SensorData_t data = data_generator.generateSensorData();
        detector.processSensorData(data);
        final_status = detector.getCurrentStatus();

        if (final_status == FALL_STATUS_POTENTIAL_FALL) {
            break;
        }

        delay(10);
        data_points++;
    }

    assert_true(final_status >= FALL_STATUS_STAGE4_INACTIVITY, "Severe fall detected");

    return true;
}

bool TestFallDetector::testFalsePositiveRejection() {
    TEST_SERIAL_PRINTLN("Testing false positive rejection...");

    detector.resetDetection();

    // Test device drop scenario
    data_generator.startScenario(SCENARIO_FALSE_POSITIVE_DROP, 5000);
    TestDataSets::setupFalsePositiveDrop(data_generator);

    FallStatus_t max_status = FALL_STATUS_MONITORING;
    int data_points = 0;

    while (data_generator.isScenarioActive() && data_points < 400) {
        SensorData_t data = data_generator.generateSensorData();
        detector.processSensorData(data);
        FallStatus_t current_status = detector.getCurrentStatus();

        if (current_status > max_status) {
            max_status = current_status;
        }

        delay(10);
        data_points++;
    }

    // False positive should not reach potential fall
    assert_true(max_status < FALL_STATUS_POTENTIAL_FALL, "False positive rejected");

    return true;
}

bool TestFallDetector::testTimeoutHandling() {
    TEST_SERIAL_PRINTLN("Testing detection timeout handling...");

    detector.resetDetection();

    // Trigger initial free fall detection
    SensorData_t free_fall_data = createTestData(0.1f, 0.1f, 0.3f, 0, 0, 0);
    for (int i = 0; i < 25; i++) {
        detector.processSensorData(free_fall_data);
        delay(10);
    }

    assert_equal(FALL_STATUS_STAGE1_FREEFALL, detector.getCurrentStatus(), "Free fall triggered");

    // Feed normal data for extended period (should timeout)
    SensorData_t normal_data = createTestData(0.0f, 0.0f, 1.0f, 0, 0, 0);
    for (int i = 0; i < 1200; i++) {  // 12 seconds (beyond detection window)
        detector.processSensorData(normal_data);
        delay(10);
    }

    // Should timeout and reset to monitoring
    assert_equal(FALL_STATUS_MONITORING, detector.getCurrentStatus(), "Timeout reset to monitoring");

    return true;
}

bool TestFallDetector::testPartialFallSequence() {
    TEST_SERIAL_PRINTLN("Testing partial fall sequence...");

    detector.resetDetection();

    // Trigger free fall but not impact
    SensorData_t free_fall_data = createTestData(0.1f, 0.1f, 0.3f, 10, 15, 8);
    for (int i = 0; i < 25; i++) {
        detector.processSensorData(free_fall_data);
        delay(10);
    }

    // Return to normal without impact
    SensorData_t normal_data = createTestData(0.0f, 0.0f, 1.0f, 2, 3, 1);
    for (int i = 0; i < 50; i++) {
        detector.processSensorData(normal_data);
        delay(10);
    }

    // Should not progress past Stage 1 without impact
    FallStatus_t status = detector.getCurrentStatus();
    assert_true(status <= FALL_STATUS_STAGE1_FREEFALL, "Partial sequence doesn't advance inappropriately");

    return true;
}

bool TestFallDetector::testRecoveryDuringDetection() {
    TEST_SERIAL_PRINTLN("Testing recovery during detection...");

    detector.resetDetection();

    // Progress to inactivity stage
    SensorData_t free_fall_data = createTestData(0.1f, 0.1f, 0.3f, 20, 15, 10);
    for (int i = 0; i < 25; i++) {
        detector.processSensorData(free_fall_data);
        delay(10);
    }

    SensorData_t impact_data = createTestData(1.0f, 2.0f, 4.5f, 300, 250, 180);
    for (int i = 0; i < 5; i++) {
        detector.processSensorData(impact_data);
        delay(10);
    }

    SensorData_t inactive_data = createTestData(0.1f, -0.05f, 0.95f, 5, 8, 3);
    for (int i = 0; i < 100; i++) {  // Brief inactivity
        detector.processSensorData(inactive_data);
        delay(10);
    }

    // Simulate recovery movement
    SensorData_t recovery_data = createTestData(0.3f, 0.5f, 1.2f, 45, 60, 35);
    for (int i = 0; i < 50; i++) {
        detector.processSensorData(recovery_data);
        delay(10);
    }

    // Should reset due to recovery movement
    FallStatus_t status = detector.getCurrentStatus();
    assert_true(status == FALL_STATUS_MONITORING, "Recovery detected, status reset");

    return true;
}

bool TestFallDetector::testSensorDataValidation() {
    TEST_SERIAL_PRINTLN("Testing sensor data validation...");

    detector.resetDetection();

    // Test invalid data handling
    SensorData_t invalid_data = createTestData(0.1f, 0.1f, 0.3f, 0, 0, 0);
    invalid_data.valid = false;

    FallStatus_t status_before = detector.getCurrentStatus();
    detector.processSensorData(invalid_data);
    FallStatus_t status_after = detector.getCurrentStatus();

    assert_equal(status_before, status_after, "Invalid data ignored");

    return true;
}

bool TestFallDetector::testWithFakeDataGenerator() {
    TEST_SERIAL_PRINTLN("Testing integration with fake data generator...");

    detector.resetDetection();

    // Test multiple scenarios
    TestScenario_t scenarios[] = {
        SCENARIO_NORMAL_ACTIVITY,
        SCENARIO_WALKING,
        SCENARIO_FALSE_POSITIVE_EXERCISE
    };

    bool all_scenarios_passed = true;

    for (int s = 0; s < 3; s++) {
        detector.resetDetection();
        data_generator.startScenario(scenarios[s], 3000);

        FallStatus_t max_status = FALL_STATUS_MONITORING;
        int data_points = 0;

        while (data_generator.isScenarioActive() && data_points < 300) {
            SensorData_t data = data_generator.generateSensorData();
            if (data_generator.validateGeneratedData(data)) {
                detector.processSensorData(data);
                FallStatus_t current_status = detector.getCurrentStatus();
                if (current_status > max_status) {
                    max_status = current_status;
                }
            }
            delay(10);
            data_points++;
        }

        // Normal activities should not trigger fall detection
        if (scenarios[s] == SCENARIO_NORMAL_ACTIVITY ||
            scenarios[s] == SCENARIO_WALKING ||
            scenarios[s] == SCENARIO_FALSE_POSITIVE_EXERCISE) {
            all_scenarios_passed &= (max_status < FALL_STATUS_POTENTIAL_FALL);
        }
    }

    assert_true(all_scenarios_passed, "Fake data generator integration");

    return true;
}

bool TestFallDetector::testDetectionTiming() {
    TEST_SERIAL_PRINTLN("Testing detection timing performance...");

    detector.resetDetection();

    SensorData_t test_data = createTestData(0.1f, 0.1f, 0.3f, 20, 25, 15);

    uint32_t start_time = micros();

    // Process 100 data points
    for (int i = 0; i < 100; i++) {
        test_data.timestamp = millis();
        detector.processSensorData(test_data);
    }

    uint32_t end_time = micros();
    uint32_t total_time = end_time - start_time;
    uint32_t avg_time_per_sample = total_time / 100;

    // Should process each sample in less than 1000 microseconds (1ms)
    assert_true(avg_time_per_sample < 1000, "Detection timing performance");

    TEST_SERIAL_PRINT("Average processing time per sample: ");
    TEST_SERIAL_PRINT(avg_time_per_sample);
    TEST_SERIAL_PRINTLN(" μs");

    return true;
}

bool TestFallDetector::testMemoryUsage() {
    TEST_SERIAL_PRINTLN("Testing memory usage...");

    // This is a basic test - in a real implementation you'd measure heap usage
    size_t initial_free_heap = ESP.getFreeHeap();

    // Create multiple detector instances
    FallDetector* detectors[5];
    for (int i = 0; i < 5; i++) {
        detectors[i] = new FallDetector();
        detectors[i]->init();
    }

    size_t after_creation_heap = ESP.getFreeHeap();

    // Clean up
    for (int i = 0; i < 5; i++) {
        delete detectors[i];
    }

    size_t memory_used = initial_free_heap - after_creation_heap;

    // Each detector should use less than 10KB
    assert_true(memory_used < 50000, "Memory usage within limits"); // 5 detectors < 50KB

    TEST_SERIAL_PRINT("Memory used by 5 detectors: ");
    TEST_SERIAL_PRINT(memory_used);
    TEST_SERIAL_PRINTLN(" bytes");

    return true;
}

bool TestFallDetector::runAllTests() {
    TEST_SERIAL_PRINTLN("========================================");
    TEST_SERIAL_PRINTLN("      FALL DETECTOR TEST SUITE");
    TEST_SERIAL_PRINTLN("========================================");

    tests_passed = 0;
    tests_failed = 0;

    bool all_passed = true;

    all_passed &= testInitialization();
    all_passed &= testResetDetection();
    all_passed &= testThresholdConfiguration();
    all_passed &= testStage1FreeFallDetection();
    all_passed &= testStage2ImpactDetection();
    all_passed &= testStage3RotationDetection();
    all_passed &= testStage4InactivityDetection();
    all_passed &= testTypicalFallSequence();
    all_passed &= testSevereFallSequence();
    all_passed &= testFalsePositiveRejection();
    all_passed &= testTimeoutHandling();
    all_passed &= testPartialFallSequence();
    all_passed &= testRecoveryDuringDetection();
    all_passed &= testSensorDataValidation();
    all_passed &= testWithFakeDataGenerator();
    all_passed &= testDetectionTiming();
    all_passed &= testMemoryUsage();

    printTestSummary();

    return all_passed;
}

void TestFallDetector::printTestSummary() {
    TEST_SERIAL_PRINTLN("========================================");
    TEST_SERIAL_PRINTLN("        FALL DETECTOR TEST RESULTS");
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

    if (tests_failed == 0) {
        TEST_SERIAL_PRINTLN("✓ ALL FALL DETECTOR TESTS PASSED!");
    } else {
        TEST_SERIAL_PRINTLN("✗ SOME TESTS FAILED!");
    }
    TEST_SERIAL_PRINTLN("========================================");
}
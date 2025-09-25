#include "test_runner.h"

TestRunner::TestRunner() : total_test_suites(0), passed_test_suites(0),
                           failed_test_suites(0), test_start_time(0), test_end_time(0) {
}

TestRunner::~TestRunner() {
}

bool TestRunner::runConfidenceScorerTests() {
    test_logger.logTestStart("Confidence Scorer Test Suite");
    test_logger.startTimer("ConfidenceScorer");

    Serial.println("🧮 Running Confidence Scorer tests...");
    total_test_suites++;

    bool passed = confidence_tests.runAllTests();

    test_logger.endTimer("ConfidenceScorer");

    if (passed) {
        passed_test_suites++;
        Serial.println("  ✓ Confidence Scorer tests completed successfully");
        LOG_TEST_PASS(LOG_FILE_CONFIDENCE, "Confidence Scorer Test Suite");
    } else {
        failed_test_suites++;
        Serial.println("  ✗ Confidence Scorer tests had failures");
        LOG_TEST_FAIL(LOG_FILE_CONFIDENCE, "Confidence Scorer Test Suite", "Some tests failed");
    }

    return passed;
}

bool TestRunner::runFallDetectorTests() {
    test_logger.logTestStart("Fall Detector Test Suite");
    test_logger.startTimer("FallDetector");

    Serial.println("🔍 Running Fall Detector tests...");
    total_test_suites++;

    bool passed = detector_tests.runAllTests();

    test_logger.endTimer("FallDetector");

    if (passed) {
        passed_test_suites++;
        Serial.println("  ✓ Fall Detector tests completed successfully");
        LOG_TEST_PASS(LOG_FILE_DETECTOR, "Fall Detector Test Suite");
    } else {
        failed_test_suites++;
        Serial.println("  ✗ Fall Detector tests had failures");
        LOG_TEST_FAIL(LOG_FILE_DETECTOR, "Fall Detector Test Suite", "Some tests failed");
    }

    return passed;
}

bool TestRunner::runIntegrationTests() {
    test_logger.logTestStart("Integration Test Suite");
    test_logger.startTimer("Integration");

    Serial.println("🔗 Running Integration tests...");
    total_test_suites++;

    bool passed = integration_tests.runAllIntegrationTests();

    test_logger.endTimer("Integration");

    if (passed) {
        passed_test_suites++;
        Serial.println("  ✓ Integration tests completed successfully");
        LOG_TEST_PASS(LOG_FILE_INTEGRATION, "Integration Test Suite");
    } else {
        failed_test_suites++;
        Serial.println("  ✗ Integration tests had failures");
        LOG_TEST_FAIL(LOG_FILE_INTEGRATION, "Integration Test Suite", "Some tests failed");
    }

    return passed;
}

bool TestRunner::runAllTests() {

    test_start_time = millis();

    // Initialize logging
    initializeLogging();

    setupTestEnvironment();

    if (!validateTestEnvironment()) {
        Serial.println("✗ Test environment validation failed!");
        LOG_ERROR_MAIN("Test environment validation failed");
        return false;
    }

    printTestConfiguration();

    // Initialize counters
    total_test_suites = 0;
    passed_test_suites = 0;
    failed_test_suites = 0;

    bool all_passed = true;

    // Run all test suites
    all_passed &= runConfidenceScorerTests();
    Serial.println();

    all_passed &= runFallDetectorTests();
    Serial.println();

    all_passed &= runIntegrationTests();
    Serial.println();

    test_end_time = millis();

    cleanupTestEnvironment();
    printOverallSummary();

    // Finalize logging and export results
    finalizeLogging();

    return all_passed;
}

bool TestRunner::runQuickTests() {
    test_start_time = millis();
    setupTestEnvironment();

    // Set quick test mode for integration tests
    integration_tests.setVerboseOutput(false);

    bool all_passed = true;

    // Run essential tests only
    Serial.println("Testing core confidence scoring...");
    TestConfidenceScorer quick_confidence;
    all_passed &= quick_confidence.testTypicalFallScoring();
    all_passed &= quick_confidence.testFalsePositiveScoring();

    Serial.println("Testing basic fall detection...");
    TestFallDetector quick_detector;
    all_passed &= quick_detector.testStage1FreeFallDetection();
    all_passed &= quick_detector.testTypicalFallSequence();

    Serial.println("Testing system integration...");
    all_passed &= integration_tests.testCompleteTypicalFall();

    test_end_time = millis();

    Serial.println("=== Quick Test Results ===");
    Serial.print("All tests passed: ");
    Serial.println(all_passed ? "YES" : "NO");
    Serial.print("Execution time: ");
    Serial.print((test_end_time - test_start_time) / 1000.0f);
    Serial.println(" seconds");

    return all_passed;
}

bool TestRunner::runExtendedTests() {
    // Enable verbose output for extended tests
    setVerboseOutput(true);

    // Run all tests with extended scenarios
    bool all_passed = runAllTests();

    // Additional extended tests
    Serial.println("Running extended stress tests...");

    // Extended fake data generator tests
    FakeDataGenerator extended_generator;

    // Test all scenario types
    TestScenario_t all_scenarios[] = {
        SCENARIO_NORMAL_ACTIVITY,
        SCENARIO_WALKING,
        SCENARIO_SITTING_DOWN,
        SCENARIO_STANDING_UP,
        SCENARIO_TYPICAL_FALL,
        SCENARIO_SEVERE_FALL,
        SCENARIO_FALSE_POSITIVE_DROP,
        SCENARIO_FALSE_POSITIVE_EXERCISE,
        SCENARIO_RECOVERY_AFTER_FALL
    };

    Serial.println("Testing all scenario types...");
    for (int i = 0; i < 9; i++) {
        extended_generator.startScenario(all_scenarios[i], 5000);

        int samples = 0;
        while (extended_generator.isScenarioActive() && samples < 100) {
            SensorData_t data = extended_generator.generateSensorData();

            if (!extended_generator.validateGeneratedData(data)) {
                Serial.print("Invalid data generated for scenario ");
                Serial.println(all_scenarios[i]);
                all_passed = false;
            }
            samples++;
            delay(10);
        }
    }

    return all_passed;
}

void TestRunner::setVerboseOutput(bool verbose) {
    integration_tests.setVerboseOutput(verbose);
}

void TestRunner::setQuickTestMode(bool quick_mode) {
    if (quick_mode) {
        integration_tests.setVerboseOutput(false);
    }
}

bool TestRunner::validateTestEnvironment() {
    Serial.println("Validating test environment...");

    // Check available memory
    size_t free_heap = ESP.getFreeHeap();
    if (free_heap < 50000) {  // Need at least 50KB for tests
        Serial.print("Insufficient memory for testing: ");
        Serial.print(free_heap);
        Serial.println(" bytes available");
        return false;
    }

    // Check core system initialization
    FallDetector test_detector;
    if (!test_detector.init()) {
        Serial.println("Fall detector initialization failed");
        return false;
    }

    // Check data generator
    FakeDataGenerator test_generator;
    test_generator.startScenario(SCENARIO_NORMAL_ACTIVITY, 1000);
    SensorData_t test_data = test_generator.generateSensorData();
    if (!test_generator.validateGeneratedData(test_data)) {
        Serial.println("Fake data generator validation failed");
        return false;
    }

    Serial.println("✓ Test environment validation passed");
    return true;
}

void TestRunner::setupTestEnvironment() {
    Serial.println("Setting up test environment...");

    // Initialize random seed for consistent but varied testing
    randomSeed(analogRead(0));

    // Clear any existing state
    // (In a real implementation, you might reset hardware interfaces)

    Serial.println("✓ Test environment setup complete");
}

void TestRunner::cleanupTestEnvironment() {
    Serial.println("Cleaning up test environment...");

    // Reset system state
    // (Cleanup any resources, reset pins, etc.)

    Serial.println("✓ Test environment cleanup complete");
}

void TestRunner::printOverallSummary() {
    uint32_t execution_time = test_end_time - test_start_time;

    // Log detailed summary to files
    LOG_INFO_MAIN("========== OVERALL TEST SUMMARY ==========");
    LOG_INFO_MAIN("Test Suites Run: " + String(total_test_suites));
    LOG_INFO_MAIN("Test Suites Passed: " + String(passed_test_suites));
    LOG_INFO_MAIN("Test Suites Failed: " + String(failed_test_suites));
    if (total_test_suites > 0) {
        LOG_INFO_MAIN("Success Rate: " + String((passed_test_suites * 100) / total_test_suites) + "%");
    }
    LOG_INFO_MAIN("Total Execution Time: " + String(execution_time / 1000.0f) + " seconds");

    // Show concise summary on serial
    Serial.println("\n📊 TEST SUMMARY");
    Serial.println("================");
    Serial.print("Suites: " + String(passed_test_suites) + "/" + String(total_test_suites) + " passed");
    if (total_test_suites > 0) {
        Serial.print(" (" + String((passed_test_suites * 100) / total_test_suites) + "%)");
    }
    Serial.println();
    Serial.println("Time: " + String(execution_time / 1000.0f) + "s");
    Serial.print("Memory: " + String(ESP.getFreeHeap()) + " bytes free");
    Serial.println();

    if (failed_test_suites == 0) {
        Serial.println("\n🎉 ALL TESTS PASSED!");
        Serial.println("✅ SmartFall system validated");
    } else {
        Serial.println("\n❌ SOME TESTS FAILED");
        Serial.println("📄 Check logs with 'L' command");
    }
    Serial.println("================");
}

void TestRunner::printSystemInfo() {
    // System info is now logged to files only via test_logger.logSystemInfo()
    // No need to clutter the console with this information
}

void TestRunner::printTestConfiguration() {
    // Log detailed configuration to files
    LOG_INFO_MAIN("=== Test Configuration ===");
    LOG_INFO_MAIN("Target Hardware: ESP32 Feather V2");
    LOG_INFO_MAIN("Simulation Mode: Wokwi Compatible");
    LOG_INFO_MAIN("Test Framework: Custom SmartFall Test Suite");
    LOG_INFO_MAIN("Sensor Simulation: Fake Data Generator");
    LOG_INFO_MAIN("Algorithm: 5-Stage Fall Detection");
    LOG_INFO_MAIN("Confidence Scoring: 105-Point System");
    LOG_INFO_MAIN("Test Start Time: " + String(millis() / 1000));
    LOG_INFO_MAIN("===========================");

    // Show brief config on console
    Serial.println("⚙️ SmartFall Test Suite - 5-Stage Fall Detection + 105-Point Confidence Scoring");
}

void TestRunner::printMemoryUsage() {
    // Memory usage is now logged to files and shown in summary
    // No need for separate detailed memory section in console
}

void TestRunner::printExecutionTime() {
    if (test_end_time > test_start_time) {
        uint32_t execution_time = test_end_time - test_start_time;
        // Execution time is now shown in the summary, no need for separate output
        LOG_INFO_MAIN("Total test execution time: " + String(execution_time) + "ms");
    }
}

void TestRunner::initializeLogging() {
    // Disable console output for logs, keep only file logging
    test_logger.enableConsoleOutput(false);
    test_logger.enableFileOutput(true);

    if (test_logger.init()) {
        Serial.println("✓ Test logging initialized - logs writing to SPIFFS files only");
        LOG_INFO_MAIN("SmartFall Test Suite Starting");
        test_logger.logSystemInfo();
    } else {
        Serial.println("⚠ Test logging failed to initialize - file logging disabled");
    }
}

void TestRunner::finalizeLogging() {
    if (failed_test_suites == 0) {
        LOG_INFO_MAIN("All test suites PASSED");
    } else {
        LOG_ERROR_MAIN("Test suites FAILED: " + String(failed_test_suites) + "/" + String(total_test_suites));
    }

    test_logger.logMemoryUsage();
    LOG_INFO_MAIN("SmartFall Test Suite Completed");

    // Export logs to serial
    exportLogsToSerial();

    // Print log summary
    printLogSummary();

    test_logger.cleanup();
}

void TestRunner::exportLogsToSerial() {
    Serial.println("\n=== EXPORTING LOGS TO SERIAL ===");
    Serial.println("Copy the log data below for external analysis:");
    Serial.println();

    test_logger.exportLogsToSerial();

    Serial.println("\n=== END OF LOG EXPORT ===");
    Serial.println("You can copy/paste the above logs to a file for analysis.");
    Serial.println();
}

void TestRunner::printLogSummary() {
    Serial.println("\n=== LOG FILE SUMMARY ===");
    test_logger.printLogFileSummary();

    Serial.println("Available commands:");
    Serial.println("- Send 'L' to export logs again");
    Serial.println("- Send 'D' to delete all log files");
    Serial.println("- Send 'S' to show log file sizes");
    Serial.println();
}
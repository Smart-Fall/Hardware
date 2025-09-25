/*
 * SmartFall Test Suite
 *
 * Comprehensive testing for the SmartFall fall detection system
 * This file serves as the main entry point for all testing scenarios
 *
 * To use this test suite:
 * 1. Replace src/main.cpp with this file temporarily
 * 2. Build and run: pio run -e esp32dev -t upload && pio device monitor
 * 3. Watch the complete test results in serial monitor
 *
 * Test Coverage:
 * - Fake data generation and validation
 * - Confidence scoring algorithm accuracy
 * - Fall detection stage progression
 * - Complete system integration with realistic scenarios
 * - Alert system activation
 * - SOS button functionality
 * - False positive rejection
 * - Memory usage and performance validation
 */

#include <Arduino.h>
#include "test_runner.h"
#include "fake_data_generator.h"

// Test configuration
SmartFallTestConfig test_config;
TestRunner test_runner;

// Test mode selection (can be controlled via serial input)
enum TestMode {
    MODE_ALL_TESTS,
    MODE_QUICK_TESTS,
    MODE_EXTENDED_TESTS,
    MODE_INTERACTIVE_DEMO,
    MODE_CONTINUOUS_MONITORING
};

TestMode current_test_mode = MODE_ALL_TESTS;

// Forward declarations
void runSelectedTestMode();
void runInteractiveDemo();
void runContinuousMonitoring();
void runDemoScenario(FakeDataGenerator& generator, FallDetector& detector,
                    ConfidenceScorer& scorer, TestScenario_t scenario,
                    const char* scenario_name);

void setup() {
    Serial.begin(115200);
    delay(3000);  // Wait for serial connection

    Serial.println("########################################");
    Serial.println("#                                      #");
    Serial.println("#         SMARTFALL TEST SUITE        #");
    Serial.println("#    Comprehensive System Validation  #");
    Serial.println("#                                      #");
    Serial.println("# 🔬 Testing all components with      #");
    Serial.println("# 📊 realistic simulated sensor data  #");
    Serial.println("#                                      #");
    Serial.println("########################################");
    Serial.println();

    // Display test options
    Serial.println("Available Test Modes:");
    Serial.println("1. Complete Test Suite (all tests)");
    Serial.println("2. Quick Test Suite (essential tests only)");
    Serial.println("3. Extended Test Suite (comprehensive + stress tests)");
    Serial.println("4. Interactive Demo (manual scenario testing)");
    Serial.println("5. Continuous Monitoring (ongoing system validation)");
    Serial.println();
    Serial.println("Send '1'-'5' to select test mode now, or wait 5 seconds for default.");
    Serial.println("Send 'L' to export logs, 'D' to delete logs, 'S' for log summary.");
    Serial.println();

    // Wait for user input or timeout
    bool user_selected = false;
    uint32_t start_wait = millis();

    while ((millis() - start_wait) < 5000 && !user_selected) {
        if (Serial.available()) {
            char input = Serial.read();
            // Process immediate input
            switch (input) {
                case '1':
                    current_test_mode = MODE_ALL_TESTS;
                    user_selected = true;
                    break;
                case '2':
                    current_test_mode = MODE_QUICK_TESTS;
                    user_selected = true;
                    break;
                case '3':
                    current_test_mode = MODE_EXTENDED_TESTS;
                    user_selected = true;
                    break;
                case '4':
                    current_test_mode = MODE_INTERACTIVE_DEMO;
                    user_selected = true;
                    break;
                case '5':
                    current_test_mode = MODE_CONTINUOUS_MONITORING;
                    user_selected = true;
                    break;
                default:
                    // For other commands, we'll handle them in the loop
                    continue;
            }
        }
        delay(100);
    }

    if (!user_selected) {
        Serial.println("No selection made, starting Complete Test Suite...");
    }

    // Run the selected test mode
    runSelectedTestMode();

    Serial.println("\n💡 System ready for interactive commands. Send a command:");
}

void loop() {
    // Check for serial input to change test mode
    if (Serial.available()) {
        int input = Serial.read();
        Serial.println(); // Add line break after command

        switch (input) {
            case '1':
                Serial.println("🚀 Switching to Complete Test Suite...");
                current_test_mode = MODE_ALL_TESTS;
                runSelectedTestMode();
                break;

            case '2':
                Serial.println("⚡ Switching to Quick Test Suite...");
                current_test_mode = MODE_QUICK_TESTS;
                runSelectedTestMode();
                break;

            case '3':
                Serial.println("🔬 Switching to Extended Test Suite...");
                current_test_mode = MODE_EXTENDED_TESTS;
                runSelectedTestMode();
                break;

            case '4':
                Serial.println("🎬 Starting Interactive Demo...");
                current_test_mode = MODE_INTERACTIVE_DEMO;
                runInteractiveDemo();
                Serial.println("\n💡 Returning to main menu. Send a command:");
                break;

            case '5':
                Serial.println("📊 Starting Continuous Monitoring...");
                current_test_mode = MODE_CONTINUOUS_MONITORING;
                runContinuousMonitoring();
                Serial.println("\n💡 Returning to main menu. Send a command:");
                break;

            case 'v':
            case 'V':
                Serial.println("🔊 Enabling verbose output...");
                test_runner.setVerboseOutput(true);
                Serial.println("✓ Verbose mode enabled for next test run");
                Serial.println("💡 Send '1'-'5' to run tests or another command:");
                break;

            case 'q':
            case 'Q':
                Serial.println("⚡ Enabling quick mode...");
                test_runner.setQuickTestMode(true);
                Serial.println("✓ Quick mode enabled for next test run");
                Serial.println("💡 Send '1'-'5' to run tests or another command:");
                break;

            case 'l':
            case 'L':
                Serial.println("📄 Exporting logs to serial...");
                test_runner.exportLogsToSerial();
                Serial.println("💡 Log export complete. Send another command:");
                break;

            case 'd':
            case 'D':
                Serial.println("🗑️ Deleting all log files...");
                if (test_logger.deleteAllLogs()) {
                    Serial.println("✓ All log files deleted successfully");
                } else {
                    Serial.println("✗ Failed to delete some log files");
                }
                Serial.println("💡 Send another command:");
                break;

            case 's':
            case 'S':
                Serial.println("📊 Showing log file summary...");
                test_runner.printLogSummary();
                Serial.println("💡 Send another command:");
                break;

            case 'h':
            case 'H':
            case '?':
                Serial.println("🎮 AVAILABLE COMMANDS:");
                Serial.println("  1-5: Run different test modes");
                Serial.println("  V/Q: Toggle verbose/quick mode");
                Serial.println("  L:   Export logs to serial");
                Serial.println("  D:   Delete all log files");
                Serial.println("  S:   Show log file summary");
                Serial.println("  H/?:  Show this help");
                Serial.println("💡 Send a command:");
                break;

            default:
                Serial.println("❓ Unknown command '" + String((char)input) + "'");
                Serial.println("💡 Send 'H' for help or '1'-'5' for test modes");
                break;
        }
    }

    // Small delay to prevent overwhelming the serial monitor
    delay(1000);
}

void runSelectedTestMode() {
    bool test_result = false;

    switch (current_test_mode) {
        case MODE_ALL_TESTS:
            Serial.println("\n🚀 COMPLETE TEST SUITE");
            Serial.println("=======================");
            test_result = test_runner.runAllTests();
            break;

        case MODE_QUICK_TESTS:
            Serial.println("\n⚡ QUICK TEST SUITE");
            Serial.println("==================");
            test_result = test_runner.runQuickTests();
            break;

        case MODE_EXTENDED_TESTS:
            Serial.println("\n🔬 EXTENDED TEST SUITE");
            Serial.println("=====================");
            test_result = test_runner.runExtendedTests();
            break;

        default:
            Serial.println("\n🚀 COMPLETE TEST SUITE (default)");
            Serial.println("================================");
            test_result = test_runner.runAllTests();
            break;
    }

    // Final result summary
    Serial.println();
    Serial.println("========================================");
    if (test_result) {
        Serial.println("🎉 ALL TESTS COMPLETED SUCCESSFULLY! 🎉");
        Serial.println("✅ SmartFall system is fully validated!");
    } else {
        Serial.println("❌ SOME TESTS FAILED!");
        Serial.println("🔧 System needs attention before deployment");
    }
    Serial.println("========================================");
    Serial.println();
    Serial.println("🎮 INTERACTIVE COMMANDS:");
    Serial.println("  1-5: Run different test modes");
    Serial.println("  V/Q: Toggle verbose/quick mode");
    Serial.println("  L:   Export logs to serial");
    Serial.println("  D:   Delete all log files");
    Serial.println("  S:   Show log file summary");
    Serial.println();
    Serial.println("💡 Waiting for your command...");
}

void runInteractiveDemo() {
    Serial.println("########################################");
    Serial.println("#        INTERACTIVE DEMO MODE        #");
    Serial.println("########################################");
    Serial.println();
    Serial.println("This demo lets you manually trigger different scenarios");
    Serial.println("and observe the SmartFall system response in real-time.");
    Serial.println();

    FakeDataGenerator demo_generator;
    FallDetector demo_detector;
    ConfidenceScorer demo_scorer;

    demo_detector.init();

    Serial.println("Available scenarios:");
    Serial.println("A - Normal Activity (baseline)");
    Serial.println("B - Walking Activity");
    Serial.println("C - Typical Fall");
    Serial.println("D - Severe Fall");
    Serial.println("E - Device Drop (false positive)");
    Serial.println("F - Exercise Activity (false positive)");
    Serial.println("S - SOS Button Press");
    Serial.println("R - Reset System");
    Serial.println("X - Exit Demo");
    Serial.println();

    bool demo_running = true;
    while (demo_running) {
        if (Serial.available()) {
            char cmd = Serial.read();

            switch (cmd) {
                case 'A':
                case 'a':
                    runDemoScenario(demo_generator, demo_detector, demo_scorer,
                                   SCENARIO_NORMAL_ACTIVITY, "Normal Activity");
                    break;

                case 'B':
                case 'b':
                    runDemoScenario(demo_generator, demo_detector, demo_scorer,
                                   SCENARIO_WALKING, "Walking Activity");
                    break;

                case 'C':
                case 'c':
                    runDemoScenario(demo_generator, demo_detector, demo_scorer,
                                   SCENARIO_TYPICAL_FALL, "Typical Fall");
                    break;

                case 'D':
                case 'd':
                    runDemoScenario(demo_generator, demo_detector, demo_scorer,
                                   SCENARIO_SEVERE_FALL, "Severe Fall");
                    break;

                case 'E':
                case 'e':
                    runDemoScenario(demo_generator, demo_detector, demo_scorer,
                                   SCENARIO_FALSE_POSITIVE_DROP, "Device Drop");
                    break;

                case 'F':
                case 'f':
                    runDemoScenario(demo_generator, demo_detector, demo_scorer,
                                   SCENARIO_FALSE_POSITIVE_EXERCISE, "Exercise Activity");
                    break;

                case 'S':
                case 's':
                    Serial.println(">>> SOS BUTTON PRESSED - MANUAL EMERGENCY!");
                    Serial.println(">>> All detection bypassed - immediate alert!");
                    Serial.println(">>> Emergency services would be contacted now");
                    break;

                case 'R':
                case 'r':
                    demo_detector.resetDetection();
                    demo_scorer.resetScore();
                    Serial.println("System reset to monitoring mode");
                    break;

                case 'X':
                case 'x':
                    Serial.println("Exiting demo mode...");
                    demo_running = false;
                    break;

                default:
                    Serial.println("Invalid command. Use A-F for scenarios, S for SOS, R to reset, X to exit.");
                    break;
            }
        }
        delay(100);
    }
}

void runDemoScenario(FakeDataGenerator& generator, FallDetector& detector,
                    ConfidenceScorer& scorer, TestScenario_t scenario,
                    const char* scenario_name) {

    Serial.print("🎬 Starting scenario: ");
    Serial.println(scenario_name);
    Serial.println("Watch the real-time detection process...");
    Serial.println();

    detector.resetDetection();
    scorer.resetScore();

    generator.startScenario(scenario, 15000);

    uint32_t start_time = millis();
    int sample_count = 0;
    FallStatus_t last_status = FALL_STATUS_MONITORING;

    while (generator.isScenarioActive() && (millis() - start_time) < 20000) {
        SensorData_t data = generator.generateSensorData();
        detector.processSensorData(data);

        FallStatus_t current_status = detector.getCurrentStatus();

        // Print status changes
        if (current_status != last_status) {
            Serial.print("Status Change: ");
            Serial.print(detector.getStatusString(last_status));
            Serial.print(" → ");
            Serial.println(detector.getStatusString(current_status));
        }

        // Print periodic updates
        if ((sample_count % 100) == 0) {
            float total_accel = sqrt(data.accel_x*data.accel_x +
                                   data.accel_y*data.accel_y +
                                   data.accel_z*data.accel_z);

            Serial.print("Sample ");
            Serial.print(sample_count);
            Serial.print(": Accel=");
            Serial.print(total_accel, 2);
            Serial.print("g, Status=");
            Serial.println(detector.getStatusString(current_status));
        }

        // Handle potential fall detection
        if (current_status == FALL_STATUS_POTENTIAL_FALL) {
            Serial.println(">>> POTENTIAL FALL DETECTED!");

            // Score the fall
            scorer.addStage1Score(detector.getFreefalDuration(), 0.2f);
            scorer.addStage2Score(detector.getMaxImpact(), 300.0f, false);
            scorer.addStage3Score(detector.getMaxRotation(), 0);
            scorer.addStage4Score(2000, true);
            scorer.addPressureFilterScore(1.0f);
            scorer.addHeartRateFilterScore(15.0f);
            scorer.addFSRFilterScore(false, true);

            uint8_t confidence_score = scorer.getTotalScore();
            FallConfidence_t confidence_level = scorer.getConfidenceLevel();

            Serial.print("Confidence Score: ");
            Serial.print(confidence_score);
            Serial.print("/105 - ");
            Serial.println(scorer.getConfidenceString(confidence_level));

            if (confidence_level >= CONFIDENCE_CONFIRMED) {
                Serial.println(">>> FALL CONFIRMED - EMERGENCY ALERT!");
                Serial.println(">>> Audio/Haptic/Visual alerts activated");
            }

            scorer.printScoreBreakdown();
            break;
        }

        last_status = current_status;
        sample_count++;
        delay(10);
    }

    Serial.println();
    Serial.print("Scenario completed. Final status: ");
    Serial.println(detector.getStatusString(detector.getCurrentStatus()));
    Serial.println("Press any key to return to demo menu...");
    Serial.println();
}

void runContinuousMonitoring() {
    Serial.println("########################################");
    Serial.println("#     CONTINUOUS MONITORING MODE      #");
    Serial.println("########################################");
    Serial.println();
    Serial.println("Simulating 24/7 system monitoring with");
    Serial.println("random activity patterns and occasional test scenarios");
    Serial.println("Press 'X' to exit continuous monitoring");
    Serial.println();

    FakeDataGenerator monitor_generator;
    FallDetector monitor_detector;
    ConfidenceScorer monitor_scorer;

    monitor_detector.init();

    // Continuous monitoring loop
    uint32_t monitoring_start = millis();
    uint32_t last_report = monitoring_start;
    uint32_t scenario_change_time = monitoring_start;

    TestScenario_t monitoring_scenarios[] = {
        SCENARIO_NORMAL_ACTIVITY,
        SCENARIO_WALKING,
        SCENARIO_NORMAL_ACTIVITY,
        SCENARIO_FALSE_POSITIVE_EXERCISE,
        SCENARIO_NORMAL_ACTIVITY
    };

    int current_scenario_index = 0;
    monitor_generator.startScenario(monitoring_scenarios[0], 30000);

    while (true) {
        // Check for exit command
        if (Serial.available()) {
            char cmd = Serial.read();
            if (cmd == 'X' || cmd == 'x') {
                Serial.println("Exiting continuous monitoring...");
                break;
            }
        }

        uint32_t current_time = millis();

        // Change scenarios periodically
        if ((current_time - scenario_change_time) > 30000) {
            current_scenario_index = (current_scenario_index + 1) % 5;
            monitor_generator.startScenario(monitoring_scenarios[current_scenario_index], 30000);
            scenario_change_time = current_time;

            Serial.print("Switched to scenario: ");
            Serial.println(current_scenario_index);
        }

        // Process sensor data
        SensorData_t data = monitor_generator.generateSensorData();
        monitor_detector.processSensorData(data);

        // Periodic status reports
        if ((current_time - last_report) > 10000) {  // Every 10 seconds
            uint32_t runtime_minutes = (current_time - monitoring_start) / 60000;

            Serial.print("Runtime: ");
            Serial.print(runtime_minutes);
            Serial.print(" min | Status: ");
            Serial.print(monitor_detector.getStatusString(monitor_detector.getCurrentStatus()));
            Serial.print(" | Free Heap: ");
            Serial.print(ESP.getFreeHeap());
            Serial.println(" bytes");

            last_report = current_time;
        }

        delay(10);  // 100Hz monitoring
    }
}
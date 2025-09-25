#include "test_confidence_scorer.h"
#include "../utils/config.h"

// Conditionally enable/disable test serial output
#if ENABLE_TEST_SERIAL_OUTPUT
  #define TEST_SERIAL_PRINT(...) Serial.print(__VA_ARGS__)
  #define TEST_SERIAL_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
  #define TEST_SERIAL_PRINT(...) do {} while(0)
  #define TEST_SERIAL_PRINTLN(...) do {} while(0)
#endif

TestConfidenceScorer::TestConfidenceScorer() : tests_passed(0), tests_failed(0) {
}

TestConfidenceScorer::~TestConfidenceScorer() {
}

void TestConfidenceScorer::assert_equal(uint8_t expected, uint8_t actual, const char* test_name) {
    bool passed = (expected == actual);
    if (!passed) {
        TEST_SERIAL_PRINT("FAIL: ");
        TEST_SERIAL_PRINT(test_name);
        TEST_SERIAL_PRINT(" - Expected: ");
        TEST_SERIAL_PRINT(expected);
        TEST_SERIAL_PRINT(", Got: ");
        TEST_SERIAL_PRINTLN(actual);
        tests_failed++;
    } else {
        tests_passed++;
    }
    print_test_result(passed, test_name);
}

void TestConfidenceScorer::assert_equal(FallConfidence_t expected, FallConfidence_t actual, const char* test_name) {
    bool passed = (expected == actual);
    if (!passed) {
        TEST_SERIAL_PRINT("FAIL: ");
        TEST_SERIAL_PRINT(test_name);
        TEST_SERIAL_PRINT(" - Expected confidence: ");
        TEST_SERIAL_PRINT(expected);
        TEST_SERIAL_PRINT(", Got: ");
        TEST_SERIAL_PRINTLN(actual);
        tests_failed++;
    } else {
        tests_passed++;
    }
    print_test_result(passed, test_name);
}

void TestConfidenceScorer::assert_true(bool condition, const char* test_name) {
    if (!condition) {
        TEST_SERIAL_PRINT("FAIL: ");
        TEST_SERIAL_PRINTLN(test_name);
        tests_failed++;
    } else {
        tests_passed++;
    }
    print_test_result(condition, test_name);
}

void TestConfidenceScorer::print_test_result(bool passed, const char* test_name) {
    if (passed) {
        TEST_SERIAL_PRINT("PASS: ");
    } else {
        TEST_SERIAL_PRINT("FAIL: ");
    }
    TEST_SERIAL_PRINTLN(test_name);
}

bool TestConfidenceScorer::testScoreReset() {
    TEST_SERIAL_PRINTLN("Testing score reset...");

    // Add some scores first
    scorer.addStage1Score(500.0f, 0.1f);
    scorer.addStage2Score(5.0f, 300.0f, true);

    // Reset should clear everything
    scorer.resetScore();

    assert_equal(0, scorer.getTotalScore(), "Total score after reset");
    assert_equal(CONFIDENCE_NO_FALL, scorer.getConfidenceLevel(), "Confidence level after reset");
    assert_true(!scorer.isScoringActive(), "Scoring active flag after reset");

    return tests_failed == 0;
}

bool TestConfidenceScorer::testStage1Scoring() {
    TEST_SERIAL_PRINTLN("Testing Stage 1 (Free Fall) scoring...");
    scorer.resetScore();

    // Test minimum free fall scoring
    scorer.addStage1Score(100.0f, 0.5f);  // Brief, partial weightlessness
    uint8_t score1 = scorer.getStageScore(1);
    assert_true(score1 >= 5 && score1 <= 10, "Minimum free fall score");

    scorer.resetScore();

    // Test maximum free fall scoring
    scorer.addStage1Score(600.0f, 0.05f);  // Extended, true free fall
    uint8_t score2 = scorer.getStageScore(1);
    assert_true(score2 >= 20 && score2 <= 25, "Maximum free fall score");

    scorer.resetScore();

    // Test typical free fall scoring
    scorer.addStage1Score(300.0f, 0.2f);  // Typical fall parameters
    uint8_t score3 = scorer.getStageScore(1);
    assert_true(score3 >= 13 && score3 <= 18, "Typical free fall score");

    return true;
}

bool TestConfidenceScorer::testStage2Scoring() {
    TEST_SERIAL_PRINTLN("Testing Stage 2 (Impact) scoring...");
    scorer.resetScore();

    // Test minimum impact scoring
    scorer.addStage2Score(3.1f, 800.0f, false);  // Just above threshold, delayed, no FSR
    uint8_t score1 = scorer.getStageScore(2);
    assert_true(score1 >= 8 && score1 <= 12, "Minimum impact score");

    scorer.resetScore();

    // Test maximum impact scoring
    scorer.addStage2Score(7.0f, 200.0f, true);  // High impact, immediate, FSR detected
    uint8_t score2 = scorer.getStageScore(2);
    assert_true(score2 >= 20 && score2 <= 25, "Maximum impact score");

    scorer.resetScore();

    // Test typical impact scoring
    scorer.addStage2Score(4.5f, 400.0f, true);  // Moderate impact with FSR
    uint8_t score3 = scorer.getStageScore(2);
    assert_true(score3 >= 17 && score3 <= 22, "Typical impact score");

    return true;
}

bool TestConfidenceScorer::testStage3Scoring() {
    TEST_SERIAL_PRINTLN("Testing Stage 3 (Rotation) scoring...");
    scorer.resetScore();

    // Test minimum rotation scoring
    scorer.addStage3Score(260.0f, 30.0f);  // Just above threshold, minor orientation change
    uint8_t score1 = scorer.getStageScore(3);
    assert_true(score1 >= 8 && score1 <= 12, "Minimum rotation score");

    scorer.resetScore();

    // Test maximum rotation scoring
    scorer.addStage3Score(700.0f, 120.0f);  // High rotation, major orientation change
    uint8_t score2 = scorer.getStageScore(3);
    assert_true(score2 >= 18 && score2 <= 20, "Maximum rotation score");

    return true;
}

bool TestConfidenceScorer::testStage4Scoring() {
    TEST_SERIAL_PRINTLN("Testing Stage 4 (Inactivity) scoring...");
    scorer.resetScore();

    // Test minimum inactivity scoring
    scorer.addStage4Score(2100.0f, false);  // Just above threshold, not stable
    uint8_t score1 = scorer.getStageScore(4);
    assert_true(score1 >= 8 && score1 <= 12, "Minimum inactivity score");

    scorer.resetScore();

    // Test maximum inactivity scoring
    scorer.addStage4Score(12000.0f, true);  // Extended inactivity, very stable
    uint8_t score2 = scorer.getStageScore(4);
    assert_true(score2 >= 18 && score2 <= 20, "Maximum inactivity score");

    return true;
}

bool TestConfidenceScorer::testFilterScoring() {
    TEST_SERIAL_PRINTLN("Testing Filter scoring...");
    scorer.resetScore();

    // Test individual filter contributions
    scorer.addPressureFilterScore(1.8f);  // Moderate altitude change
    scorer.addHeartRateFilterScore(25.0f);  // Significant HR change
    scorer.addFSRFilterScore(true, true);   // Impact detected, strap secure

    uint8_t filter_score = scorer.getStageScore(5);
    assert_true(filter_score >= 8 && filter_score <= 15, "Combined filter score");

    return true;
}

bool TestConfidenceScorer::testTypicalFallScoring() {
    TEST_SERIAL_PRINTLN("Testing typical fall scenario scoring...");
    scorer.resetScore();

    // Simulate a typical fall sequence
    scorer.addStage1Score(350.0f, 0.15f);       // Good free fall detection
    scorer.addStage2Score(4.2f, 300.0f, true);  // Solid impact with FSR
    scorer.addStage3Score(420.0f, 85.0f);       // Good rotation
    scorer.addStage4Score(3500.0f, true);       // Extended inactivity
    scorer.addPressureFilterScore(1.5f);        // Altitude change
    scorer.addHeartRateFilterScore(20.0f);      // Stress response
    scorer.addFSRFilterScore(true, true);       // FSR validation

    uint8_t total_score = scorer.getTotalScore();
    FallConfidence_t confidence = scorer.getConfidenceLevel();

    assert_true(total_score >= 75 && total_score <= 90, "Typical fall total score");
    assert_true(confidence >= CONFIDENCE_CONFIRMED, "Typical fall confidence level");

    return true;
}

bool TestConfidenceScorer::testSevereFallScoring() {
    TEST_SERIAL_PRINTLN("Testing severe fall scenario scoring...");
    scorer.resetScore();

    // Simulate a severe fall sequence
    scorer.addStage1Score(500.0f, 0.08f);       // Extended free fall
    scorer.addStage2Score(6.8f, 200.0f, true);  // High impact
    scorer.addStage3Score(650.0f, 110.0f);      // Severe rotation
    scorer.addStage4Score(8000.0f, true);       // Very extended inactivity
    scorer.addPressureFilterScore(2.3f);        // Significant altitude change
    scorer.addHeartRateFilterScore(35.0f);      // Strong stress response
    scorer.addFSRFilterScore(true, true);       // FSR validation

    uint8_t total_score = scorer.getTotalScore();
    FallConfidence_t confidence = scorer.getConfidenceLevel();

    assert_true(total_score >= 85 && total_score <= 105, "Severe fall total score");
    assert_equal(CONFIDENCE_HIGH, confidence, "Severe fall confidence level");

    return true;
}

bool TestConfidenceScorer::testFalsePositiveScoring() {
    TEST_SERIAL_PRINTLN("Testing false positive scenario scoring...");
    scorer.resetScore();

    // Simulate a device drop (false positive)
    scorer.addStage1Score(150.0f, 0.3f);        // Brief, partial free fall
    scorer.addStage2Score(2.8f, 600.0f, false); // Low impact, delayed, no FSR
    scorer.addStage3Score(180.0f, 25.0f);       // Minimal rotation
    scorer.addStage4Score(800.0f, false);       // Brief inactivity, unstable
    scorer.addPressureFilterScore(0.3f);        // Minimal altitude change
    scorer.addHeartRateFilterScore(5.0f);       // Minimal HR change
    scorer.addFSRFilterScore(false, false);     // No FSR validation

    uint8_t total_score = scorer.getTotalScore();
    FallConfidence_t confidence = scorer.getConfidenceLevel();

    assert_true(total_score < 50, "False positive total score");
    assert_true(confidence <= CONFIDENCE_POTENTIAL, "False positive confidence level");

    return true;
}

bool TestConfidenceScorer::testBorderlineScoring() {
    TEST_SERIAL_PRINTLN("Testing borderline scenario scoring...");
    scorer.resetScore();

    // Simulate a borderline case
    scorer.addStage1Score(220.0f, 0.4f);        // Borderline free fall
    scorer.addStage2Score(3.2f, 700.0f, false); // Just above threshold
    scorer.addStage3Score(280.0f, 50.0f);       // Moderate rotation
    scorer.addStage4Score(2200.0f, true);       // Just above threshold inactivity
    scorer.addPressureFilterScore(0.8f);        // Moderate altitude change
    scorer.addHeartRateFilterScore(12.0f);      // Moderate HR change
    scorer.addFSRFilterScore(true, false);      // Partial FSR validation

    uint8_t total_score = scorer.getTotalScore();
    FallConfidence_t confidence = scorer.getConfidenceLevel();

    assert_true(total_score >= 45 && total_score <= 65, "Borderline total score");
    assert_equal(CONFIDENCE_POTENTIAL, confidence, "Borderline confidence level");

    return true;
}

bool TestConfidenceScorer::testMaximumScore() {
    TEST_SERIAL_PRINTLN("Testing maximum possible score...");
    scorer.resetScore();

    // Max out all scoring categories
    scorer.addStage1Score(800.0f, 0.05f);       // Max free fall
    scorer.addStage2Score(8.0f, 100.0f, true);  // Max impact
    scorer.addStage3Score(800.0f, 150.0f);      // Max rotation
    scorer.addStage4Score(15000.0f, true);      // Max inactivity
    scorer.addPressureFilterScore(3.0f);        // Max altitude change
    scorer.addHeartRateFilterScore(50.0f);      // Max HR change
    scorer.addFSRFilterScore(true, true);       // Max FSR validation

    uint8_t total_score = scorer.getTotalScore();

    assert_equal(105, total_score, "Maximum possible score");
    assert_equal(CONFIDENCE_HIGH, scorer.getConfidenceLevel(), "Maximum confidence level");

    return true;
}

bool TestConfidenceScorer::testMinimumScore() {
    TEST_SERIAL_PRINTLN("Testing minimum score conditions...");
    scorer.resetScore();

    // Don't add any scores - should remain at zero
    uint8_t total_score = scorer.getTotalScore();

    assert_equal(0, total_score, "Minimum possible score");
    assert_equal(CONFIDENCE_NO_FALL, scorer.getConfidenceLevel(), "Minimum confidence level");

    return true;
}

bool TestConfidenceScorer::testScoreValidation() {
    TEST_SERIAL_PRINTLN("Testing score validation and capping...");
    scorer.resetScore();

    // Test that individual stage scores are properly capped
    scorer.addStage1Score(1000.0f, 0.01f);  // Extreme values that should be capped
    uint8_t stage1_score = scorer.getStageScore(1);
    assert_true(stage1_score <= 25, "Stage 1 score properly capped");

    return true;
}

bool TestConfidenceScorer::runAllTests() {
    TEST_SERIAL_PRINTLN("========================================");
    TEST_SERIAL_PRINTLN("    CONFIDENCE SCORER TEST SUITE");
    TEST_SERIAL_PRINTLN("========================================");

    tests_passed = 0;
    tests_failed = 0;

    bool all_passed = true;

    all_passed &= testScoreReset();
    all_passed &= testStage1Scoring();
    all_passed &= testStage2Scoring();
    all_passed &= testStage3Scoring();
    all_passed &= testStage4Scoring();
    all_passed &= testFilterScoring();
    all_passed &= testTypicalFallScoring();
    all_passed &= testSevereFallScoring();
    all_passed &= testFalsePositiveScoring();
    all_passed &= testBorderlineScoring();
    all_passed &= testMaximumScore();
    all_passed &= testMinimumScore();
    all_passed &= testScoreValidation();

    printTestSummary();

    return all_passed;
}

void TestConfidenceScorer::printTestSummary() {
    TEST_SERIAL_PRINTLN("========================================");
    TEST_SERIAL_PRINTLN("      CONFIDENCE SCORER TEST RESULTS");
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
        TEST_SERIAL_PRINTLN("✓ ALL CONFIDENCE SCORER TESTS PASSED!");
    } else {
        TEST_SERIAL_PRINTLN("✗ SOME TESTS FAILED!");
    }
    TEST_SERIAL_PRINTLN("========================================");
}
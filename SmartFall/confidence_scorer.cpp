#include "detection/confidence_scorer.h"

ConfidenceScorer::ConfidenceScorer() : stage1_score(0), stage2_score(0), stage3_score(0),
                                       stage4_score(0), filter_score(0),
                                       scoring_active(false), scoring_start_time(0) {
    resetScore();
}

ConfidenceScorer::~ConfidenceScorer() {
    // Cleanup if needed
}

void ConfidenceScorer::resetScore() {
    stage1_score = 0;
    stage2_score = 0;
    stage3_score = 0;
    stage4_score = 0;
    filter_score = 0;

    // Reset detailed breakdowns
    stage1_breakdown = {0, 0};
    stage2_breakdown = {0, 0, 0};
    stage3_breakdown = {0, 0};
    stage4_breakdown = {0, 0};
    filter_breakdown = {0, 0, 0};

    scoring_active = false;
    scoring_start_time = 0;
}

void ConfidenceScorer::startScoring() {
    scoring_active = true;
    scoring_start_time = millis();
}

void ConfidenceScorer::addStage1Score(float duration_ms, float min_magnitude_g) {
    if (!scoring_active) startScoring();

    stage1_breakdown.duration_score = calculateDurationScore(duration_ms);
    stage1_breakdown.magnitude_score = calculateMagnitudeScore(min_magnitude_g);

    stage1_score = stage1_breakdown.duration_score + stage1_breakdown.magnitude_score;
    capScore(stage1_score, 25);

    if (DEBUG_ALGORITHM_STEPS) {
        Serial.print("Stage 1 Score: ");
        Serial.print(stage1_score);
        Serial.print("/25 (Duration: ");
        Serial.print(stage1_breakdown.duration_score);
        Serial.print(", Magnitude: ");
        Serial.print(stage1_breakdown.magnitude_score);
        Serial.println(")");
    }
}

void ConfidenceScorer::addStage2Score(float impact_g, float timing_ms, bool fsr_detected) {
    stage2_breakdown.impact_magnitude_score = calculateImpactScore(impact_g);
    stage2_breakdown.timing_score = calculateTimingScore(timing_ms);
    stage2_breakdown.fsr_validation_score = fsr_detected ? 7 : 0;

    stage2_score = stage2_breakdown.impact_magnitude_score +
                  stage2_breakdown.timing_score +
                  stage2_breakdown.fsr_validation_score;
    capScore(stage2_score, 25);

    if (DEBUG_ALGORITHM_STEPS) {
        Serial.print("Stage 2 Score: ");
        Serial.print(stage2_score);
        Serial.print("/25 (Impact: ");
        Serial.print(stage2_breakdown.impact_magnitude_score);
        Serial.print(", Timing: ");
        Serial.print(stage2_breakdown.timing_score);
        Serial.print(", FSR: ");
        Serial.print(stage2_breakdown.fsr_validation_score);
        Serial.println(")");
    }
}

void ConfidenceScorer::addStage3Score(float angular_velocity_dps, float orientation_change_deg) {
    stage3_breakdown.angular_velocity_score = calculateAngularScore(angular_velocity_dps);

    // Orientation change scoring
    if (orientation_change_deg >= 90.0f) {
        stage3_breakdown.orientation_change_score = 5;
    } else if (orientation_change_deg >= 45.0f) {
        stage3_breakdown.orientation_change_score = 3;
    } else {
        stage3_breakdown.orientation_change_score = 0;
    }

    stage3_score = stage3_breakdown.angular_velocity_score +
                  stage3_breakdown.orientation_change_score;
    capScore(stage3_score, 20);

    if (DEBUG_ALGORITHM_STEPS) {
        Serial.print("Stage 3 Score: ");
        Serial.print(stage3_score);
        Serial.print("/20 (Angular: ");
        Serial.print(stage3_breakdown.angular_velocity_score);
        Serial.print(", Orientation: ");
        Serial.print(stage3_breakdown.orientation_change_score);
        Serial.println(")");
    }
}

void ConfidenceScorer::addStage4Score(float inactivity_duration_ms, bool stable) {
    stage4_breakdown.inactivity_duration_score = calculateInactivityScore(inactivity_duration_ms);
    stage4_breakdown.stability_score = stable ? 5 : 0;

    stage4_score = stage4_breakdown.inactivity_duration_score +
                  stage4_breakdown.stability_score;
    capScore(stage4_score, 20);

    if (DEBUG_ALGORITHM_STEPS) {
        Serial.print("Stage 4 Score: ");
        Serial.print(stage4_score);
        Serial.print("/20 (Duration: ");
        Serial.print(stage4_breakdown.inactivity_duration_score);
        Serial.print(", Stability: ");
        Serial.print(stage4_breakdown.stability_score);
        Serial.println(")");
    }
}

void ConfidenceScorer::addPressureFilterScore(float altitude_change_m) {
    filter_breakdown.pressure_filter_score = calculatePressureScore(altitude_change_m);
    updateFilterScore();
}

void ConfidenceScorer::addHeartRateFilterScore(float hr_change_bpm) {
    filter_breakdown.heart_rate_filter_score = calculateHeartRateScore(hr_change_bpm);
    updateFilterScore();
}

void ConfidenceScorer::addFSRFilterScore(bool impact_detected, bool strap_secure) {
    uint8_t fsr_score = 0;
    if (strap_secure) fsr_score += 2;  // Device attached throughout sequence
    if (impact_detected) fsr_score += 3;  // Impact spike detected

    filter_breakdown.fsr_filter_score = fsr_score;
    updateFilterScore();
}

void ConfidenceScorer::updateFilterScore() {
    filter_score = filter_breakdown.pressure_filter_score +
                  filter_breakdown.heart_rate_filter_score +
                  filter_breakdown.fsr_filter_score;
    capScore(filter_score, 15);
}

uint8_t ConfidenceScorer::getTotalScore() {
    return stage1_score + stage2_score + stage3_score + stage4_score + filter_score;
}

FallConfidence_t ConfidenceScorer::getConfidenceLevel() {
    uint8_t total = getTotalScore();

    if (total >= HIGH_CONFIDENCE_THRESHOLD) {
        return CONFIDENCE_HIGH;
    } else if (total >= CONFIRMED_THRESHOLD) {
        return CONFIDENCE_CONFIRMED;
    } else if (total >= POTENTIAL_THRESHOLD) {
        return CONFIDENCE_POTENTIAL;
    } else if (total >= SUSPICIOUS_THRESHOLD) {
        return CONFIDENCE_SUSPICIOUS;
    } else {
        return CONFIDENCE_NO_FALL;
    }
}

uint8_t ConfidenceScorer::getStageScore(uint8_t stage_number) {
    switch(stage_number) {
        case 1: return stage1_score;
        case 2: return stage2_score;
        case 3: return stage3_score;
        case 4: return stage4_score;
        case 5: return filter_score;
        default: return 0;
    }
}

void ConfidenceScorer::getScoreBreakdown(uint8_t& s1, uint8_t& s2, uint8_t& s3, uint8_t& s4, uint8_t& filters) {
    s1 = stage1_score;
    s2 = stage2_score;
    s3 = stage3_score;
    s4 = stage4_score;
    filters = filter_score;
}

bool ConfidenceScorer::isValidFallSequence() {
    // Valid fall sequence requires minimum scores in key stages
    return (stage1_score >= 5) &&   // Minimum free fall detected
           (stage2_score >= 8) &&   // Minimum impact detected
           (getTotalScore() >= 30); // Overall minimum threshold
}

bool ConfidenceScorer::isScoringActive() {
    return scoring_active;
}

uint32_t ConfidenceScorer::getScoringDuration() {
    return scoring_active ? (millis() - scoring_start_time) : 0;
}

// Private calculation functions
uint8_t ConfidenceScorer::calculateDurationScore(float duration_ms) {
    if (duration_ms >= 500.0f) return 15;      // Extended fall
    if (duration_ms >= 200.0f) return 10;      // Typical fall
    if (duration_ms >= 100.0f) return 5;       // Brief drop
    return 0;
}

uint8_t ConfidenceScorer::calculateMagnitudeScore(float magnitude_g) {
    if (magnitude_g <= 0.1f) return 10;        // True free fall
    if (magnitude_g <= 0.3f) return 8;         // Significant weightlessness
    if (magnitude_g <= 0.5f) return 5;         // Partial weightlessness
    return 0;
}

uint8_t ConfidenceScorer::calculateImpactScore(float impact_g) {
    if (impact_g >= 6.0f) return 15;           // Severe impact
    if (impact_g >= 4.0f) return 12;           // Significant impact
    if (impact_g >= 3.0f) return 8;            // Moderate impact
    return 0;
}

uint8_t ConfidenceScorer::calculateTimingScore(float timing_ms) {
    if (timing_ms <= 500.0f) return 5;         // Immediate impact
    if (timing_ms <= 1000.0f) return 3;        // Delayed impact
    return 0;
}

uint8_t ConfidenceScorer::calculateAngularScore(float angular_velocity_dps) {
    if (angular_velocity_dps >= 600.0f) return 15;  // Severe rotation
    if (angular_velocity_dps >= 400.0f) return 12;  // Significant rotation
    if (angular_velocity_dps >= 250.0f) return 8;   // Moderate rotation
    return 0;
}

uint8_t ConfidenceScorer::calculateInactivityScore(float duration_ms) {
    if (duration_ms >= 10000.0f) return 15;    // Extended incapacitation
    if (duration_ms >= 5000.0f) return 12;     // Moderate incapacitation
    if (duration_ms >= 2000.0f) return 8;      // Brief incapacitation
    return 0;
}

uint8_t ConfidenceScorer::calculatePressureScore(float altitude_change_m) {
    if (altitude_change_m >= 2.0f) return 5;   // Significant fall height
    if (altitude_change_m >= 1.0f) return 3;   // Moderate fall height
    if (altitude_change_m >= 0.5f) return 2;   // Minor fall height
    return 0;
}

uint8_t ConfidenceScorer::calculateHeartRateScore(float hr_change_bpm) {
    float abs_change = abs(hr_change_bpm);
    if (abs_change >= 30.0f) return 5;         // Major stress response
    if (abs_change >= 10.0f) return 3;         // Moderate stress response
    if (abs_change >= 2.0f) return 2;          // Minor stress response
    return 0;
}

bool ConfidenceScorer::validateScoreRange(uint8_t score, uint8_t max_score) {
    return score <= max_score;
}

void ConfidenceScorer::capScore(uint8_t& score, uint8_t max_value) {
    if (score > max_value) {
        score = max_value;
    }
}

const char* ConfidenceScorer::getConfidenceString(FallConfidence_t confidence) {
    switch(confidence) {
        case CONFIDENCE_HIGH: return "HIGH";
        case CONFIDENCE_CONFIRMED: return "CONFIRMED";
        case CONFIDENCE_POTENTIAL: return "POTENTIAL";
        case CONFIDENCE_SUSPICIOUS: return "SUSPICIOUS";
        case CONFIDENCE_NO_FALL: return "NO_FALL";
        default: return "UNKNOWN";
    }
}

void ConfidenceScorer::printScoreBreakdown() {
    Serial.println("=== Confidence Score Breakdown ===");
    Serial.print("Stage 1 (Free Fall): ");
    Serial.print(stage1_score);
    Serial.println("/25");

    Serial.print("Stage 2 (Impact): ");
    Serial.print(stage2_score);
    Serial.println("/25");

    Serial.print("Stage 3 (Rotation): ");
    Serial.print(stage3_score);
    Serial.println("/20");

    Serial.print("Stage 4 (Inactivity): ");
    Serial.print(stage4_score);
    Serial.println("/20");

    Serial.print("Filters: ");
    Serial.print(filter_score);
    Serial.println("/15");

    Serial.print("TOTAL SCORE: ");
    Serial.print(getTotalScore());
    Serial.print("/105 - ");
    Serial.println(getConfidenceString(getConfidenceLevel()));
    Serial.println("===================================");
}

void ConfidenceScorer::printDetailedAnalysis() {
    Serial.println("=== Detailed Fall Analysis ===");

    Serial.println("Stage 1 - Free Fall:");
    Serial.print("  Duration Score: ");
    Serial.print(stage1_breakdown.duration_score);
    Serial.print(", Magnitude Score: ");
    Serial.println(stage1_breakdown.magnitude_score);

    Serial.println("Stage 2 - Impact:");
    Serial.print("  Impact Score: ");
    Serial.print(stage2_breakdown.impact_magnitude_score);
    Serial.print(", Timing Score: ");
    Serial.print(stage2_breakdown.timing_score);
    Serial.print(", FSR Score: ");
    Serial.println(stage2_breakdown.fsr_validation_score);

    Serial.println("Stage 3 - Rotation:");
    Serial.print("  Angular Score: ");
    Serial.print(stage3_breakdown.angular_velocity_score);
    Serial.print(", Orientation Score: ");
    Serial.println(stage3_breakdown.orientation_change_score);

    Serial.println("Stage 4 - Inactivity:");
    Serial.print("  Duration Score: ");
    Serial.print(stage4_breakdown.inactivity_duration_score);
    Serial.print(", Stability Score: ");
    Serial.println(stage4_breakdown.stability_score);

    Serial.println("Filters:");
    Serial.print("  Pressure: ");
    Serial.print(filter_breakdown.pressure_filter_score);
    Serial.print(", Heart Rate: ");
    Serial.print(filter_breakdown.heart_rate_filter_score);
    Serial.print(", FSR: ");
    Serial.println(filter_breakdown.fsr_filter_score);

    Serial.println("===============================");
}
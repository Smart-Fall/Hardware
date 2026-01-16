/*
 * MAX30102 Heart Rate & SpO2 Sensor Test
 * ESP32 Feather V2
 *
 * Wiring:
 * MAX30102 VCC -> 3.3V
 * MAX30102 GND -> GND
 * MAX30102 SDA -> GPIO 22
 * MAX30102 SCL -> GPIO 20
 * MAX30102 RST -> GPIO 21 (A9)
 *
 * Instructions:
 * 1. Upload this sketch to your ESP32
 * 2. Open Serial Monitor (115200 baud)
 * 3. Place your finger on the sensor
 * 4. Wait 10-15 seconds for readings to stabilize
 * 5. Observe heart rate (BPM) and SpO2 (%) values
 */

#include "MAX30102_Sensor.h"

MAX30102Sensor heartRateSensor;

// Test state variables
uint32_t lastReadTime = 0;
uint32_t readCount = 0;
uint16_t minHeartRate = 255;
uint16_t maxHeartRate = 0;
uint8_t minSpO2 = 255;
uint8_t maxSpO2 = 0;

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n========================================");
    Serial.println("    MAX30102 Heart Rate & SpO2 Test");
    Serial.println("========================================\n");

    Serial.println("Initializing MAX30102 sensor...");

    if (!heartRateSensor.begin(0x57))
    {
        Serial.println("ERROR: MAX30102 initialization failed!");
        Serial.println("Troubleshooting:");
        Serial.println("  1. Check I2C wiring (SDA=GPIO22, SCL=GPIO20)");
        Serial.println("  2. Verify 3.3V power connection");
        Serial.println("  3. Try different I2C address if needed");
        while (1)
            delay(1000);
    }

    Serial.println("✓ MAX30102 initialized successfully");

    heartRateSensor.configure();
    heartRateSensor.printInfo();

    Serial.println("\n========================================");
    Serial.println("     Ready to measure heart rate!");
    Serial.println("========================================");
    Serial.println("\nInstructions:");
    Serial.println("1. Place your finger firmly on the sensor");
    Serial.println("2. Keep finger still for accurate readings");
    Serial.println("3. Wait 10-15 seconds for stabilization");
    Serial.println("4. Remove finger slowly when done\n");
    Serial.println("Sensor will update every 4 seconds.\n");
}

void loop()
{
    uint32_t currentTime = millis();

    // Try to read every 4 seconds (sensor update rate)
    if (currentTime - lastReadTime >= 4000)
    {
        lastReadTime = currentTime;

        uint16_t heartRate;
        uint8_t spO2;
        float temperature;

        // Attempt to read sensor data
        if (heartRateSensor.readData(heartRate, spO2, temperature))
        {
            readCount++;

            // Update min/max values
            if (heartRate > 0)
            {
                if (heartRate < minHeartRate)
                    minHeartRate = heartRate;
                if (heartRate > maxHeartRate)
                    maxHeartRate = heartRate;
            }
            if (spO2 > 0)
            {
                if (spO2 < minSpO2)
                    minSpO2 = spO2;
                if (spO2 > maxSpO2)
                    maxSpO2 = spO2;
            }

            // Print formatted data
            Serial.println("========== Reading #" + String(readCount) + " ==========");
            Serial.print("Heart Rate: ");
            Serial.print(heartRate);
            Serial.println(" BPM");

            Serial.print("SpO2:       ");
            Serial.print(spO2);
            Serial.println(" %");

            Serial.print("Temp:       ");
            Serial.print(temperature, 1);
            Serial.println(" °C");

            // Status indicators
            Serial.print("Status:     ");
            if (heartRate >= 60 && heartRate <= 100)
            {
                Serial.print("✓ Normal heart rate");
            }
            else if (heartRate < 60)
            {
                Serial.print("⚠ Low heart rate (bradycardia)");
            }
            else if (heartRate > 100)
            {
                Serial.print("⚠ High heart rate (tachycardia)");
            }
            else
            {
                Serial.print("⚠ Abnormal reading");
            }
            Serial.println();

            Serial.print("O2 Status:  ");
            if (spO2 >= 95)
            {
                Serial.print("✓ Healthy oxygen level");
            }
            else if (spO2 >= 90)
            {
                Serial.print("⚠ Low oxygen (borderline)");
            }
            else
            {
                Serial.print("⚠ Critical oxygen level");
            }
            Serial.println();

            // Running statistics
            if (readCount > 1)
            {
                Serial.print("HR Stats:   Min=");
                Serial.print(minHeartRate);
                Serial.print(" Max=");
                Serial.println(maxHeartRate);

                Serial.print("SpO2 Stats: Min=");
                Serial.print(minSpO2);
                Serial.print(" Max=");
                Serial.println(maxSpO2);
            }

            Serial.println();
        }
        else
        {
            Serial.println("⚠ No new data available (sensor still stabilizing...)");
            Serial.println("  Place finger on sensor and wait\n");
        }
    }

    // Check for serial commands
    if (Serial.available())
    {
        char cmd = Serial.read();
        switch (cmd)
        {
        case 'r':
        case 'R':
            // Reset statistics
            readCount = 0;
            minHeartRate = 255;
            maxHeartRate = 0;
            minSpO2 = 255;
            maxSpO2 = 0;
            Serial.println("\n✓ Statistics reset!\n");
            break;

        case 'i':
        case 'I':
            // Print info
            heartRateSensor.printInfo();
            break;

        case 'h':
        case 'H':
            // Print help
            printHelp();
            break;

        default:
            if (cmd != '\n' && cmd != '\r')
            {
                Serial.println("Unknown command. Type 'h' for help.");
            }
        }
    }
}

void printHelp()
{
    Serial.println("\n========== Command Help ==========");
    Serial.println("r - Reset statistics (min/max values)");
    Serial.println("i - Print sensor information");
    Serial.println("h - Show this help message");
    Serial.println("==================================\n");
}

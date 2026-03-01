#include "MAX30102_Sensor.h"
#include "Board_Config.h"

#ifdef MAX30102_USE_UART
MAX30102Sensor::MAX30102Sensor(uint8_t rx, uint8_t tx)
    : heartRateSensor(&Serial1, MAX30102_UART_BAUD),
      initialized(false),
      rx_pin(rx == 255 ? MAX30102_UART_RX_PIN : rx),
      tx_pin(tx == 255 ? MAX30102_UART_TX_PIN : tx),
      buffer_index(0), last_read_time(0), baseline_heart_rate(60), baseline_spo2(95), baseline_set(false)
{
    // Initialize buffers
    memset(heart_rate_buffer, 0, sizeof(heart_rate_buffer));
    memset(spo2_buffer, 0, sizeof(spo2_buffer));
}
#else
MAX30102Sensor::MAX30102Sensor(uint8_t sda, uint8_t scl)
    : heartRateSensor(&Wire, MAX30102_I2C_ADDRESS), initialized(false),
      sda_pin(sda == 255 ? Board_Config::getSDA() : sda),
      scl_pin(scl == 255 ? Board_Config::getSCL() : scl),
      buffer_index(0), last_read_time(0), baseline_heart_rate(60), baseline_spo2(95), baseline_set(false)
{
    // Initialize buffers
    memset(heart_rate_buffer, 0, sizeof(heart_rate_buffer));
    memset(spo2_buffer, 0, sizeof(spo2_buffer));
}
#endif

bool MAX30102Sensor::begin(uint8_t address)
{
    #ifdef MAX30102_USE_UART
        // Initialize UART
        Serial1.begin(MAX30102_UART_BAUD, SERIAL_8N1, rx_pin, tx_pin);
        Serial.println("========================================");
        Serial.println("  MAX30102 UART Mode (Modbus RTU)");
        Serial.println("========================================");
        Serial.print("RX Pin: GPIO ");
        Serial.println(rx_pin);
        Serial.print("TX Pin: GPIO ");
        Serial.println(tx_pin);
        Serial.print("Baud:   ");
        Serial.println(MAX30102_UART_BAUD);
        Serial.println("========================================\n");
        delay(100);
    #else
        // Initialize I2C
        if (!Wire.begin(sda_pin, scl_pin))
        {
            Serial.println("ERROR: Failed to initialize I2C for MAX30102");
            return false;
        }
        Serial.println("MAX30102 I2C Mode");
    #endif

    // Initialize sensor with retries
    for (uint8_t attempt = 0; attempt < MAX_RETRIES; attempt++)
    {
        if (heartRateSensor.begin())
        {
            initialized = true;
            #ifdef MAX30102_USE_UART
                Serial.println("[MAX30102] ✓ Initialized (UART mode)");
            #else
                Serial.println("[MAX30102] ✓ Initialized (I2C mode)");
            #endif

            startCollection();
            delay(100);
            return true;
        }
        if (attempt < MAX_RETRIES - 1) {
            Serial.printf("[MAX30102] Init retry %d/%d\n", attempt + 1, MAX_RETRIES);
            delay(RETRY_DELAY_MS);
        }
    }

    #ifdef MAX30102_USE_UART
        Serial.println("ERROR: MAX30102 init failed (UART mode)");
        Serial.println("Check: UART wiring, power, sensor mode, baud rate");
    #else
        Serial.println("ERROR: MAX30102 init failed (I2C mode)");
        Serial.println("Check: I2C wiring, power, address 0x57");
    #endif

    initialized = false;
    return false;
}

void MAX30102Sensor::configure()
{
    if (!initialized)
        return;

    // Sensor starts data collection automatically via startCollection()
    Serial.println("MAX30102 configured");
}

bool MAX30102Sensor::readData(uint16_t &heart_rate, uint8_t &spo2, float &temperature)
{
    if (!initialized)
        return false;

    uint32_t currentTime = millis();

    // Sensor updates every 4 seconds per documentation
    if (currentTime - last_read_time < 4000)
    {
        return false; // No new data yet
    }

    last_read_time = currentTime;

    // Read sensor data with retries
    for (uint8_t attempt = 0; attempt < MAX_RETRIES; attempt++)
    {
        try
        {
            heartRateSensor.getHeartbeatSPO2();

            heart_rate = heartRateSensor._sHeartbeatSPO2.Heartbeat;
            spo2 = heartRateSensor._sHeartbeatSPO2.SPO2;
            temperature = heartRateSensor.getTemperature_C();

            // Check for stale data (same heart rate = sensor frozen)
            if (heart_rate == last_heart_rate && heart_rate != 0) {
                stale_count++;
                if (stale_count >= STALE_THRESHOLD) {
                    Serial.printf("[MAX30102] Stale data detected after %d identical reads, resetting sensor...\n", STALE_THRESHOLD);
                    stale_count = 0;
                    // Reset by reinitializing
                    begin();
                    return false;  // Discard this read, retry on next call
                }
            } else {
                stale_count = 0;  // Reset counter if data changed
                last_heart_rate = heart_rate;
            }

            // Validate ranges (typical healthy values)
            if (heart_rate < 30 || heart_rate > 200)
            {
                Serial.print("[MAX30102] WARNING: Heart rate out of range: ");
                Serial.println(heart_rate);
                return false;
            }

            if (spo2 < 80 || spo2 > 100)
            {
                Serial.print("[MAX30102] WARNING: SpO2 out of range: ");
                Serial.println(spo2);
                return false;
            }

            // Buffer values for stability
            heart_rate_buffer[buffer_index] = heart_rate;
            spo2_buffer[buffer_index] = spo2;
            buffer_index = (buffer_index + 1) % 5;

            return true;
        }
        catch (...)
        {
            if (attempt < MAX_RETRIES - 1)
            {
                Serial.printf("[MAX30102] Read error on attempt %d/%d, retrying...\n", attempt + 1, MAX_RETRIES);
                delay(RETRY_DELAY_MS);
            }
            else
            {
                Serial.printf("[MAX30102] Read error - all %d retry attempts failed\n", MAX_RETRIES);
                return false;
            }
        }
    }
    return false;
}

bool MAX30102Sensor::getRawData(uint16_t &heart_rate, uint8_t &spo2)
{
    if (!initialized)
        return false;

    for (uint8_t attempt = 0; attempt < MAX_RETRIES; attempt++)
    {
        try
        {
            heartRateSensor.getHeartbeatSPO2();

            heart_rate = heartRateSensor._sHeartbeatSPO2.Heartbeat;
            spo2 = heartRateSensor._sHeartbeatSPO2.SPO2;

            return true;
        }
        catch (...)
        {
            if (attempt < MAX_RETRIES - 1)
            {
                Serial.printf("[MAX30102] getRawData error on attempt %d/%d, retrying...\n", attempt + 1, MAX_RETRIES);
                delay(RETRY_DELAY_MS);
            }
            else
            {
                Serial.printf("[MAX30102] getRawData error - all %d retry attempts failed\n", MAX_RETRIES);
                return false;
            }
        }
    }
    return false;
}

float MAX30102Sensor::getTemperature()
{
    if (!initialized)
        return -1.0f;

    for (uint8_t attempt = 0; attempt < MAX_RETRIES; attempt++)
    {
        try
        {
            return heartRateSensor.getTemperature_C();
        }
        catch (...)
        {
            if (attempt < MAX_RETRIES - 1)
            {
                Serial.printf("[MAX30102] getTemperature error on attempt %d/%d, retrying...\n", attempt + 1, MAX_RETRIES);
                delay(RETRY_DELAY_MS);
            }
            else
            {
                Serial.printf("[MAX30102] getTemperature error - all %d retry attempts failed\n", MAX_RETRIES);
                return -1.0f;
            }
        }
    }
    return -1.0f;
}

void MAX30102Sensor::setBaselineHeartRate(uint16_t baseline)
{
    baseline_heart_rate = baseline;
    baseline_set = true;
}

void MAX30102Sensor::setBaselineSPO2(uint8_t baseline)
{
    baseline_spo2 = baseline;
}

uint16_t MAX30102Sensor::getBaselineHeartRate()
{
    return baseline_heart_rate;
}

uint8_t MAX30102Sensor::getBaselineSPO2()
{
    return baseline_spo2;
}

void MAX30102Sensor::resetBaseline()
{
    baseline_heart_rate = 60;
    baseline_spo2 = 95;
    baseline_set = false;
    Serial.println("MAX30102 baseline reset");
}

void MAX30102Sensor::startCollection()
{
    if (!initialized)
        return;

    heartRateSensor.sensorStartCollect();
    Serial.println("MAX30102 data collection started");
}

void MAX30102Sensor::stopCollection()
{
    if (!initialized)
        return;

    heartRateSensor.sensorEndCollect();
    Serial.println("MAX30102 data collection stopped");
}

bool MAX30102Sensor::isInitialized()
{
    return initialized;
}

void MAX30102Sensor::printInfo()
{
    if (!initialized)
    {
        Serial.println("MAX30102 sensor not initialized");
        return;
    }

    Serial.println("\n--- MAX30102 Sensor Info ---");

    #ifdef MAX30102_USE_UART
        Serial.println("Mode: UART (Modbus RTU)");
        Serial.print("Pins: RX=GPIO ");
        Serial.print(rx_pin);
        Serial.print(", TX=GPIO ");
        Serial.println(tx_pin);
        Serial.print("Baud: ");
        Serial.println(MAX30102_UART_BAUD);
    #else
        Serial.println("Mode: I2C");
        Serial.print("Pins: SDA=GPIO ");
        Serial.print(sda_pin);
        Serial.print(", SCL=GPIO ");
        Serial.println(scl_pin);
    #endif

    Serial.print("Baseline HR: ");
    Serial.print(baseline_heart_rate);
    Serial.println(" BPM");
    Serial.print("Baseline SpO2: ");
    Serial.print(baseline_spo2);
    Serial.println(" %");
    Serial.println("----------------------------\n");
}

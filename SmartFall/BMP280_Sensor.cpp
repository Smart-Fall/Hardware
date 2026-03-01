#include "BMP280_Sensor.h"
#include "Board_Config.h"

BMP280_Sensor::BMP280_Sensor(uint8_t sda, uint8_t scl)
    : initialized(false),
      sda_pin(sda == 255 ? Board_Config::getSDA() : sda),
      scl_pin(scl == 255 ? Board_Config::getSCL() : scl),
      baselineAltitude(0.0), seaLevelPressure(1013.25)
{
}

bool BMP280_Sensor::begin(uint8_t address)
{
    // Try primary address with retries
    for (uint8_t attempt = 0; attempt < MAX_RETRIES; attempt++)
    {
        if (bmp.begin(address))
        {
            initialized = true;
            return true;
        }
        if (attempt < MAX_RETRIES - 1)
        {
            Serial.printf("[BMP280] Init retry %d/%d for address 0x%02X\n", attempt + 1, MAX_RETRIES, address);
            delay(RETRY_DELAY_MS);
        }
    }

    // Try alternate address with retries
    uint8_t alt_address = (address == 0x76) ? 0x77 : 0x76;
    for (uint8_t attempt = 0; attempt < MAX_RETRIES; attempt++)
    {
        if (bmp.begin(alt_address))
        {
            initialized = true;
            Serial.printf("[BMP280] Initialized at alternate address 0x%02X\n", alt_address);
            return true;
        }
        if (attempt < MAX_RETRIES - 1)
        {
            Serial.printf("[BMP280] Init retry %d/%d for address 0x%02X\n", attempt + 1, MAX_RETRIES, alt_address);
            delay(RETRY_DELAY_MS);
        }
    }

    Serial.println("[BMP280] Failed to initialize - no sensor detected at 0x76 or 0x77");
    return false;
}

void BMP280_Sensor::configure()
{
    if (!initialized)
        return;

    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X2,
                    Adafruit_BMP280::SAMPLING_X16,
                    Adafruit_BMP280::FILTER_X16,
                    Adafruit_BMP280::STANDBY_MS_1);
}

void BMP280_Sensor::setSeaLevelPressure(float pressure_hPa)
{
    seaLevelPressure = pressure_hPa;
}

void BMP280_Sensor::resetBaselineAltitude()
{
    if (!initialized)
        return;

    baselineAltitude = bmp.readAltitude(seaLevelPressure);
    Serial.print("Baseline altitude set to: ");
    Serial.print(baselineAltitude, 2);
    Serial.println(" m");
}

bool BMP280_Sensor::readData(float &temperature, float &pressure, float &altitude)
{
    if (!initialized)
        return false;

    for (uint8_t attempt = 0; attempt < MAX_RETRIES; attempt++)
    {
        try
        {
            temperature = bmp.readTemperature();
            pressure = bmp.readPressure() / 100.0; // Pa to hPa
            altitude = bmp.readAltitude(seaLevelPressure);

            // Check for stale data (same pressure = sensor frozen)
            // Use epsilon comparison — float equality is unreliable for ADC noise
            if (fabsf(pressure - last_pressure) < 0.01f) {
                stale_count++;
                if (stale_count >= STALE_THRESHOLD) {
                    Serial.printf("[BMP280] Stale data detected after %d identical reads, resetting sensor...\n", STALE_THRESHOLD);
                    stale_count = 0;
                    // Reset by reinitializing
                    begin();
                    return false;  // Discard this read, retry on next call
                }
            } else {
                stale_count = 0;  // Reset counter if data changed
                last_pressure = pressure;
            }

            return true;
        }
        catch (...)
        {
            // I2C communication error occurred
            if (attempt < MAX_RETRIES - 1)
            {
                Serial.printf("[BMP280] I2C error on attempt %d/%d, retrying...\n", attempt + 1, MAX_RETRIES);
                delay(RETRY_DELAY_MS);
            }
            else
            {
                Serial.printf("[BMP280] I2C error - all %d retry attempts failed\n", MAX_RETRIES);
                return false;
            }
        }
    }

    return false;
}

float BMP280_Sensor::getAltitudeChange()
{
    if (!initialized)
        return 0.0;

    float current_altitude = bmp.readAltitude(seaLevelPressure);
    return current_altitude - baselineAltitude;
}

bool BMP280_Sensor::isInitialized()
{
    return initialized;
}

void BMP280_Sensor::printInfo()
{
    if (!initialized)
    {
        Serial.println("BMP280 not initialized");
        return;
    }

    Serial.println("=== BMP280 Info ===");
    Serial.println("Mode: NORMAL");
    Serial.println("Pressure oversampling: X16");
    Serial.println("Temperature oversampling: X2");
    Serial.println("Filter: X16");
}

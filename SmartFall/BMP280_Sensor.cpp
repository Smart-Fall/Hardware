#include "BMP280_Sensor.h"
#include "Board_Config.h"

BMP280_Sensor::BMP280_Sensor(uint8_t sda, uint8_t scl)
    : initialized(false),
      i2c_address(0x76),
      sda_pin(sda == 255 ? Board_Config::getSDA() : sda),
      scl_pin(scl == 255 ? Board_Config::getSCL() : scl),
      baselineAltitude(0.0), seaLevelPressure(1013.25)
{
}

bool BMP280_Sensor::begin(uint8_t address)
{
    // Wire.begin() is called once globally in initializeSensors().
    // Re-calling it here would reset the I2C bus and break all other sensors.
    initialized = false;

    // Try both addresses with limited retries. Each failed I2C transaction
    // incurs a wire timeout; too many back-to-back attempts at 80MHz CPU
    // can trigger the ESP32 Task Watchdog (TG1WDT_SYS_RESET).
    static const uint8_t INIT_RETRIES = 3;
    uint8_t addresses[2] = {address, (uint8_t)((address == 0x76) ? 0x77 : 0x76)};

    for (uint8_t a = 0; a < 2; a++)
    {
        for (uint8_t attempt = 0; attempt < INIT_RETRIES; attempt++)
        {
            yield(); // Feed watchdog between I2C transactions
            if (bmp.begin(addresses[a]))
            {
                initialized = true;
                i2c_address = addresses[a];
                if (a > 0)
                    Serial.printf("[BMP280] Initialized at alternate address 0x%02X\n", addresses[a]);
                return true;
            }
            if (attempt < INIT_RETRIES - 1)
            {
                Serial.printf("[BMP280] Init retry %d/%d for address 0x%02X\n", attempt + 1, INIT_RETRIES, addresses[a]);
                delay(RETRY_DELAY_MS);
            }
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

    Wire.beginTransmission(i2c_address);
    if (Wire.endTransmission() != 0)
    {
        initialized = false;
        return false;
    }

    temperature = bmp.readTemperature();
    pressure = bmp.readPressure() / 100.0; // Pa to hPa
    altitude = bmp.readAltitude(seaLevelPressure);

    if (!isfinite(temperature) || !isfinite(pressure) || !isfinite(altitude) || pressure <= 0.0f)
    {
        initialized = false;
        return false;
    }

    return true;
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

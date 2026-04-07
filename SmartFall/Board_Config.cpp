#include "Board_Config.h"

// Static member initialization
BoardConfig Board_Config::currentConfig = {BOARD_UNKNOWN, "Unknown", 22, 20, CHIP_ESP32};
bool Board_Config::initialized = false;

// Initialize board detection
void Board_Config::begin()
{
  if (initialized)
  {
    Serial.println("[Board_Config] Already initialized");
    return;
  }

  Serial.println("\n=== Board Detection ===");
  currentConfig = detectBoard();
  initialized = true;

  printBoardInfo();
}

// Detect board type based on chip model
BoardConfig Board_Config::detectBoard()
{
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  BoardConfig config;
  config.chip_model = chip_info.model;

  switch (chip_info.model)
  {
  case CHIP_ESP32:
    // Adafruit ESP32 Feather V2 uses a plain ESP32 (not S3)
    config.type = BOARD_ESP32_FEATHER_V2;
    config.name = "ESP32 Feather V2";
    config.sda_pin = 22;
    config.scl_pin = 20;
    break;

  default:
    // Unsupported board - error message
    config.type = BOARD_UNKNOWN;
    config.name = "Unknown Board";
    config.sda_pin = 22;
    config.scl_pin = 20;
    Serial.println("[Board_Config] ERROR: This project only supports ESP32 Feather V2!");
    Serial.printf("[Board_Config] Detected chip model: %d\n", chip_info.model);
    Serial.println("[Board_Config] Please use an Adafruit ESP32 Feather V2 board");
    break;
  }

  return config;
}

// Get auto-detected SDA pin
uint8_t Board_Config::getSDA()
{
  if (!initialized)
  {
    Serial.println("[Board_Config] ERROR: Board_Config::begin() not called!");
    Serial.println("[Board_Config] Please call Board_Config::begin() in setup() before using sensors");
    return 22; // Default fallback
  }
  return currentConfig.sda_pin;
}

// Get auto-detected SCL pin
uint8_t Board_Config::getSCL()
{
  if (!initialized)
  {
    Serial.println("[Board_Config] ERROR: Board_Config::begin() not called!");
    Serial.println("[Board_Config] Please call Board_Config::begin() in setup() before using sensors");
    return 20; // Default fallback
  }
  return currentConfig.scl_pin;
}

// Get detected board type
BoardType Board_Config::getBoardType()
{
  return currentConfig.type;
}

// Get board name
const char *Board_Config::getBoardName()
{
  return currentConfig.name;
}

// Get chip model
esp_chip_model_t Board_Config::getChipModel()
{
  return currentConfig.chip_model;
}

// Manual override for I2C pins
void Board_Config::setI2CPins(uint8_t sda, uint8_t scl)
{
  currentConfig.sda_pin = sda;
  currentConfig.scl_pin = scl;
  Serial.printf("[Board_Config] Manual override: SDA=GPIO %d, SCL=GPIO %d\n", sda, scl);
}

// Print board information
void Board_Config::printBoardInfo()
{
  Serial.println("Board detected: " + String(currentConfig.name));

  switch (currentConfig.chip_model)
  {
  case CHIP_ESP32:
    Serial.println("Chip: ESP32");
    break;
  case CHIP_ESP32S3:
    Serial.println("Chip: ESP32-S3");
    break;
  case CHIP_ESP32S2:
    Serial.println("Chip: ESP32-S2");
    break;
  case CHIP_ESP32C3:
    Serial.println("Chip: ESP32-C3");
    break;
  default:
    Serial.printf("Chip: Unknown (%d)\n", currentConfig.chip_model);
    break;
  }

  Serial.printf("I2C Pins: SDA=GPIO %d, SCL=GPIO %d\n", currentConfig.sda_pin, currentConfig.scl_pin);
  Serial.println("=======================\n");
}

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include <Arduino.h>
#include <esp_chip_info.h>

// Board type enumeration
enum BoardType {
  BOARD_ESP32_FEATHER_V2,  // ESP32-S3 chip, SDA=GPIO 22, SCL=GPIO 20
  BOARD_UNKNOWN            // Unknown board type
};

// Board configuration structure
struct BoardConfig {
  BoardType type;
  const char* name;
  uint8_t sda_pin;
  uint8_t scl_pin;
  esp_chip_model_t chip_model;
};

// Board configuration utility class
class Board_Config {
public:
  // Initialize board detection and configure pins
  static void begin();

  // Get auto-detected I2C pins
  static uint8_t getSDA();
  static uint8_t getSCL();

  // Get detected board information
  static BoardType getBoardType();
  static const char* getBoardName();
  static esp_chip_model_t getChipModel();

  // Manual override for edge cases
  static void setI2CPins(uint8_t sda, uint8_t scl);

  // Debug output
  static void printBoardInfo();

private:
  static BoardConfig detectBoard();
  static BoardConfig currentConfig;
  static bool initialized;
};

#endif // BOARD_CONFIG_H

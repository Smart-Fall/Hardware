// Wokwi Custom Chip - For information and examples see:
// https://docs.wokwi.com/chips-api/getting-started
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2022 Uri Shaked / wokwi.com

#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

const int ADDRESS = 0x22;

typedef struct {
  pin_t pin_int;
  uint8_t counter;
  uint32_t threshold_attr;
} chip_state_t;

static bool on_i2c_connect(void *user_data, uint32_t address, bool connect);
static uint8_t on_i2c_read(void *user_data);
static bool on_i2c_write(void *user_data, uint8_t data);
static void on_i2c_disconnect(void *user_data);

void chip_init() {
  chip_state_t *chip = malloc(sizeof(chip_state_t));

  chip->pin_int = pin_init("INT", INPUT);
  chip->counter = 0;

  const i2c_config_t i2c_config = {
    .user_data = chip,
    .address = ADDRESS,
    .scl = pin_init("SCL", INPUT),
    .sda = pin_init("SDA", INPUT),
    .connect = on_i2c_connect,
    .read = on_i2c_read,
    .write = on_i2c_write,
    .disconnect = on_i2c_disconnect, // Optional
  };
  i2c_init(&i2c_config);

  // This attribute can be edited by the user. It's defined in wokwi-custom-part.json:
  chip->threshold_attr = attr_init("threshold", 127);

  // The following message will appear in the browser's DevTools console:
  printf("Hello from custom chip!\n");
}

static void counter_updated(chip_state_t *chip) {
  const uint32_t threshold = attr_read(chip->threshold_attr);
  if (chip->counter > threshold) {
    pin_write(chip->pin_int, LOW);
    pin_mode(chip->pin_int, OUTPUT);
  } else {
    pin_mode(chip->pin_int, INPUT);
  }
  printf("counter_updated");
}

bool on_i2c_connect(void *user_data, uint32_t address, bool connect) {
  return true; /* Ack */
  printf("i2c_connect");
}

uint8_t on_i2c_read(void *user_data) {
  chip_state_t *chip = user_data;
  chip->counter++;
  counter_updated(chip);
  printf("i2c_read");
  return chip->counter;
}

bool on_i2c_write(void *user_data, uint8_t data) {
  chip_state_t *chip = user_data;
  chip->counter = data;
  printf("i2c_write");
  counter_updated(chip);
  return true; // Ack
}

void on_i2c_disconnect(void *user_data) {
  // Do nothing
}
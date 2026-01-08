#pragma once

#include "config.h"
#include "globals.h"
#include <EEPROM.h>

inline void loadSettings() {
  EEPROM.begin(EEPROM_SIZE);
  if (EEPROM.read(ADDR_MAGIC) == MAGIC_VALUE) {
    fireMode = constrain(EEPROM.read(ADDR_MODE), 0, MODE_COUNT - 1);
    flickerSpeed = constrain(EEPROM.read(ADDR_SPEED), 5, 100);
    maxBrightness = constrain(EEPROM.read(ADDR_BRIGHTNESS), 10, 255);
    int savedLeds = EEPROM.read(ADDR_NUM_LEDS_L) | (EEPROM.read(ADDR_NUM_LEDS_H) << 8);
    if (savedLeds >= 1 && savedLeds <= MAX_LEDS) {
      numLeds = savedLeds;
    }
    Serial.println("Settings loaded from EEPROM");
  } else {
    Serial.println("No saved settings, using defaults");
  }
}

inline void saveSettings() {
  EEPROM.write(ADDR_MAGIC, MAGIC_VALUE);
  EEPROM.write(ADDR_MODE, fireMode);
  EEPROM.write(ADDR_SPEED, flickerSpeed);
  EEPROM.write(ADDR_BRIGHTNESS, maxBrightness);
  EEPROM.write(ADDR_NUM_LEDS_L, numLeds & 0xFF);
  EEPROM.write(ADDR_NUM_LEDS_H, (numLeds >> 8) & 0xFF);
  EEPROM.commit();
}

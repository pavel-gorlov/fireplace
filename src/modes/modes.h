#pragma once

#include "../config.h"
#include "../globals.h"
#include "fire.h"
#include "rainbow.h"
#include "firework.h"
#include "storm.h"
#include "rain.h"
#include "tree.h"

// Диспетчер режимов
inline void updateMode() {
  // Off режим
  if (fireMode == MODE_OFF) {
    fill_solid(leds, numLeds, CRGB::Black);
    FastLED.show();
    return;
  }

  // Rainbow
  if (fireMode == MODE_RAINBOW) {
    updateRainbow();
    return;
  }

  // Firework
  if (fireMode == MODE_FIREWORK) {
    updateFirework();
    return;
  }

  // Storm
  if (fireMode == MODE_STORM) {
    updateStorm();
    return;
  }

  // Rain
  if (fireMode == MODE_RAIN) {
    updateRain();
    return;
  }

  // Tree
  if (fireMode == MODE_TREE) {
    updateTree();
    return;
  }

  // Fire modes (Embers, Fire, Flame, Ice)
  updateFireMode();
}

#pragma once

#include "../config.h"
#include "../globals.h"

inline void updateRainbow() {
  rainbowHue += map(flickerSpeed, 100, 5, 1, 5);  // чем меньше interval, тем быстрее
  for (int i = 0; i < numLeds; i++) {
    leds[i] = CHSV(rainbowHue + (i * 256 / numLeds), 255, 255);
  }
  FastLED.setBrightness(maxBrightness);
  FastLED.show();
}

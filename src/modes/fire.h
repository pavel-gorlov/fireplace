#pragma once

#include "../config.h"
#include "../globals.h"

// Целевая яркость для режимов огня
inline byte getTargetBrightness() {
  switch (fireMode) {
    case MODE_EMBERS:   return random8(20, 120);   // тусклые угли
    case MODE_FIRE:     return random8(40, 200);   // средний огонь
    case MODE_FLAME:    return random8(30, 255);   // от углей до яркого
    case MODE_ICE:      return random8(30, 255);   // ледяное пламя
    default:            return random8(40, 200);
  }
}

// Цвет для режимов огня
inline CRGB getFireColor(byte bright) {
  byte hue, sat, val;

  if (fireMode == MODE_EMBERS) {
    // Embers: красный -> тёмно-оранжевый
    hue = map(bright, 0, 255, 0, 20);
    sat = 255;
    val = map(bright, 0, 255, 10, 150);
  } else if (fireMode == MODE_FIRE) {
    // Fire: красный -> оранжевый
    hue = map(bright, 0, 255, 0, 32);
    sat = 255;
    val = map(bright, 0, 255, 30, 220);
  } else if (fireMode == MODE_FLAME) {
    // Flame: красный -> оранжево-жёлтый
    hue = map(bright, 0, 255, 0, 35);
    sat = 255;
    val = map(bright, 0, 255, 80, 255);
  } else {
    // Ice: тёмно-синий -> лазурно-голубой
    hue = map(bright, 0, 255, 160, 140);
    sat = 255;
    val = map(bright, 0, 255, 80, 255);
  }

  return CHSV(hue, sat, val);
}

// Обновление режимов огня (Embers, Fire, Flame, Ice)
inline void updateFireMode() {
  // Скорость изменения: чем меньше flickerSpeed, тем быстрее
  // Множитель для режима: Embers ещё медленнее
  float modeMultiplier = (fireMode == MODE_EMBERS) ? 3.0 : (fireMode == MODE_FIRE) ? 1.5 : 1.0;
  float step = 255.0 / (flickerSpeed * 10 * modeMultiplier);

  for (int i = 0; i < numLeds; i++) {
    if (brightness[i] < targetBrightness[i] - step) {
      brightness[i] += step;
    } else if (brightness[i] > targetBrightness[i] + step) {
      brightness[i] -= step;
    } else {
      brightness[i] = targetBrightness[i];
      targetBrightness[i] = getTargetBrightness();
    }
    leds[i] = getFireColor((byte)brightness[i]);
  }

  FastLED.setBrightness(maxBrightness);
  FastLED.show();
}

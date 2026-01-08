#pragma once

#include "../config.h"
#include "../globals.h"

inline void updateStorm() {
  int fadeSpeed = map(flickerSpeed, 100, 5, 8, 25);

  // Тёмно-синий фон
  for (int i = 0; i < numLeds; i++) {
    leds[i] = CHSV(160, 255, 25);
  }

  // Обновляем активные молнии
  int activeLightnings = 0;
  for (int l = 0; l < MAX_LIGHTNINGS; l++) {
    if (!lightnings[l].active) continue;
    activeLightnings++;

    // Уменьшаем яркость
    lightnings[l].brightness = lightnings[l].brightness > fadeSpeed ? lightnings[l].brightness - fadeSpeed : 0;

    if (lightnings[l].brightness < 20) {
      lightnings[l].active = false;
      continue;
    }

    // Рисуем молнию (сверху вниз = по X)
    for (int x = 0; x < MATRIX_SIZE; x++) {
      int y = lightnings[l].path[x];
      if (y >= 0 && y < MATRIX_SIZE) {
        int idx = y * MATRIX_SIZE + x;
        if (idx >= 0 && idx < numLeds) {
          // Жёлтая молния
          leds[idx] = CHSV(40, 200, lightnings[l].brightness);
        }
      }
    }
  }

  // Создаём новые молнии (редко)
  if (activeLightnings < MAX_LIGHTNINGS && random8() < map(flickerSpeed, 100, 5, 3, 15)) {
    for (int l = 0; l < MAX_LIGHTNINGS; l++) {
      if (!lightnings[l].active) {
        // Генерируем путь молнии сверху вниз
        int y = random8(MATRIX_SIZE);
        for (int x = 0; x < MATRIX_SIZE; x++) {
          lightnings[l].path[x] = y;
          int shift = random8(3) - 1;  // -1, 0, или 1
          y = constrain(y + shift, 0, MATRIX_SIZE - 1);
        }
        lightnings[l].brightness = 255;
        lightnings[l].active = true;
        break;
      }
    }
  }

  FastLED.setBrightness(maxBrightness);
  FastLED.show();
}

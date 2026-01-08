#pragma once

#include "../config.h"
#include "../globals.h"

inline void updateRain() {
  float fallSpeed = map(flickerSpeed, 100, 5, 5, 25) / 100.0;
  float splashSpeed = map(flickerSpeed, 100, 5, 8, 30) / 100.0;
  int fadeSpeed = map(flickerSpeed, 100, 5, 5, 20);

  // Чёрный фон
  fill_solid(leds, numLeds, CRGB::Black);

  // Обновляем капли
  int activeDrops = 0;
  for (int d = 0; d < MAX_RAINDROPS; d++) {
    if (!raindrops[d].active) continue;
    activeDrops++;

    if (!raindrops[d].splashing) {
      // Капля падает
      raindrops[d].y += fallSpeed;

      // Достигла дна - начинаем брызги
      if (raindrops[d].y >= MATRIX_SIZE - 1) {
        raindrops[d].splashing = true;
        raindrops[d].splashRadius = 0;
        raindrops[d].y = MATRIX_SIZE - 1;
      }

      // Рисуем каплю (2 пикселя)
      int pos1 = (int)raindrops[d].y;
      int pos2 = pos1 > 0 ? pos1 - 1 : pos1;
      for (int pos = pos2; pos <= pos1 && pos < MATRIX_SIZE; pos++) {
        if (pos >= 0) {
          int idx = raindrops[d].x * MATRIX_SIZE + pos;
          if (idx >= 0 && idx < numLeds) {
            byte bright = (pos == pos1) ? raindrops[d].brightness : raindrops[d].brightness / 2;
            leds[idx] = CHSV(140, 255, bright);  // голубой
          }
        }
      }
    } else {
      // Брызги расходятся
      raindrops[d].splashRadius += splashSpeed;
      raindrops[d].brightness = raindrops[d].brightness > fadeSpeed ? raindrops[d].brightness - fadeSpeed : 0;

      if (raindrops[d].brightness < 20 || raindrops[d].splashRadius > 4) {
        raindrops[d].active = false;
        continue;
      }

      // Рисуем брызги - полукруг от точки удара
      int row = raindrops[d].x;
      int hitPos = MATRIX_SIZE - 1;
      float r = raindrops[d].splashRadius;

      for (int dr = -2; dr <= 0; dr++) {
        for (int dc = -2; dc <= 2; dc++) {
          int newRow = row + dc;
          int newPos = hitPos + dr;
          if (newRow < 0 || newRow >= MATRIX_SIZE || newPos < 0 || newPos >= MATRIX_SIZE) continue;

          float dist = sqrt(dc * dc + dr * dr);

          if (abs(dist - r) < 1.0) {
            int idx = newRow * MATRIX_SIZE + newPos;
            if (idx >= 0 && idx < numLeds) {
              float intensity = 1.0 - abs(dist - r);
              byte val = raindrops[d].brightness * intensity;
              if (val > 10) {
                leds[idx] += CHSV(140, 255, val);
              }
            }
          }
        }
      }
    }
  }

  // Создаём новые капли
  if (activeDrops < MAX_RAINDROPS && random8() < map(flickerSpeed, 100, 5, 8, 30)) {
    for (int d = 0; d < MAX_RAINDROPS; d++) {
      if (!raindrops[d].active) {
        raindrops[d].x = random8(MATRIX_SIZE);
        raindrops[d].y = 0;
        raindrops[d].brightness = 255;
        raindrops[d].active = true;
        raindrops[d].splashing = false;
        raindrops[d].splashRadius = 0;
        break;
      }
    }
  }

  FastLED.setBrightness(maxBrightness);
  FastLED.show();
}

#pragma once

#include "../config.h"
#include "../globals.h"

inline void updateTree() {
  // Скорость изменения как у углей
  float step = 255.0 / (flickerSpeed * 12);

  // Чёрный фон
  fill_solid(leds, numLeds, CRGB::Black);

  // Ствол: коричневый, в центре (row 3-4), нижняя часть (col 5-7)
  for (int row = 3; row <= 4; row++) {
    for (int col = 5; col <= 7; col++) {
      int idx = row * MATRIX_SIZE + col;
      byte brownHue = 30 + random8(5);
      byte brownSat = 255;
      byte brownVal = 60 + random8(40);
      leds[idx] = CHSV(brownHue, brownSat, brownVal);
    }
  }

  // Крона: форма дерева
  const int crownLeft[] =  {3, 2, 1, 1, 2};
  const int crownRight[] = {4, 5, 6, 6, 5};

  for (int col = 0; col <= 4; col++) {
    for (int row = crownLeft[col]; row <= crownRight[col]; row++) {
      int idx = row * MATRIX_SIZE + col;
      if (idx < 0 || idx >= numLeds) continue;

      // Плавное движение к целевой яркости (как угли)
      if (leafBright[idx] < leafTargetBright[idx] - step) {
        leafBright[idx] += step;
      } else if (leafBright[idx] > leafTargetBright[idx] + step) {
        leafBright[idx] -= step;
      } else {
        // Достигли цели - новая случайная цель
        leafBright[idx] = leafTargetBright[idx];
        leafTargetBright[idx] = random8(40, 255);
        leafHue[idx] = random8(80, 110);  // от жёлто-зелёного до сине-зелёного
      }

      // Рисуем лист
      leds[idx] = CHSV(leafHue[idx], 255, (byte)leafBright[idx]);
    }
  }

  FastLED.setBrightness(maxBrightness);
  FastLED.show();
}

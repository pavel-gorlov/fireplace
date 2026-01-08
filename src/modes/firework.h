#pragma once

#include "../config.h"
#include "../globals.h"

inline void updateFirework() {
  // Скорость расширения зависит от flickerSpeed
  float expandSpeed = map(flickerSpeed, 100, 5, 3, 15) / 100.0;
  int brightFade = map(flickerSpeed, 100, 5, 1, 4);

  // Полностью очищаем — рисуем только кольца
  fill_solid(leds, numLeds, CRGB::Black);

  // Обновляем активные взрывы
  int activeBursts = 0;
  for (int b = 0; b < MAX_BURSTS; b++) {
    if (!bursts[b].active) continue;
    activeBursts++;

    // Расширяем радиус и уменьшаем яркость
    bursts[b].radius += expandSpeed;
    bursts[b].brightness = bursts[b].brightness > brightFade ? bursts[b].brightness - brightFade : 0;

    // Если взрыв затух или вышел за пределы - деактивируем
    if (bursts[b].brightness < 15 || bursts[b].radius > MATRIX_SIZE * 1.5) {
      bursts[b].active = false;
      continue;
    }

    // Рисуем кольцо взрыва
    for (int y = 0; y < MATRIX_SIZE; y++) {
      for (int x = 0; x < MATRIX_SIZE; x++) {
        float dx = x - bursts[b].x;
        float dy = y - bursts[b].y;
        float dist = sqrt(dx * dx + dy * dy);

        // Яркость зависит от расстояния до фронта волны
        float diff = abs(dist - bursts[b].radius);
        if (diff < 1.5) {
          int idx = y * MATRIX_SIZE + x;
          if (idx >= 0 && idx < numLeds) {
            float intensity = 1.0 - (diff / 1.5);
            byte val = bursts[b].brightness * intensity;
            if (val > 10) {
              CRGB color = CHSV(bursts[b].hue, 255, val);
              leds[idx] += color;  // Аддитивное смешивание
            }
          }
        }
      }
    }
  }

  // Создаём новые взрывы (реже)
  if (activeBursts < MAX_BURSTS && random8() < 30) {
    for (int b = 0; b < MAX_BURSTS; b++) {
      if (!bursts[b].active) {
        bursts[b].x = random8(MATRIX_SIZE);
        bursts[b].y = random8(MATRIX_SIZE);
        bursts[b].radius = 0;
        bursts[b].hue = random8();  // случайный цвет
        bursts[b].brightness = 255;
        bursts[b].active = true;
        break;
      }
    }
  }

  FastLED.setBrightness(maxBrightness);
  FastLED.show();
}

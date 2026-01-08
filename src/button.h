#pragma once

#include "config.h"
#include "globals.h"
#include "display.h"
#include "storage.h"

#if HAS_BUTTON

// Переменные для кнопки
static unsigned long btnPressTime = 0;
static unsigned long lastClickTime = 0;
static unsigned long lastAdjustTime = 0;
static bool lastBtnState = HIGH;
static bool adjustingBrightness = false;
static bool adjustingSpeed = false;
static int brightDirection = 1;
static int speedDirection = -1;
static int clickCount = 0;
static bool wasAdjusting = false;

inline void initButton() {
  pinMode(BTN_PIN, INPUT_PULLUP);
}

inline void handleButton() {
  bool btnState = digitalRead(BTN_PIN);
  unsigned long now = millis();

  // Нажатие кнопки (HIGH -> LOW)
  if (btnState == LOW && lastBtnState == HIGH) {
    #if HAS_OLED
      // Если экран выключен - просто включаем его и выходим
      if (!isScreenOn()) {
        wakeScreen();
        lastBtnState = btnState;
        return;
      }
      resetActivityTimer();
    #endif

    btnPressTime = now;
    wasAdjusting = false;

    // Проверяем двойной клик
    if (now - lastClickTime < DOUBLE_CLICK_TIME) {
      clickCount = 2;
    } else {
      clickCount = 1;
    }
    lastClickTime = now;
  }

  // Кнопка удерживается
  if (btnState == LOW) {
    unsigned long holdTime = now - btnPressTime;

    // Начало регулировки после удержания
    if (holdTime > HOLD_THRESHOLD) {
      if (clickCount == 1 && !adjustingBrightness) {
        adjustingBrightness = true;
        wasAdjusting = true;
        lastClickTime = 0;
        Serial.println("Adjusting brightness...");
      } else if (clickCount == 2 && !adjustingSpeed) {
        adjustingSpeed = true;
        wasAdjusting = true;
        lastClickTime = 0;
        Serial.println("Adjusting speed...");
      }

      // Плавная регулировка
      if (now - lastAdjustTime > ADJUST_INTERVAL) {
        lastAdjustTime = now;

        if (adjustingBrightness) {
          int newBright = maxBrightness + brightDirection * 3;
          if (newBright >= 255) {
            maxBrightness = 255;
          } else if (newBright <= 10) {
            maxBrightness = 10;
          } else {
            maxBrightness = newBright;
          }
          FastLED.setBrightness(maxBrightness);
          FastLED.show();
          updateDisplay();
        }

        if (adjustingSpeed) {
          int newSpeed = flickerSpeed + speedDirection * 1;
          if (newSpeed >= 100) {
            flickerSpeed = 100;
          } else if (newSpeed <= 5) {
            flickerSpeed = 5;
          } else {
            flickerSpeed = newSpeed;
          }
          updateDisplay();
        }
      }
    }
  }

  // Отпускание кнопки (LOW -> HIGH)
  if (btnState == HIGH && lastBtnState == LOW) {
    if (adjustingBrightness) {
      // Меняем направление для следующего раза
      if (maxBrightness >= 255) brightDirection = -1;
      else if (maxBrightness <= 10) brightDirection = 1;
      else brightDirection = -brightDirection;

      saveSettings();
      adjustingBrightness = false;
      Serial.print("Brightness saved: ");
      Serial.println(maxBrightness);
    }

    if (adjustingSpeed) {
      // Меняем направление для следующего раза
      if (flickerSpeed >= 100) speedDirection = -1;
      else if (flickerSpeed <= 5) speedDirection = 1;
      else speedDirection = -speedDirection;

      saveSettings();
      adjustingSpeed = false;
      Serial.print("Speed saved: ");
      Serial.println(flickerSpeed);
    }

    clickCount = 0;
  }

  // Обработка одиночного клика (смена режима)
  if (btnState == HIGH && !wasAdjusting) {
    if (lastClickTime > 0 && now - lastClickTime > DOUBLE_CLICK_TIME) {
      fireMode++;
      if (fireMode > MODE_TREE) fireMode = MODE_EMBERS;

      // Применяем пресет для нового режима
      flickerSpeed = presetSpeed[fireMode];
      maxBrightness = presetBrightness[fireMode];
      FastLED.setBrightness(maxBrightness);

      saveSettings();
      updateDisplay();
      Serial.print("Mode: ");
      Serial.println(modeNames[fireMode]);

      lastClickTime = 0;
    }
  }

  lastBtnState = btnState;
}

#else
  // Заглушки для сборки без кнопки
  inline void initButton() {}
  inline void handleButton() {}
#endif

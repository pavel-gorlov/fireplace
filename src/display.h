#pragma once

#include "config.h"
#include "globals.h"
#include <ESP8266WiFi.h>

#if HAS_OLED
  #include <Wire.h>
  #include <SSD1306Wire.h>

  // OLED дисплей 128x64 на I2C
  static SSD1306Wire display(0x3C, OLED_SDA, OLED_SCL);

  // Состояние экрана
  static unsigned long lastActivityTime = 0;
  static bool screenOn = true;
  static bool showingWifiBeforeSleep = false;
  static unsigned long wifiInfoStartTime = 0;

  // Иконка песочных часов 12x12
  static const uint8_t icon_hourglass[] PROGMEM = {
    0xFE, 0x07, 0xFE, 0x07, 0x04, 0x02, 0x88, 0x01,
    0xD0, 0x00, 0x60, 0x00, 0x60, 0x00, 0xD0, 0x00,
    0x88, 0x01, 0x04, 0x02, 0xFE, 0x07, 0xFE, 0x07
  };

  // Иконка солнца 16x16
  static const uint8_t icon_sun[] PROGMEM = {
    0x80, 0x00,
    0x84, 0x10,
    0x88, 0x08,
    0x10, 0x04,
    0x00, 0x00,
    0xC0, 0x03,
    0xC0, 0x03,
    0xC7, 0xE3,
    0xC0, 0x03,
    0xC0, 0x03,
    0x00, 0x00,
    0x10, 0x04,
    0x88, 0x08,
    0x84, 0x10,
    0x80, 0x00,
    0x00, 0x00
  };

  inline void initDisplay() {
    display.init();
    display.flipScreenVertically();
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.clear();
    display.drawString(64, 24, "Connecting...");
    display.display();
  }

  inline void updateDisplay() {
    display.clear();

    // Строка 1: IP адрес
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 0, WiFi.localIP().toString());

    // Строка 2: Режим
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 20, modeNames[fireMode]);

    // Строка 3: Скорость и Яркость с иконками
    display.setFont(ArialMT_Plain_16);

    // Скорость: иконка песочных часов + значение
    display.drawXbm(0, 50, 12, 12, icon_hourglass);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(14, 46, String(flickerSpeed));

    // Яркость: значение + иконка солнца
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.drawString(110, 46, String(maxBrightness));
    display.drawXbm(112, 48, 16, 16, icon_sun);

    display.display();
  }

  inline void showWiFiSetupScreen() {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 0, "WiFi not configured");
    display.drawString(64, 14, "Connect to AP:");
    display.setFont(ArialMT_Plain_16);
    display.drawString(64, 28, "Fireplace-Setup");
    display.setFont(ArialMT_Plain_10);
    display.drawString(64, 48, "Pass: 12345678");
    display.display();
  }

  inline void resetActivityTimer() {
    lastActivityTime = millis();
  }

  inline bool isScreenOn() {
    return screenOn;
  }

  inline void wakeScreen() {
    display.displayOn();
    screenOn = true;
    showingWifiBeforeSleep = false;
    lastActivityTime = millis();
    updateDisplay();
    Serial.println("Screen wake up");
  }

  inline void handleScreenSleep() {
    // Засыпание экрана по таймауту
    if (screenOn && !showingWifiBeforeSleep && millis() - lastActivityTime > SCREEN_TIMEOUT) {
      if (!wifiConnected) {
        showingWifiBeforeSleep = true;
        wifiInfoStartTime = millis();
        showWiFiSetupScreen();
        Serial.println("Showing WiFi info before sleep");
      } else {
        display.displayOff();
        screenOn = false;
        Serial.println("Screen off");
      }
    }

    // После 10 сек WiFi инфо — выключить экран
    if (showingWifiBeforeSleep && millis() - wifiInfoStartTime > WIFI_INFO_BEFORE_SLEEP) {
      display.displayOff();
      screenOn = false;
      showingWifiBeforeSleep = false;
      Serial.println("Screen off after WiFi info");
    }
  }

  inline bool isShowingWifiBeforeSleep() {
    return showingWifiBeforeSleep;
  }

#else
  // Заглушки для сборки без OLED
  inline void initDisplay() {}
  inline void updateDisplay() {}
  inline void showWiFiSetupScreen() {}
  inline void resetActivityTimer() {}
  inline bool isScreenOn() { return false; }
  inline void wakeScreen() {}
  inline void handleScreenSleep() {}
  inline bool isShowingWifiBeforeSleep() { return false; }
#endif

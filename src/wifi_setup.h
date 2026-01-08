#pragma once

#include "config.h"
#include "globals.h"
#include "display.h"
#include <WiFiManager.h>

static WiFiManager wm;

inline void setupWiFi() {
  // wm.resetSettings();  // Раскомментируй для сброса WiFi

  Serial.println();
  Serial.println("Connecting to WiFi...");

  #if HAS_BUTTON
    // С кнопкой: неблокирующий режим — камин работает сразу
    wm.setConfigPortalBlocking(false);
    wm.setConfigPortalTimeout(0);  // Портал не закрывается по таймауту

    if (wm.autoConnect("Fireplace-Setup", "12345678")) {
      wifiConnected = true;
      Serial.println("WiFi connected!");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
    } else {
      wifiPortalActive = true;
      wifiSetupStartTime = millis();
      showWiFiSetupScreen();
      Serial.println("WiFi portal started, running in offline mode");
    }
  #else
    // Без кнопки: блокирующий режим — ждём WiFi
    wm.setConfigPortalTimeout(180);
    wm.setAPCallback([](WiFiManager *myWiFiManager) {
      Serial.println("Entered config mode");
      fill_solid(leds, numLeds, CRGB(255, 100, 20));
      FastLED.setBrightness(100);
      FastLED.show();
    });

    if (!wm.autoConnect("Fireplace-Setup", "12345678")) {
      Serial.println("Failed to connect, restarting...");
      delay(3000);
      ESP.restart();
    }
    wifiConnected = true;
    Serial.println("WiFi connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  #endif
}

inline void handleWiFiPortal() {
  #if HAS_BUTTON
    if (wifiPortalActive) {
      wm.process();

      // Проверяем подключение к WiFi
      if (WiFi.status() == WL_CONNECTED && !wifiConnected) {
        wifiConnected = true;
        wifiPortalActive = false;
        server.begin();
        Serial.println("WiFi connected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        updateDisplay();
      }

      // Через 10 сек переключаемся на обычный дисплей (если не показываем WiFi перед сном)
      #if HAS_OLED
        if (!isShowingWifiBeforeSleep() && millis() - wifiSetupStartTime > WIFI_INFO_DURATION) {
          wifiSetupStartTime = millis() + 1000000;  // Не показывать больше
          updateDisplay();
        }
      #else
        if (millis() - wifiSetupStartTime > WIFI_INFO_DURATION) {
          wifiSetupStartTime = millis() + 1000000;
        }
      #endif
    }
  #endif
}

// Fireplace v0.4.0
// Модульная структура

#define GLOBALS_IMPL  // Определить глобальные переменные в этом файле

#include "config.h"
#include "globals.h"
#include "storage.h"
#include "display.h"
#include "button.h"
#include "wifi_setup.h"
#include "web/server.h"
#include "modes/modes.h"

void setup() {
  Serial.begin(115200);

  // Инициализация кнопки
  initButton();

  // Инициализация дисплея
  initDisplay();

  // Загрузка настроек из EEPROM
  loadSettings();

  // Отладка: показываем конфигурацию
  Serial.print("LED config: ");
  #if LED_MATRIX
    Serial.println("MATRIX 8x8, WS2812, GRB");
  #else
    Serial.println("STRIP 144, WS2811, GRB");
  #endif
  Serial.print("numLeds: ");
  Serial.println(numLeds);

  // Инициализация LED
  FastLED.addLeds<LED_TYPE, LED_PIN, LED_COLOR_ORDER>(leds, numLeds).setCorrection(TypicalLEDStrip);

  // Тест LED при старте - красная вспышка
  fill_solid(leds, numLeds, CRGB::Red);
  FastLED.show();
  delay(300);
  fill_solid(leds, numLeds, CRGB::Black);
  FastLED.show();
  FastLED.setBrightness(maxBrightness);

  // Инициализация массивов яркости
  for (int i = 0; i < numLeds; i++) {
    brightness[i] = (float)random8();
    targetBrightness[i] = random8();
  }

  // Настройка WiFi
  setupWiFi();

  // Настройка веб-сервера
  setupWebServer();

  if (wifiConnected) {
    startWebServer();
    updateDisplay();
  }

  resetActivityTimer();
}

void loop() {
  // Обработка WiFi портала
  handleWiFiPortal();

  // Обработка веб-запросов
  if (wifiConnected) {
    server.handleClient();
  }

  // Обработка кнопки
  handleButton();

  // Засыпание экрана
  handleScreenSleep();

  // Обновление LED эффектов
  if (millis() - lastUpdate > UPDATE_INTERVAL) {
    lastUpdate = millis();
    updateMode();
  }
}

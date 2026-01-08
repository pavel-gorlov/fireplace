#pragma once

#include <Arduino.h>

// === КОНФИГУРАЦИЯ ===
// Эти значения можно переопределить через build_flags в platformio.ini
#ifndef LED_MATRIX
  #define LED_MATRIX 1     // 1 = матрица 8x8, 0 = лента
#endif
#ifndef HAS_OLED
  #define HAS_OLED 1       // 1 = есть OLED дисплей
#endif
#ifndef HAS_BUTTON
  #define HAS_BUTTON 1     // 1 = есть кнопка
#endif

// === ПИНЫ ===
#define LED_PIN 0          // GPIO0 = D3

#if HAS_BUTTON
  #define BTN_PIN 13       // GPIO13 = D7
#endif

#if HAS_OLED
  #define OLED_SDA 14      // GPIO14 = D5
  #define OLED_SCL 12      // GPIO12 = D6
#endif

// === LED КОНФИГУРАЦИЯ ===
#if LED_MATRIX
  #define DEFAULT_NUM_LEDS 64
  #define LED_TYPE WS2812
  #define LED_COLOR_ORDER GRB
  #define MATRIX_SIZE 8
#else
  #define DEFAULT_NUM_LEDS 144
  #define LED_TYPE WS2811
  #define LED_COLOR_ORDER GRB
  #define MATRIX_SIZE 8    // используется в режимах
#endif

#define MAX_LEDS 300

// === EEPROM ===
#define EEPROM_SIZE 16
#define ADDR_MAGIC 0      // 1 byte - маркер валидности данных
#define ADDR_MODE 1       // 1 byte
#define ADDR_SPEED 2      // 1 byte
#define ADDR_BRIGHTNESS 3 // 1 byte
#define ADDR_NUM_LEDS_L 4 // 1 byte - младший байт количества LED
#define ADDR_NUM_LEDS_H 5 // 1 byte - старший байт количества LED
#define MAGIC_VALUE 0x42  // маркер что данные записаны

// === ТАЙМАУТЫ ===
#if HAS_OLED
  #define SCREEN_TIMEOUT 30000          // мс до засыпания экрана (30 сек)
  #define WIFI_INFO_BEFORE_SLEEP 10000  // показывать WiFi инфо 10 сек перед сном
#endif

#define WIFI_INFO_DURATION 10000        // показывать инфо о WiFi 10 сек
#define UPDATE_INTERVAL 10              // интервал обновления LED (мс)

#if HAS_BUTTON
  #define BTN_DEBOUNCE 50               // мс
  #define HOLD_THRESHOLD 400            // мс до начала регулировки
  #define ADJUST_INTERVAL 50            // мс между шагами
  #define DOUBLE_CLICK_TIME 300         // мс между кликами
#endif

// === РЕЖИМЫ ===
enum Mode {
  MODE_OFF = 0,
  MODE_EMBERS,
  MODE_FIRE,
  MODE_FLAME,
  MODE_ICE,
  MODE_RAINBOW,
  MODE_FIREWORK,
  MODE_STORM,
  MODE_RAIN,
  MODE_TREE,
  MODE_COUNT
};

// Названия режимов
const char* const modeNames[] = {"Off", "Embers", "Fire", "Flame", "Ice", "Rainbow", "Firework", "Storm", "Rain", "Tree"};

// Пресеты: скорость и яркость для каждого режима
const byte presetSpeed[] =      {0, 70, 30,  5, 10, 20, 90, 50, 80, 15};
const byte presetBrightness[] = {0, 70, 150, 255, 200, 100, 255, 200, 70, 55};

// === ЭФФЕКТЫ: КОНСТАНТЫ ===
#define MAX_BURSTS 4       // Фейерверк: макс взрывов
#define MAX_LIGHTNINGS 2   // Гроза: макс молний
#define MAX_RAINDROPS 4    // Дождь: макс капель

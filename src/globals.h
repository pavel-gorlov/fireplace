#pragma once

#include "config.h"
#include "FastLED.h"
#include <ESP8266WebServer.h>

// === СТРУКТУРЫ ЭФФЕКТОВ ===

// Фейерверк: структура взрыва
struct Burst {
  float x, y;       // центр взрыва
  float radius;     // текущий радиус
  byte hue;         // цвет
  byte brightness;  // яркость (затухает)
  bool active;
};

// Гроза: структура молнии
struct Lightning {
  int path[MATRIX_SIZE];  // x-позиция на каждом уровне y
  byte brightness;        // яркость (затухает)
  bool active;
};

// Дождь: структура капли
struct Raindrop {
  int x;              // колонка
  float y;            // позиция по вертикали (float для плавности)
  byte brightness;    // яркость
  bool active;
  bool splashing;     // режим брызг после падения
  float splashRadius; // радиус брызг
};

// === ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ===

// LED
extern CRGB leds[MAX_LEDS];
extern int numLeds;
extern float brightness[MAX_LEDS];
extern byte targetBrightness[MAX_LEDS];

// Настройки
extern int fireMode;
extern int flickerSpeed;
extern int maxBrightness;

// Веб-сервер
extern ESP8266WebServer server;

// Время
extern unsigned long lastUpdate;

// Эффекты
extern Burst bursts[MAX_BURSTS];
extern Lightning lightnings[MAX_LIGHTNINGS];
extern Raindrop raindrops[MAX_RAINDROPS];
extern byte rainbowHue;

// Дерево: состояние листьев
extern float leafBright[64];
extern byte leafTargetBright[64];
extern byte leafHue[64];

// WiFi состояние
extern bool wifiConnected;
extern bool wifiPortalActive;
extern unsigned long wifiSetupStartTime;

// === ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ (определения в globals.cpp или main.cpp) ===
// Если используется header-only, раскомментировать:
#ifdef GLOBALS_IMPL

CRGB leds[MAX_LEDS];
int numLeds = DEFAULT_NUM_LEDS;
float brightness[MAX_LEDS];
byte targetBrightness[MAX_LEDS];

int fireMode = MODE_FIRE;
int flickerSpeed = 30;
int maxBrightness = 255;

ESP8266WebServer server(80);

unsigned long lastUpdate = 0;

Burst bursts[MAX_BURSTS];
Lightning lightnings[MAX_LIGHTNINGS];
Raindrop raindrops[MAX_RAINDROPS];
byte rainbowHue = 0;

float leafBright[64];
byte leafTargetBright[64];
byte leafHue[64];

bool wifiConnected = false;
bool wifiPortalActive = false;
unsigned long wifiSetupStartTime = 0;

#endif

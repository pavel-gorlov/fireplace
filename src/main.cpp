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

#define LED_PIN 0          // GPIO0 = D3

#if HAS_BUTTON
  #define BTN_PIN 13       // GPIO13 = D7
#endif

#if LED_MATRIX
  #define DEFAULT_NUM_LEDS 64
  #define LED_TYPE WS2812
  #define LED_COLOR_ORDER GRB
#else
  #define DEFAULT_NUM_LEDS 144
  #define LED_TYPE WS2811
  #define LED_COLOR_ORDER GRB    // как в старой рабочей версии
#endif

#define MAX_LEDS 300
#include "FastLED.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <EEPROM.h>
#if HAS_OLED
  #include <Wire.h>
  #include <SSD1306Wire.h>
  // OLED дисплей 128x64 на I2C (HW-364A: SDA=GPIO14/D5, SCL=GPIO12/D6)
  #define OLED_SDA 14
  #define OLED_SCL 12
  SSD1306Wire display(0x3C, OLED_SDA, OLED_SCL);
#endif

// EEPROM addresses
#define EEPROM_SIZE 16
#define ADDR_MAGIC 0      // 1 byte - маркер валидности данных
#define ADDR_MODE 1       // 1 byte
#define ADDR_SPEED 2      // 1 byte
#define ADDR_BRIGHTNESS 3 // 1 byte
#define ADDR_NUM_LEDS_L 4 // 1 byte - младший байт количества LED
#define ADDR_NUM_LEDS_H 5 // 1 byte - старший байт количества LED
#define MAGIC_VALUE 0x42  // маркер что данные записаны

void handleRoot();
void handleSet();
void handleSetLeds();
void handleWiFi();
void handleWiFiConnect();
void handleWiFiScan();
void updateFire();
void updateFirework();
void updateStorm();
void updateRain();
void updateTree();
byte getTargetBrightness();
CRGB getFireColor(byte bright);
void loadSettings();
void saveSettings();
void updateDisplay();
void handleButton();

#if HAS_BUTTON
  // Переменные для кнопки
  unsigned long btnPressTime = 0;       // время нажатия
  unsigned long lastClickTime = 0;      // время последнего клика
  unsigned long lastAdjustTime = 0;     // время последней корректировки
  bool lastBtnState = HIGH;
  bool adjustingBrightness = false;     // режим регулировки яркости
  bool adjustingSpeed = false;          // режим регулировки скорости
  int brightDirection = 1;              // направление яркости: 1 = вверх, -1 = вниз
  int speedDirection = -1;              // направление скорости: -1 = быстрее, 1 = медленнее
  int clickCount = 0;                   // счётчик кликов
  bool wasAdjusting = false;            // флаг что была регулировка (не менять режим)
  #define BTN_DEBOUNCE 50               // мс
  #define HOLD_THRESHOLD 400            // мс до начала регулировки
  #define ADJUST_INTERVAL 50            // мс между шагами (медленнее)
  #define DOUBLE_CLICK_TIME 300         // мс между кликами
#endif

#if HAS_OLED
  // Засыпание экрана
  unsigned long lastActivityTime = 0;   // время последней активности
  bool screenOn = true;                 // экран включён
  bool showingWifiBeforeSleep = false;  // показываем WiFi инфо перед сном
  unsigned long wifiInfoStartTime = 0;  // начало показа WiFi инфо
  #define SCREEN_TIMEOUT 30000          // мс до засыпания (30 сек)
  #define WIFI_INFO_BEFORE_SLEEP 10000  // показывать WiFi инфо 10 сек перед сном
#endif

// Названия режимов (0 = выключено)
const char* modeNames[] = {"Off", "Embers", "Fire", "Flame", "Ice", "Rainbow", "Firework", "Storm", "Rain", "Tree"};

// Пресеты: скорость и яркость для каждого режима (индекс 0 не используется)
const byte presetSpeed[] =      {0, 70, 30,  5, 10, 20, 90, 50, 80, 15};
const byte presetBrightness[] = {0, 70, 150, 255, 200, 100, 255, 200, 70, 55};

// Общие константы для матрицы
#define MATRIX_SIZE 8

// Фейерверк: структура взрыва
#define MAX_BURSTS 4
struct Burst {
  float x, y;       // центр взрыва
  float radius;     // текущий радиус
  byte hue;         // цвет
  byte brightness;  // яркость (затухает)
  bool active;
};
Burst bursts[MAX_BURSTS];

// Гроза: структура молнии
#define MAX_LIGHTNINGS 2
struct Lightning {
  int path[MATRIX_SIZE];  // x-позиция на каждом уровне y
  byte brightness;        // яркость (затухает)
  bool active;
};
Lightning lightnings[MAX_LIGHTNINGS];

// Дождь: структура капли
#define MAX_RAINDROPS 4
struct Raindrop {
  int x;              // колонка
  float y;            // позиция по вертикали (float для плавности)
  byte brightness;    // яркость
  bool active;
  bool splashing;     // режим брызг после падения
  float splashRadius; // радиус брызг
};
Raindrop raindrops[MAX_RAINDROPS];

// Дерево: состояние каждого листа (как угли)
float leafBright[64];         // текущая яркость
byte leafTargetBright[64];    // целевая яркость
byte leafHue[64];             // текущий оттенок зелёного

#if HAS_OLED
  // Иконка песочных часов 12x12
  const uint8_t icon_hourglass[] PROGMEM = {
    0xFE, 0x07, 0xFE, 0x07, 0x04, 0x02, 0x88, 0x01,
    0xD0, 0x00, 0x60, 0x00, 0x60, 0x00, 0xD0, 0x00,
    0x88, 0x01, 0x04, 0x02, 0xFE, 0x07, 0xFE, 0x07
  };

  // Иконка солнца 16x16: маленький круг 4x4, лучи 1px по сторонам и диагоналям
  const uint8_t icon_sun[] PROGMEM = {
    0x80, 0x00,  // верхний луч
    0x84, 0x10,  // диагонали + луч
    0x88, 0x08,  // диагонали + луч
    0x10, 0x04,  // диагонали
    0x00, 0x00,
    0xC0, 0x03,  // круг 4x4
    0xC0, 0x03,
    0xC7, 0xE3,  // круг + боковые лучи
    0xC0, 0x03,
    0xC0, 0x03,  // круг 4x4
    0x00, 0x00,
    0x10, 0x04,  // диагонали
    0x88, 0x08,  // диагонали + луч
    0x84, 0x10,  // диагонали + луч
    0x80, 0x00,  // нижний луч
    0x00, 0x00
  };
#endif

CRGB leds[MAX_LEDS];
ESP8266WebServer server(80);

int numLeds = DEFAULT_NUM_LEDS;
int fireMode = 2;
int flickerSpeed = 30;
int maxBrightness = 255;

float brightness[MAX_LEDS];
byte targetBrightness[MAX_LEDS];
byte rainbowHue = 0;  // смещение для радуги

unsigned long lastUpdate = 0;
#define UPDATE_INTERVAL 10  // фиксированный интервал обновления (мс)

// WiFi состояние
WiFiManager wm;
bool wifiConnected = false;
bool wifiPortalActive = false;
unsigned long wifiSetupStartTime = 0;
#define WIFI_INFO_DURATION 10000  // показывать инфо о WiFi 10 сек

void showWiFiSetupScreen() {
  #if HAS_OLED
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
  #endif
}

void setup() {
  Serial.begin(115200);

  #if HAS_BUTTON
    pinMode(BTN_PIN, INPUT_PULLUP);
  #endif

  #if HAS_OLED
    display.init();
    display.flipScreenVertically();
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.clear();
    display.drawString(64, 24, "Connecting...");
    display.display();
  #endif

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

  FastLED.addLeds<LED_TYPE, LED_PIN, LED_COLOR_ORDER>(leds, numLeds).setCorrection(TypicalLEDStrip);

  // Тест LED при старте - красная вспышка
  fill_solid(leds, numLeds, CRGB::Red);
  FastLED.show();
  delay(300);
  fill_solid(leds, numLeds, CRGB::Black);
  FastLED.show();
  FastLED.setBrightness(maxBrightness);

  for (int i = 0; i < numLeds; i++) {
    brightness[i] = (float)random8();
    targetBrightness[i] = random8();
  }
  
  // WiFiManager настройка
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

  // Настройка веб-сервера
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/setleds", handleSetLeds);
  server.on("/wifi", handleWiFi);
  server.on("/wifi/scan", handleWiFiScan);
  server.on("/wifi/connect", handleWiFiConnect);

  if (wifiConnected) {
    server.begin();
    updateDisplay();
  }

  #if HAS_OLED
    lastActivityTime = millis();
  #endif
}

void loop() {
  // Обработка WiFi портала (неблокирующий режим)
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
        if (!showingWifiBeforeSleep && millis() - wifiSetupStartTime > WIFI_INFO_DURATION) {
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

  if (wifiConnected) {
    server.handleClient();
  }

  #if HAS_BUTTON
    handleButton();
  #endif

  #if HAS_OLED
    // Засыпание экрана по таймауту
    if (screenOn && !showingWifiBeforeSleep && millis() - lastActivityTime > SCREEN_TIMEOUT) {
      // Если WiFi не подключён — сначала показать инфо о настройке
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
  #endif

  if (millis() - lastUpdate > UPDATE_INTERVAL) {
    lastUpdate = millis();
    updateFire();
  }
}

void updateFire() {
  // Off режим — выключить все LED
  if (fireMode == 0) {
    fill_solid(leds, numLeds, CRGB::Black);
    FastLED.show();
    return;
  }

  // Rainbow режим — отдельная логика
  if (fireMode == 5) {
    rainbowHue += map(flickerSpeed, 100, 5, 1, 5);  // чем меньше interval, тем быстрее (в 2 раза медленнее)
    for (int i = 0; i < numLeds; i++) {
      leds[i] = CHSV(rainbowHue + (i * 256 / numLeds), 255, 255);
    }
    FastLED.setBrightness(maxBrightness);
    FastLED.show();
    return;
  }

  // Firework режим
  if (fireMode == 6) {
    updateFirework();
    return;
  }

  // Storm режим
  if (fireMode == 7) {
    updateStorm();
    return;
  }

  // Rain режим
  if (fireMode == 8) {
    updateRain();
    return;
  }

  // Tree режим
  if (fireMode == 9) {
    updateTree();
    return;
  }

  // Скорость изменения: чем меньше flickerSpeed, тем быстрее
  // Множитель для режима: Embers ещё медленнее
  float modeMultiplier = (fireMode == 1) ? 3.0 : (fireMode == 2) ? 1.5 : 1.0;
  float step = 255.0 / (flickerSpeed * 10 * modeMultiplier);

  for (int i = 0; i < numLeds; i++) {
    if (brightness[i] < targetBrightness[i] - step) {
      brightness[i] += step;
    } else if (brightness[i] > targetBrightness[i] + step) {
      brightness[i] -= step;
    } else {
      brightness[i] = targetBrightness[i];
      targetBrightness[i] = getTargetBrightness();
    }
    leds[i] = getFireColor((byte)brightness[i]);
  }

  FastLED.setBrightness(maxBrightness);
  FastLED.show();
}

void updateFirework() {
  // Скорость расширения зависит от flickerSpeed (медленнее)
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

    // Если взрыв затух или вышел за пределы - деактивируем (больше радиус)
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
            // Плавное затухание от центра кольца
            float intensity = 1.0 - (diff / 1.5);
            byte val = bursts[b].brightness * intensity;
            if (val > 10) {
              CRGB color = CHSV(bursts[b].hue, 255, val);
              // Аддитивное смешивание для перекрытия колец
              leds[idx] += color;
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

void updateStorm() {
  int fadeSpeed = map(flickerSpeed, 100, 5, 8, 25);

  // Тёмно-синий фон
  for (int i = 0; i < numLeds; i++) {
    leds[i] = CHSV(160, 255, 25);  // тёмно-синий
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
        // Генерируем путь молнии сверху вниз (по X, отклонение по Y)
        int y = random8(MATRIX_SIZE);
        for (int x = 0; x < MATRIX_SIZE; x++) {
          lightnings[l].path[x] = y;
          // Случайное отклонение вверх/вниз
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

void updateRain() {
  float fallSpeed = map(flickerSpeed, 100, 5, 5, 25) / 100.0;
  float splashSpeed = map(flickerSpeed, 100, 5, 8, 30) / 100.0;
  int fadeSpeed = map(flickerSpeed, 100, 5, 5, 20);

  // Чёрный фон
  fill_solid(leds, numLeds, CRGB::Black);

  // Обновляем капли
  // x = колонка (строка матрицы), y = позиция падения (по X в idx)
  int activeDrops = 0;
  for (int d = 0; d < MAX_RAINDROPS; d++) {
    if (!raindrops[d].active) continue;
    activeDrops++;

    if (!raindrops[d].splashing) {
      // Капля падает (по визуальной вертикали = x в idx)
      raindrops[d].y += fallSpeed;

      // Достигла дна - начинаем брызги
      if (raindrops[d].y >= MATRIX_SIZE - 1) {
        raindrops[d].splashing = true;
        raindrops[d].splashRadius = 0;
        raindrops[d].y = MATRIX_SIZE - 1;
      }

      // Рисуем каплю (2 пикселя вдоль падения)
      int pos1 = (int)raindrops[d].y;
      int pos2 = pos1 > 0 ? pos1 - 1 : pos1;
      for (int pos = pos2; pos <= pos1 && pos < MATRIX_SIZE; pos++) {
        if (pos >= 0) {
          // idx = row * MATRIX_SIZE + col, где row = x капли, col = pos
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
      int row = raindrops[d].x;  // строка матрицы
      int hitPos = MATRIX_SIZE - 1;  // позиция удара (низ)
      float r = raindrops[d].splashRadius;

      for (int dr = -2; dr <= 0; dr++) {  // брызги вверх (меньше pos)
        for (int dc = -2; dc <= 2; dc++) {  // в стороны по строкам
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
        raindrops[d].x = random8(MATRIX_SIZE);  // случайная строка
        raindrops[d].y = 0;  // начало падения (верх)
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

void updateTree() {
  // Скорость изменения как у углей - зависит от flickerSpeed
  float step = 255.0 / (flickerSpeed * 12);  // медленнее чем огонь

  // Чёрный фон
  fill_solid(leds, numLeds, CRGB::Black);

  // Ствол: коричневый, в центре (row 3-4), нижняя часть (col 5-7)
  for (int row = 3; row <= 4; row++) {
    for (int col = 5; col <= 7; col++) {
      int idx = row * MATRIX_SIZE + col;
      // Коричневый цвет (hue 30-35 = коричневый)
      byte brownHue = 30 + random8(5);
      byte brownSat = 255;
      byte brownVal = 60 + random8(40);  // тусклый
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
        leafTargetBright[idx] = random8(40, 255);  // широкий разброс яркости
        // Новый случайный оттенок зелёного
        leafHue[idx] = random8(80, 110);  // от жёлто-зелёного до сине-зелёного
      }

      // Рисуем лист
      byte hue = leafHue[idx];
      byte sat = 255;
      byte val = (byte)leafBright[idx];

      leds[idx] = CHSV(hue, sat, val);
    }
  }

  FastLED.setBrightness(maxBrightness);
  FastLED.show();
}

byte getTargetBrightness() {
  switch (fireMode) {
    case 1: return random8(20, 120);   // Embers: тусклые угли
    case 2: return random8(40, 200);   // Fire: средний огонь
    case 3: return random8(30, 255);   // Flame: от углей до яркого пламени
    case 4: return random8(30, 255);   // Ice: ледяное пламя
    case 5: return 255;                // Rainbow: полная яркость
    case 6: return 255;                // Firework: полная яркость
    case 7: return 255;                // Storm: полная яркость
    case 8: return 255;                // Rain: полная яркость
    case 9: return 255;                // Tree: полная яркость
    default: return random8(40, 200);
  }
}

CRGB getFireColor(byte bright) {
  byte hue, sat, val;

  if (fireMode == 1) {
    // Embers: красный -> тёмно-оранжевый
    hue = map(bright, 0, 255, 0, 20);
    sat = 255;
    val = map(bright, 0, 255, 10, 150);
  } else if (fireMode == 2) {
    // Fire: красный -> оранжевый
    hue = map(bright, 0, 255, 0, 32);
    sat = 255;
    val = map(bright, 0, 255, 30, 220);
  } else if (fireMode == 3) {
    // Flame: красный -> оранжево-жёлтый
    hue = map(bright, 0, 255, 0, 35);
    sat = 255;
    val = map(bright, 0, 255, 80, 255);
  } else {
    // Ice: тёмно-синий -> лазурно-голубой
    hue = map(bright, 0, 255, 160, 140);
    sat = 255;
    val = map(bright, 0, 255, 80, 255);
  }

  return CHSV(hue, sat, val);
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>Fireplace</title>";
  html += "<style>";
  html += "body{font-family:Arial;background:#1a1a1a;color:#fff;padding:20px}";
  html += ".box{background:#333;padding:15px;border-radius:10px;margin:10px 0}";
  html += "input[type=range]{width:100%}";
  html += ".btns{display:flex;gap:10px}";
  html += ".btn{flex:1;padding:12px;border:none;border-radius:8px;background:#444;color:#fff}";
  html += ".btn.on{background:#ff6600}";
    html += "</style></head><body>";
  
  html += "<h2 style='color:#ff6600;text-align:center;cursor:pointer' onclick='document.getElementById(\"adv\").style.display=document.getElementById(\"adv\").style.display==\"none\"?\"block\":\"none\"'>Fireplace</h2>";

  // Кнопка питания
  html += "<div class='box' style='text-align:center'>";
  html += "<button id='pwr' class='btn' onclick='togglePower()' style='font-size:24px;padding:15px 40px;background:" + String(fireMode==0?"#333":"#ff6600") + "'>" + String(fireMode==0?"&#9788;":"&#9728;") + "</button>";
  html += "</div>";

  html += "<div class='box'><div class='btns'>";
  html += "<button class='btn" + String(fireMode==1?" on":"") + "' onclick='setMode(1)' style='background:" + String(fireMode==1?"#aa2200":"#522") + "'>Embers</button>";
  html += "<button class='btn" + String(fireMode==2?" on":"") + "' onclick='setMode(2)' style='background:" + String(fireMode==2?"#ff6600":"#633") + "'>Fire</button>";
  html += "<button class='btn" + String(fireMode==3?" on":"") + "' onclick='setMode(3)' style='background:" + String(fireMode==3?"#ff9922":"#653") + "'>Flame</button>";
  html += "<button class='btn" + String(fireMode==4?" on":"") + "' onclick='setMode(4)' style='background:" + String(fireMode==4?"#0099ff":"#446") + "'>Ice</button>";
  html += "<button class='btn" + String(fireMode==5?" on":"") + "' onclick='setMode(5)' style='background:" + String(fireMode==5?"linear-gradient(90deg,red,orange,yellow,green,blue,violet)":"linear-gradient(90deg,#633,#663,#363,#336,#636)") + "'>Rainbow</button>";
  html += "</div><div class='btns' style='margin-top:10px'>";
  html += "<button class='btn" + String(fireMode==6?" on":"") + "' onclick='setMode(6)' style='background:" + String(fireMode==6?"linear-gradient(135deg,#ff0066,#ffcc00,#00ffcc,#ff00ff)":"linear-gradient(135deg,#633,#653,#356,#636)") + "'>Firework</button>";
  html += "<button class='btn" + String(fireMode==7?" on":"") + "' onclick='setMode(7)' style='background:" + String(fireMode==7?"linear-gradient(180deg,#001133,#ffff00,#001133)":"linear-gradient(180deg,#223,#553,#223)") + "'>Storm</button>";
  html += "<button class='btn" + String(fireMode==8?" on":"") + "' onclick='setMode(8)' style='background:" + String(fireMode==8?"linear-gradient(180deg,#000,#00aaff,#000)":"linear-gradient(180deg,#111,#234,#111)") + "'>Rain</button>";
  html += "<button class='btn" + String(fireMode==9?" on":"") + "' onclick='setMode(9)' style='background:" + String(fireMode==9?"linear-gradient(180deg,#228b22,#8b4513,#228b22)":"linear-gradient(180deg,#243,#432,#243)") + "'>Tree</button>";
  html += "</div></div>";

  html += "<div class='box'>Update interval: <span id='sv'>" + String(flickerSpeed) + "</span> ms<br>";
  html += "<input type='range' id='sp' min='5' max='100' value='" + String(flickerSpeed) + "' oninput='sv.innerText=this.value' onchange='send()'></div>";

  html += "<div class='box'>Brightness: <span id='bv'>" + String(maxBrightness) + "</span><br>";
  html += "<input type='range' id='br' min='10' max='255' value='" + String(maxBrightness) + "' oninput='bv.innerText=this.value' onchange='send()'></div>";

  html += "<div class='box' style='text-align:center'><a href='/wifi' style='color:#ff6600'>WiFi Settings</a></div>";

  html += "<div id='adv' class='box' style='display:none'>LED count: <span id='lv'>" + String(numLeds) + "</span><br>";
  html += "<input type='range' id='leds' min='1' max='300' value='" + String(numLeds) + "' oninput='lv.innerText=this.value'>";
  html += "<button class='btn' style='background:#ff6600;margin-top:10px' onclick='setLeds()'>Apply & Restart</button></div>";

  html += "<script>var m=" + String(fireMode) + ";";
  // Генерируем пресеты из C++ массивов (единая точка правды)
  html += "var presets={";
  for (int i = 1; i <= 9; i++) {
    if (i > 1) html += ",";
    html += String(i) + ":{s:" + String(presetSpeed[i]) + ",b:" + String(presetBrightness[i]) + "}";
  }
  html += "};";
  html += "var colors={1:['#aa2200','#522'],2:['#ff6600','#633'],3:['#ff9922','#653'],4:['#0099ff','#446'],5:['linear-gradient(90deg,red,orange,yellow,green,blue,violet)','linear-gradient(90deg,#633,#663,#363,#336,#636)'],6:['linear-gradient(135deg,#ff0066,#ffcc00,#00ffcc,#ff00ff)','linear-gradient(135deg,#633,#653,#356,#636)'],7:['linear-gradient(180deg,#001133,#ffff00,#001133)','linear-gradient(180deg,#223,#553,#223)'],8:['linear-gradient(180deg,#000,#00aaff,#000)','linear-gradient(180deg,#111,#234,#111)'],9:['linear-gradient(180deg,#228b22,#8b4513,#228b22)','linear-gradient(180deg,#243,#432,#243)']};";
  html += "var lastMode=localStorage.getItem('lastMode')||2;";
  html += "function setMode(n){m=n;lastMode=n;localStorage.setItem('lastMode',n);sp.value=presets[n].s;br.value=presets[n].b;sv.innerText=presets[n].s;bv.innerText=presets[n].b;upd();send();updPwr();}";
  html += "function upd(){document.querySelectorAll('.btns .btn').forEach(function(b,i){var n=i+1;b.className='btn'+(n==m?' on':'');b.style.background=colors[n][n==m?0:1];});}";
  html += "function send(){fetch('/set?mode='+m+'&speed='+sp.value+'&bright='+br.value);}";
  html += "function togglePower(){if(m>0){lastMode=m;localStorage.setItem('lastMode',m);m=0;}else{m=parseInt(lastMode);sp.value=presets[m].s;br.value=presets[m].b;sv.innerText=presets[m].s;bv.innerText=presets[m].b;}upd();send();updPwr();}";
  html += "function updPwr(){var p=document.getElementById('pwr');p.style.background=m==0?'#333':'#ff6600';p.innerHTML=m==0?'&#9788;':'&#9728;';}";
  html += "function setLeds(){fetch('/setleds?n='+document.getElementById('leds').value).then(()=>location.reload());}</script>";

  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleSet() {
  if (server.hasArg("mode")) fireMode = constrain(server.arg("mode").toInt(), 0, 9);
  if (server.hasArg("speed")) flickerSpeed = constrain(server.arg("speed").toInt(), 5, 100);
  if (server.hasArg("bright")) maxBrightness = constrain(server.arg("bright").toInt(), 10, 255);
  saveSettings();
  updateDisplay();
  server.send(200, "text/plain", "OK");
}

void handleSetLeds() {
  if (server.hasArg("n")) {
    numLeds = constrain(server.arg("n").toInt(), 1, MAX_LEDS);
    saveSettings();
    server.send(200, "text/plain", "OK");
    delay(500);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Missing parameter");
  }
}

void handleWiFi() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>WiFi Settings</title>";
  html += "<style>";
  html += "body{font-family:Arial;background:#1a1a1a;color:#fff;padding:20px}";
  html += ".box{background:#333;padding:15px;border-radius:10px;margin:10px 0}";
  html += "input[type=text],input[type=password]{width:100%;padding:10px;margin:5px 0;border:none;border-radius:5px;box-sizing:border-box}";
  html += ".btn{width:100%;padding:12px;border:none;border-radius:8px;background:#ff6600;color:#fff;margin:5px 0}";
  html += ".net{padding:10px;background:#444;margin:5px 0;border-radius:5px;cursor:pointer}";
  html += ".net:hover{background:#555}";
  html += "a{color:#ff6600}";
  html += "</style></head><body>";
  html += "<h2 style='color:#ff6600;text-align:center'>WiFi Settings</h2>";
  html += "<p>Current: <b>" + WiFi.SSID() + "</b> (" + WiFi.localIP().toString() + ")</p>";
  html += "<div class='box'><div id='nets'>Scanning...</div></div>";
  html += "<div class='box'><input type='text' id='ssid' placeholder='Network name'>";
  html += "<input type='password' id='pass' placeholder='Password'>";
  html += "<button class='btn' onclick='conn()'>Connect</button></div>";
  html += "<div class='box' style='text-align:center'><a href='/'>Back to Fireplace</a></div>";
  html += "<script>";
  html += "function scan(){fetch('/wifi/scan').then(r=>r.json()).then(d=>{";
  html += "var h='';d.forEach(n=>{h+='<div class=\"net\" onclick=\"document.getElementById(\\'ssid\\').value=\\''+n.ssid+'\\'\">';";
  html += "h+=n.ssid+' ('+n.rssi+'dBm)</div>';});";
  html += "document.getElementById('nets').innerHTML=h||'No networks found';});}";
  html += "function conn(){var s=document.getElementById('ssid').value;var p=document.getElementById('pass').value;";
  html += "if(!s){alert('Enter network name');return;}";
  html += "document.getElementById('nets').innerHTML='Connecting to '+s+'...';";
  html += "fetch('/wifi/connect?ssid='+encodeURIComponent(s)+'&pass='+encodeURIComponent(p)).then(r=>r.text()).then(t=>alert(t));}";
  html += "scan();</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleWiFiScan() {
  int n = WiFi.scanNetworks();
  String json = "[";
  for (int i = 0; i < n; i++) {
    if (i > 0) json += ",";
    json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
  }
  json += "]";
  WiFi.scanDelete();
  server.send(200, "application/json", json);
}

void handleWiFiConnect() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  if (ssid.length() == 0) {
    server.send(400, "text/plain", "SSID required");
    return;
  }

  WiFi.begin(ssid.c_str(), pass.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    server.send(200, "text/plain", "Connected! New IP: " + WiFi.localIP().toString());
  } else {
    WiFi.begin();  // reconnect to previous network
    server.send(200, "text/plain", "Failed to connect to " + ssid);
  }
}

void loadSettings() {
  EEPROM.begin(EEPROM_SIZE);
  if (EEPROM.read(ADDR_MAGIC) == MAGIC_VALUE) {
    fireMode = constrain(EEPROM.read(ADDR_MODE), 0, 9);
    flickerSpeed = constrain(EEPROM.read(ADDR_SPEED), 5, 100);
    maxBrightness = constrain(EEPROM.read(ADDR_BRIGHTNESS), 10, 255);
    int savedLeds = EEPROM.read(ADDR_NUM_LEDS_L) | (EEPROM.read(ADDR_NUM_LEDS_H) << 8);
    if (savedLeds >= 1 && savedLeds <= MAX_LEDS) {
      numLeds = savedLeds;
    }
    Serial.println("Settings loaded from EEPROM");
  } else {
    Serial.println("No saved settings, using defaults");
  }
}

void saveSettings() {
  EEPROM.write(ADDR_MAGIC, MAGIC_VALUE);
  EEPROM.write(ADDR_MODE, fireMode);
  EEPROM.write(ADDR_SPEED, flickerSpeed);
  EEPROM.write(ADDR_BRIGHTNESS, maxBrightness);
  EEPROM.write(ADDR_NUM_LEDS_L, numLeds & 0xFF);
  EEPROM.write(ADDR_NUM_LEDS_H, (numLeds >> 8) & 0xFF);
  EEPROM.commit();
}

#if HAS_OLED
void updateDisplay() {
  display.clear();

  // Строка 1: IP адрес (мелкий шрифт, сверху)
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 0, WiFi.localIP().toString());

  // Строка 2: Режим (средний шрифт)
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
#else
void updateDisplay() {
  // Без OLED - ничего не делаем
}
#endif

#if HAS_BUTTON
void handleButton() {
  bool btnState = digitalRead(BTN_PIN);
  unsigned long now = millis();

  // Нажатие кнопки (HIGH -> LOW)
  if (btnState == LOW && lastBtnState == HIGH) {
    #if HAS_OLED
      // Если экран выключен - просто включаем его и выходим
      if (!screenOn) {
        display.displayOn();
        screenOn = true;
        showingWifiBeforeSleep = false;
        lastActivityTime = now;
        lastBtnState = btnState;
        updateDisplay();
        Serial.println("Screen wake up");
        return;
      }
      // Сброс таймера засыпания при активности
      lastActivityTime = now;
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
        lastClickTime = 0;  // сброс чтобы не менялся режим
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
            // Останавливаемся на максимуме, направление сменим при следующем удержании
          } else if (newBright <= 10) {
            maxBrightness = 10;
            // Останавливаемся на минимуме
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
      else brightDirection = -brightDirection;  // инвертируем

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
      if (fireMode > 9) fireMode = 1;

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
void handleButton() {
  // Без кнопки - ничего не делаем
}
#endif

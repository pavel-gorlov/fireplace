// === КОНФИГУРАЦИЯ LED ===
#define LED_MATRIX 1       // 1 = матрица 8x8, 0 = лента
#define LED_PIN 0          // GPIO0 = D3

// === КНОПКА ===
#define BTN_PIN 13         // GPIO13 = D7

#if LED_MATRIX
  #define DEFAULT_NUM_LEDS 64
  #define LED_TYPE WS2812
#else
  #define DEFAULT_NUM_LEDS 144
  #define LED_TYPE WS2811
#endif

#define MAX_LEDS 300
#include "FastLED.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <EEPROM.h>
#include <Wire.h>
#include <SSD1306Wire.h>

// OLED дисплей 128x64 на I2C (HW-364A: SDA=GPIO14/D5, SCL=GPIO12/D6)
#define OLED_SDA 14
#define OLED_SCL 12
SSD1306Wire display(0x3C, OLED_SDA, OLED_SCL);

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
byte getTargetBrightness();
CRGB getFireColor(byte bright);
void loadSettings();
void saveSettings();
void updateDisplay();
void handleButton();

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

// Засыпание экрана
unsigned long lastActivityTime = 0;   // время последней активности
bool screenOn = true;                 // экран включён
#define SCREEN_TIMEOUT 30000          // мс до засыпания (30 сек)

// Названия режимов
const char* modeNames[] = {"", "Embers", "Fire", "Flame", "Ice", "Rainbow", "Firework", "Storm"};

// Фейерверк: структура взрыва
#define MAX_BURSTS 4
#define MATRIX_SIZE 8
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

void setup() {
  Serial.begin(115200);

  // Инициализация кнопки
  pinMode(BTN_PIN, INPUT_PULLUP);

  // Инициализация OLED
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.clear();
  display.drawString(64, 24, "Connecting...");
  display.display();

  loadSettings();

  FastLED.addLeds<LED_TYPE, LED_PIN, GRB>(leds, numLeds).setCorrection(TypicalLEDStrip);

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
  
  // WiFiManager - автоматически создаёт точку доступа при первом запуске
  WiFiManager wm;

  // Раскомментируй для сброса настроек WiFi при каждом запуске (для отладки)
  // wm.resetSettings();

  // Таймаут портала настройки (секунды). 0 = без таймаута
  wm.setConfigPortalTimeout(180);

  Serial.println();
  Serial.println("Connecting to WiFi...");

  // autoConnect создаёт AP "Fireplace-Setup" если нет сохранённых данных
  // После ввода credentials перезагружается и подключается к WiFi
  if (!wm.autoConnect("Fireplace-Setup", "12345678")) {
    Serial.println("Failed to connect, restarting...");
    delay(3000);
    ESP.restart();
  }

  Serial.println("WiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  updateDisplay();
  lastActivityTime = millis();  // Запуск таймера засыпания экрана

  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/setleds", handleSetLeds);
  server.on("/wifi", handleWiFi);
  server.on("/wifi/scan", handleWiFiScan);
  server.on("/wifi/connect", handleWiFiConnect);
  server.begin();
}

void loop() {
  server.handleClient();
  handleButton();

  // Засыпание экрана по таймауту
  if (screenOn && millis() - lastActivityTime > SCREEN_TIMEOUT) {
    display.displayOff();
    screenOn = false;
    Serial.println("Screen off");
  }

  if (millis() - lastUpdate > UPDATE_INTERVAL) {
    lastUpdate = millis();
    updateFire();
  }
}

void updateFire() {
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

byte getTargetBrightness() {
  switch (fireMode) {
    case 1: return random8(20, 120);   // Embers: тусклые угли
    case 2: return random8(40, 200);   // Fire: средний огонь
    case 3: return random8(30, 255);   // Flame: от углей до яркого пламени
    case 4: return random8(30, 255);   // Ice: ледяное пламя
    case 5: return 255;                // Rainbow: полная яркость
    case 6: return 255;                // Firework: полная яркость
    case 7: return 255;                // Storm: полная яркость
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
  
  html += "<div class='box'><div class='btns'>";
  html += "<button class='btn" + String(fireMode==1?" on":"") + "' onclick='setMode(1)' style='background:" + String(fireMode==1?"#aa2200":"#522") + "'>Embers</button>";
  html += "<button class='btn" + String(fireMode==2?" on":"") + "' onclick='setMode(2)' style='background:" + String(fireMode==2?"#ff6600":"#633") + "'>Fire</button>";
  html += "<button class='btn" + String(fireMode==3?" on":"") + "' onclick='setMode(3)' style='background:" + String(fireMode==3?"#ff9922":"#653") + "'>Flame</button>";
  html += "<button class='btn" + String(fireMode==4?" on":"") + "' onclick='setMode(4)' style='background:" + String(fireMode==4?"#0099ff":"#446") + "'>Ice</button>";
  html += "<button class='btn" + String(fireMode==5?" on":"") + "' onclick='setMode(5)' style='background:" + String(fireMode==5?"linear-gradient(90deg,red,orange,yellow,green,blue,violet)":"linear-gradient(90deg,#633,#663,#363,#336,#636)") + "'>Rainbow</button>";
  html += "</div><div class='btns' style='margin-top:10px'>";
  html += "<button class='btn" + String(fireMode==6?" on":"") + "' onclick='setMode(6)' style='background:" + String(fireMode==6?"linear-gradient(135deg,#ff0066,#ffcc00,#00ffcc,#ff00ff)":"linear-gradient(135deg,#633,#653,#356,#636)") + "'>Firework</button>";
  html += "<button class='btn" + String(fireMode==7?" on":"") + "' onclick='setMode(7)' style='background:" + String(fireMode==7?"linear-gradient(180deg,#001133,#ffff00,#001133)":"linear-gradient(180deg,#223,#553,#223)") + "'>Storm</button>";
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
  html += "var presets={1:{s:70,b:70},2:{s:30,b:150},3:{s:5,b:255},4:{s:10,b:200},5:{s:20,b:255},6:{s:30,b:255},7:{s:50,b:200}};";
  html += "var colors={1:['#aa2200','#522'],2:['#ff6600','#633'],3:['#ff9922','#653'],4:['#0099ff','#446'],5:['linear-gradient(90deg,red,orange,yellow,green,blue,violet)','linear-gradient(90deg,#633,#663,#363,#336,#636)'],6:['linear-gradient(135deg,#ff0066,#ffcc00,#00ffcc,#ff00ff)','linear-gradient(135deg,#633,#653,#356,#636)'],7:['linear-gradient(180deg,#001133,#ffff00,#001133)','linear-gradient(180deg,#223,#553,#223)']};";
  html += "function setMode(n){m=n;sp.value=presets[n].s;br.value=presets[n].b;sv.innerText=presets[n].s;bv.innerText=presets[n].b;upd();send();}";
  html += "function upd(){document.querySelectorAll('.btns .btn').forEach(function(b,i){var n=i+1;b.className='btn'+(n==m?' on':'');b.style.background=colors[n][n==m?0:1];});}";
  html += "function send(){fetch('/set?mode='+m+'&speed='+sp.value+'&bright='+br.value);}";
  html += "function setLeds(){fetch('/setleds?n='+document.getElementById('leds').value).then(()=>location.reload());}</script>";

  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleSet() {
  if (server.hasArg("mode")) fireMode = constrain(server.arg("mode").toInt(), 1, 7);
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
    fireMode = constrain(EEPROM.read(ADDR_MODE), 1, 5);
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

void handleButton() {
  bool btnState = digitalRead(BTN_PIN);
  unsigned long now = millis();

  // Нажатие кнопки (HIGH -> LOW)
  if (btnState == LOW && lastBtnState == HIGH) {
    // Если экран выключен - просто включаем его и выходим
    if (!screenOn) {
      display.displayOn();
      screenOn = true;
      lastActivityTime = now;
      lastBtnState = btnState;
      Serial.println("Screen wake up");
      return;
    }

    // Сброс таймера засыпания при активности
    lastActivityTime = now;

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
      if (fireMode > 7) fireMode = 1;

      saveSettings();
      updateDisplay();
      Serial.print("Mode: ");
      Serial.println(modeNames[fireMode]);

      lastClickTime = 0;
    }
  }

  lastBtnState = btnState;
}

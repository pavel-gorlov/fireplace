#define NUM_LEDS 144
#include "FastLED.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#define PIN 0

void handleRoot();
void handleSet();
void updateFire();
byte getTargetBrightness();
CRGB getFireColor(byte bright);

CRGB leds[NUM_LEDS];
ESP8266WebServer server(80);

int fireMode = 2;
int flickerSpeed = 30;
int maxBrightness = 255;

byte brightness[NUM_LEDS];
byte targetBrightness[NUM_LEDS];

unsigned long lastUpdate = 0;

void setup() {
  Serial.begin(115200);
  
  FastLED.addLeds<WS2811, PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(maxBrightness);
  
  for (int i = 0; i < NUM_LEDS; i++) {
    brightness[i] = random8();
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
  
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.begin();
}

void loop() {
  server.handleClient();
  
  if (millis() - lastUpdate > (unsigned long)(flickerSpeed / 3)) {
    lastUpdate = millis();
    updateFire();
  }
}

void updateFire() {
  for (int i = 0; i < NUM_LEDS; i++) {
    if (brightness[i] < targetBrightness[i]) {
      brightness[i]++;
    } else if (brightness[i] > targetBrightness[i]) {
      brightness[i]--;
    } else {
      targetBrightness[i] = getTargetBrightness();
    }
    leds[i] = getFireColor(brightness[i]);
  }
  
  FastLED.setBrightness(maxBrightness);
  FastLED.show();
}

byte getTargetBrightness() {
  switch (fireMode) {
    case 1: return random8(20, 120);   // Embers: тусклые угли
    case 2: return random8(40, 200);   // Fire: средний огонь
    case 3: return random8(30, 255);   // Flame: от углей до яркого пламени
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
  } else {
    // Flame: красный -> жёлтый
    hue = map(bright, 0, 255, 0, 45);
    sat = random8(200, 255);
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
  
  html += "<h2 style='color:#ff6600;text-align:center'>Fireplace</h2>";
  
  html += "<div class='box'><div class='btns'>";
  html += "<button class='btn" + String(fireMode==1?" on":"") + "' onclick='m=1;upd();send()'>Embers</button>";
  html += "<button class='btn" + String(fireMode==2?" on":"") + "' onclick='m=2;upd();send()'>Fire</button>";
  html += "<button class='btn" + String(fireMode==3?" on":"") + "' onclick='m=3;upd();send()'>Flame</button>";
  html += "</div></div>";

  html += "<div class='box'>Update interval: <span id='sv'>" + String(flickerSpeed) + "</span> ms<br>";
  html += "<input type='range' id='sp' min='5' max='100' value='" + String(flickerSpeed) + "' oninput='sv.innerText=this.value' onchange='send()'></div>";

  html += "<div class='box'>Brightness: <span id='bv'>" + String(maxBrightness) + "</span><br>";
  html += "<input type='range' id='br' min='10' max='255' value='" + String(maxBrightness) + "' oninput='bv.innerText=this.value' onchange='send()'></div>";

  html += "<script>var m=" + String(fireMode) + ";";
  html += "function upd(){document.querySelectorAll('.btn').forEach(function(b,i){b.className='btn'+(i+1==m?' on':'')});}";
  html += "function send(){fetch('/set?mode='+m+'&speed='+sp.value+'&bright='+br.value);}</script>";
  
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleSet() {
  if (server.hasArg("mode")) fireMode = constrain(server.arg("mode").toInt(), 1, 3);
  if (server.hasArg("speed")) flickerSpeed = constrain(server.arg("speed").toInt(), 5, 100);
  if (server.hasArg("bright")) maxBrightness = constrain(server.arg("bright").toInt(), 10, 255);
  server.send(200, "text/plain", "OK");
}
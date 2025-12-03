#define NUM_LEDS 144
#include "FastLED.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <EEPROM.h>
#define PIN 0

// EEPROM addresses
#define EEPROM_SIZE 16
#define ADDR_MAGIC 0      // 1 byte - маркер валидности данных
#define ADDR_MODE 1       // 1 byte
#define ADDR_SPEED 2      // 1 byte
#define ADDR_BRIGHTNESS 3 // 1 byte
#define MAGIC_VALUE 0x42  // маркер что данные записаны

void handleRoot();
void handleSet();
void handleWiFi();
void handleWiFiConnect();
void handleWiFiScan();
void updateFire();
byte getTargetBrightness();
CRGB getFireColor(byte bright);
void loadSettings();
void saveSettings();

CRGB leds[NUM_LEDS];
ESP8266WebServer server(80);

int fireMode = 2;
int flickerSpeed = 30;
int maxBrightness = 255;

float brightness[NUM_LEDS];
byte targetBrightness[NUM_LEDS];

unsigned long lastUpdate = 0;
#define UPDATE_INTERVAL 10  // фиксированный интервал обновления (мс)

void setup() {
  Serial.begin(115200);

  loadSettings();

  FastLED.addLeds<WS2811, PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(maxBrightness);
  
  for (int i = 0; i < NUM_LEDS; i++) {
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
  
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/wifi", handleWiFi);
  server.on("/wifi/scan", handleWiFiScan);
  server.on("/wifi/connect", handleWiFiConnect);
  server.begin();
}

void loop() {
  server.handleClient();

  if (millis() - lastUpdate > UPDATE_INTERVAL) {
    lastUpdate = millis();
    updateFire();
  }
}

void updateFire() {
  // Скорость изменения: чем меньше flickerSpeed, тем быстрее
  // Множитель для режима: Embers ещё медленнее
  float modeMultiplier = (fireMode == 1) ? 3.0 : (fireMode == 2) ? 1.5 : 1.0;
  float step = 255.0 / (flickerSpeed * 10 * modeMultiplier);

  for (int i = 0; i < NUM_LEDS; i++) {
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
    // Flame: красный -> оранжево-жёлтый
    hue = map(bright, 0, 255, 0, 35);
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
  
  html += "<h2 style='color:#ff6600;text-align:center'>Fireplace</h2>";
  
  html += "<div class='box'><div class='btns'>";
  html += "<button class='btn" + String(fireMode==1?" on":"") + "' onclick='setMode(1)'>Embers</button>";
  html += "<button class='btn" + String(fireMode==2?" on":"") + "' onclick='setMode(2)'>Fire</button>";
  html += "<button class='btn" + String(fireMode==3?" on":"") + "' onclick='setMode(3)'>Flame</button>";
  html += "</div></div>";

  html += "<div class='box'>Update interval: <span id='sv'>" + String(flickerSpeed) + "</span> ms<br>";
  html += "<input type='range' id='sp' min='5' max='100' value='" + String(flickerSpeed) + "' oninput='sv.innerText=this.value' onchange='send()'></div>";

  html += "<div class='box'>Brightness: <span id='bv'>" + String(maxBrightness) + "</span><br>";
  html += "<input type='range' id='br' min='10' max='255' value='" + String(maxBrightness) + "' oninput='bv.innerText=this.value' onchange='send()'></div>";

  html += "<div class='box' style='text-align:center'><a href='/wifi' style='color:#ff6600'>WiFi Settings</a></div>";

  html += "<script>var m=" + String(fireMode) + ";";
  html += "var presets={1:{s:70,b:70},2:{s:30,b:150},3:{s:5,b:255}};";
  html += "function setMode(n){m=n;sp.value=presets[n].s;br.value=presets[n].b;sv.innerText=presets[n].s;bv.innerText=presets[n].b;upd();send();}";
  html += "function upd(){document.querySelectorAll('.btn').forEach(function(b,i){b.className='btn'+(i+1==m?' on':'')});}";
  html += "function send(){fetch('/set?mode='+m+'&speed='+sp.value+'&bright='+br.value);}</script>";

  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleSet() {
  if (server.hasArg("mode")) fireMode = constrain(server.arg("mode").toInt(), 1, 3);
  if (server.hasArg("speed")) flickerSpeed = constrain(server.arg("speed").toInt(), 5, 100);
  if (server.hasArg("bright")) maxBrightness = constrain(server.arg("bright").toInt(), 10, 255);
  saveSettings();
  server.send(200, "text/plain", "OK");
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
    fireMode = constrain(EEPROM.read(ADDR_MODE), 1, 3);
    flickerSpeed = constrain(EEPROM.read(ADDR_SPEED), 5, 100);
    maxBrightness = constrain(EEPROM.read(ADDR_BRIGHTNESS), 10, 255);
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
  EEPROM.commit();
}

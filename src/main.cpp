#define NUM_LEDS 144
#include "FastLED.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
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
  
  WiFi.softAP("Fireplace", "12345678");
  Serial.println();
  Serial.println("WiFi: Fireplace / 12345678");
  Serial.print("Open: http://");
  Serial.println(WiFi.softAPIP());
  
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
    case 1: return random8(20, 120);
    case 2: return random8(60, 200);
    case 3: return random8(100, 255);
    default: return random8(60, 200);
  }
}

CRGB getFireColor(byte bright) {
  byte hue, sat, val;
  
  if (fireMode == 1) {
    hue = random8(0, 20);
    sat = 255;
    val = map(bright, 0, 255, 10, 150);
  } else if (fireMode == 2) {
    hue = map(bright, 0, 255, 0, 32);
    sat = 255;
    val = map(bright, 0, 255, 30, 220);
  } else {
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
  html += ".go{background:#ff6600;color:#fff;width:100%;padding:12px;border:none;border-radius:8px;margin-top:10px}";
  html += "</style></head><body>";
  
  html += "<h2 style='color:#ff6600;text-align:center'>Fireplace</h2>";
  
  html += "<div class='box'><div class='btns'>";
  html += "<button class='btn" + String(fireMode==1?" on":"") + "' onclick='m=1;upd()'>Embers</button>";
  html += "<button class='btn" + String(fireMode==2?" on":"") + "' onclick='m=2;upd()'>Fire</button>";
  html += "<button class='btn" + String(fireMode==3?" on":"") + "' onclick='m=3;upd()'>Flame</button>";
  html += "</div></div>";
  
  html += "<div class='box'>Speed: <span id='sv'>" + String(flickerSpeed) + "</span><br>";
  html += "<input type='range' id='sp' min='5' max='100' value='" + String(flickerSpeed) + "' oninput='sv.innerText=this.value'></div>";
  
  html += "<div class='box'>Brightness: <span id='bv'>" + String(maxBrightness) + "</span><br>";
  html += "<input type='range' id='br' min='10' max='255' value='" + String(maxBrightness) + "' oninput='bv.innerText=this.value'></div>";
  
  html += "<button class='go' onclick='send()'>Apply</button>";
  
  html += "<script>var m=" + String(fireMode) + ";";
  html += "function upd(){document.querySelectorAll('.btn').forEach(function(b,i){b.className='btn'+(i+1==m?' on':'')});}";
  html += "function send(){fetch('/set?mode='+m+'&speed='+sp.value+'&bright='+br.value).then(function(){location.reload();});}</script>";
  
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleSet() {
  if (server.hasArg("mode")) fireMode = constrain(server.arg("mode").toInt(), 1, 3);
  if (server.hasArg("speed")) flickerSpeed = constrain(server.arg("speed").toInt(), 5, 100);
  if (server.hasArg("bright")) maxBrightness = constrain(server.arg("bright").toInt(), 10, 255);
  server.send(200, "text/plain", "OK");
}
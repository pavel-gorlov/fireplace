#pragma once

#include "../config.h"
#include "../globals.h"
#include "../storage.h"
#include "../display.h"
#include "page_main.h"
#include "page_wifi.h"

inline void handleSet() {
  if (server.hasArg("mode")) fireMode = constrain(server.arg("mode").toInt(), 0, MODE_COUNT - 1);
  if (server.hasArg("speed")) flickerSpeed = constrain(server.arg("speed").toInt(), 5, 100);
  if (server.hasArg("bright")) maxBrightness = constrain(server.arg("bright").toInt(), 10, 255);
  saveSettings();
  updateDisplay();
  server.send(200, "text/plain", "OK");
}

inline void handleSetLeds() {
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

inline void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/setleds", handleSetLeds);
  server.on("/wifi", handleWiFi);
  server.on("/wifi/scan", handleWiFiScan);
  server.on("/wifi/connect", handleWiFiConnect);
}

inline void startWebServer() {
  server.begin();
}

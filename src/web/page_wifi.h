#pragma once

#include "../config.h"
#include "../globals.h"
#include <ESP8266WiFi.h>

inline void handleWiFi() {
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

inline void handleWiFiScan() {
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

inline void handleWiFiConnect() {
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

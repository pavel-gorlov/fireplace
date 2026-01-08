#pragma once

#include "../config.h"
#include "../globals.h"

inline void handleRoot() {
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
  // Генерируем пресеты из C++ массивов
  html += "var presets={";
  for (int i = 1; i < MODE_COUNT; i++) {
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

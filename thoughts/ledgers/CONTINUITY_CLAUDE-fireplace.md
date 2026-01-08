# Session: fireplace
Updated: 2026-01-08T09:10:41.367Z

## Goal
ESP8266 LED controller with multiple visual modes, OLED display, and button control.

Success criteria:
- All 9 modes work reliably on ESP8266 + 8x8 WS2812 matrix
- OLED shows status (IP, mode, speed, brightness)
- Button control for mode switching and parameter adjustment
- Settings persist in EEPROM
- Web interface fully functional

## Constraints
- **Platform**: ESP8266 NodeMCU (HW-364A board)
- **Framework**: Arduino/PlatformIO
- **Language**: C++ with Russian comments
- **LED**: WS2812 8x8 matrix (64 LEDs) on GPIO0 (D3)
- **OLED**: SSD1306 128x64 on I2C (SDA=GPIO14, SCL=GPIO12)
- **Button**: GPIO13 (D7) with INPUT_PULLUP
- **Libraries**: FastLED 3.10.3, WiFiManager 0.16.0, SSD1306Wire
- **Build**: `~/.platformio/penv/bin/pio run`

## Key Decisions
- Matrix axes: visual top-to-bottom = x in idx formula (discovered via Storm/Rain bugs)
- Button control: single click = mode, hold = brightness, double+hold = speed
- Screen sleep: 30s timeout, first press wakes without action
- Firework: expanding rings with fill_solid each frame (no residue)
- Storm: lightning path array, random deviation per row
- Rain: drops with splash effect (upper semicircle)
- Tree: brown trunk at bottom center, green crown with sin-wave leaf animation

## State
- Done:
  - [x] OLED display integration (custom I2C pins)
  - [x] WS2812 8x8 matrix support
  - [x] Button control (mode/brightness/speed)
  - [x] Screen sleep with wake on press
  - [x] Firework mode (v0.3.2)
  - [x] Storm mode (v0.3.3)
  - [x] Rain mode (v0.3.4)
  - [x] Tree mode (v0.3.5)
- Now: Session idle - awaiting user request
- Next: Potential new modes or features

## Working Set
- **Entry point**: `src/main.cpp`
- **Config**: `platformio.ini`
- **Build**: `~/.platformio/penv/bin/pio run`
- **Upload**: `pio run -t upload`
- **Current version**: v0.3.6
- **Branch**: master

## Open Questions
(None - all features working)

## Current Modes (9 total)
1. **Embers** - Тлеющие угли (slow, dim red-orange)
2. **Fire** - Средний огонь (balanced orange)
3. **Flame** - Яркое пламя (fast, bright yellow)
4. **Ice** - Ледяное пламя (blue flame)
5. **Rainbow** - Радуга (color cycling)
6. **Firework** - Фейерверк (expanding colorful rings)
7. **Storm** - Гроза (lightning on dark blue background)
8. **Rain** - Дождь (falling drops with splash)
9. **Tree** - Дерево (swaying green crown on brown trunk)

## Hardware Notes
- Matrix coordinate system: idx = row * 8 + col
- Visual top-to-bottom corresponds to col (x in idx)
- Visual left-to-right corresponds to row (y in idx)
- Button: 4-pin tactile, connect diagonal pins

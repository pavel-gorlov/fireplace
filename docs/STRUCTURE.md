# Структура проекта

## Обзор

```
fireplace/
├── src/                    # Исходный код
│   ├── main.cpp            # Точка входа: setup() и loop()
│   ├── config.h            # Конфигурация и константы
│   ├── globals.h           # Глобальные переменные и структуры
│   ├── storage.h           # Работа с EEPROM
│   ├── display.h           # OLED дисплей
│   ├── button.h            # Обработка кнопки
│   ├── wifi_setup.h        # WiFi и WiFiManager
│   ├── modes/              # Визуальные режимы
│   │   ├── modes.h         # Диспетчер режимов
│   │   ├── fire.h          # Embers/Fire/Flame/Ice
│   │   ├── rainbow.h       # Rainbow
│   │   ├── firework.h      # Firework
│   │   ├── storm.h         # Storm
│   │   ├── rain.h          # Rain
│   │   └── tree.h          # Tree
│   └── web/                # Веб-интерфейс
│       ├── server.h        # Роуты и обработчики
│       ├── page_main.h     # Главная страница
│       └── page_wifi.h     # Страница WiFi настроек
├── platformio.ini          # Конфигурация PlatformIO
├── README.md               # Документация проекта
├── CLAUDE.md               # Инструкции для AI-ассистента
├── wiring.svg              # Схема подключения
└── hw364a.jpg              # Фото платы HW-364A
```

## Описание файлов

### Корневые файлы конфигурации

| Файл | Описание |
|------|----------|
| `platformio.ini` | Конфигурация сборки: окружения nodemcu и d1_mini, зависимости |
| `CLAUDE.md` | Инструкции для Claude Code при работе с проектом |

### src/ — Исходный код

#### Главные файлы

| Файл | Строк | Описание |
|------|-------|----------|
| `main.cpp` | ~90 | Точка входа. Только `setup()` и `loop()`, вся логика в модулях |
| `config.h` | ~95 | Defines, пины, EEPROM адреса, enum Mode, пресеты скорости/яркости |
| `globals.h` | ~100 | Глобальные переменные, структуры эффектов (Burst, Lightning, Raindrop) |

#### Модули периферии

| Файл | Описание |
|------|----------|
| `storage.h` | `loadSettings()` / `saveSettings()` — работа с EEPROM |
| `display.h` | OLED: иконки, `updateDisplay()`, `showWiFiSetupScreen()`, screen sleep |
| `button.h` | Обработка кнопки: клик → режим, удержание → яркость, двойной+удержание → скорость |
| `wifi_setup.h` | WiFiManager: блокирующий (d1_mini) и неблокирующий (nodemcu) режимы |

#### src/modes/ — Визуальные режимы

| Файл | Режимы | Описание |
|------|--------|----------|
| `modes.h` | — | Диспетчер `updateMode()`, вызывает нужный режим по `fireMode` |
| `fire.h` | Embers, Fire, Flame, Ice | Огненные режимы с плавным мерцанием |
| `rainbow.h` | Rainbow | Радуга — переливание всеми цветами |
| `firework.h` | Firework | Фейерверк — расходящиеся цветные кольца |
| `storm.h` | Storm | Гроза — молнии на тёмно-синем фоне |
| `rain.h` | Rain | Дождь — падающие капли с брызгами |
| `tree.h` | Tree | Дерево — коричневый ствол и зелёная крона |

#### src/web/ — Веб-интерфейс

| Файл | Описание |
|------|----------|
| `server.h` | Регистрация роутов, `handleSet()`, `handleSetLeds()` |
| `page_main.h` | Главная страница: кнопки режимов, слайдеры, JS логика |
| `page_wifi.h` | Страница WiFi: сканирование сетей, подключение |

## Конфигурация сборки

### Окружения PlatformIO

| Окружение | Плата | Особенности |
|-----------|-------|-------------|
| `nodemcu` | NodeMCU v2 (HW-364A) | OLED + кнопка + матрица 8x8 |
| `d1_mini` | Wemos D1 Mini | Только лента WS2811, без OLED/кнопки |

### Build flags для d1_mini

```ini
build_flags =
    -DLED_MATRIX=0
    -DHAS_OLED=0
    -DHAS_BUTTON=0
```

## Условная компиляция

Код использует препроцессорные директивы для поддержки разных конфигураций:

```cpp
#if HAS_OLED
  // Код для OLED дисплея
#endif

#if HAS_BUTTON
  // Код для кнопки
#endif

#if LED_MATRIX
  // Матрица 8x8
#else
  // Лента WS2811
#endif
```

## Добавление нового режима

1. Создать `src/modes/mymode.h`:
```cpp
#pragma once
#include "../config.h"
#include "../globals.h"

inline void updateMyMode() {
  // Логика эффекта
  FastLED.setBrightness(maxBrightness);
  FastLED.show();
}
```

2. Добавить в `config.h`:
```cpp
enum Mode {
  // ...
  MODE_MYMODE,
  MODE_COUNT
};

const char* const modeNames[] = {..., "MyMode"};
const byte presetSpeed[] = {..., 50};
const byte presetBrightness[] = {..., 200};
```

3. Добавить в `src/modes/modes.h`:
```cpp
#include "mymode.h"

inline void updateMode() {
  // ...
  if (fireMode == MODE_MYMODE) {
    updateMyMode();
    return;
  }
}
```

4. Добавить кнопку в `src/web/page_main.h`

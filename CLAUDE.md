# Claude Code Instructions

## После изменения кода

- Обновляй README.md если изменились:
  - Режимы работы или их параметры
  - Пресеты (interval, brightness)
  - Настройки веб-интерфейса
  - Железо или пины подключения
  - Зависимости в platformio.ini

## Сборка

```bash
~/.platformio/penv/bin/pio run
```

## Структура проекта

- `src/main.cpp` — основной код
- `platformio.ini` — конфигурация PlatformIO и зависимости
- `README.md` — документация проекта

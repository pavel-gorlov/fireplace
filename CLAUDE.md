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

Подробная документация: **[docs/STRUCTURE.md](docs/STRUCTURE.md)**

Основные файлы:
- `src/main.cpp` — точка входа (setup/loop)
- `src/config.h` — конфигурация, enum Mode, пресеты
- `src/modes/` — визуальные режимы (fire, rainbow, storm, rain, tree...)
- `src/web/` — веб-интерфейс (HTML страницы)
- `platformio.ini` — конфигурация PlatformIO

## Добавление нового режима

См. секцию "Добавление нового режима" в [docs/STRUCTURE.md](docs/STRUCTURE.md)

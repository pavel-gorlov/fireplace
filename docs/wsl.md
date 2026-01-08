# Прошивка ESP8266 из WSL

## Проброс USB в WSL

Для прошивки контроллера из WSL нужно пробросить USB-Serial адаптер.

### 1. Найти устройство (PowerShell)

```powershell
usbipd list
```

Ищи устройство **USB-SERIAL CH340** (или CP210x, FTDI):

```
BUSID  VID:PID    DEVICE                    STATE
7-4    1a86:7523  USB-SERIAL CH340 (COM4)   Not shared
```

### 2. Расшарить и подключить (PowerShell от администратора)

```powershell
usbipd bind --busid 7-4      # один раз, расшаривает устройство
usbipd attach --wsl --busid 7-4
```

Если есть предупреждение про USBPcap, используй `--force`:

```powershell
usbipd bind --force --busid 7-4
```

### 3. Проверить в WSL

```bash
ls /dev/ttyUSB*
# Должен появиться /dev/ttyUSB0
```

### 4. Прошивка

```bash
~/.platformio/penv/bin/pio run -t upload
```

## Примечания

- BUSID может меняться при переподключении устройства
- После перезагрузки Windows нужно повторить `usbipd attach`
- `bind` делается один раз, `attach` — после каждой перезагрузки

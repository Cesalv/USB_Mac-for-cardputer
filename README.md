# CardPuter ADV — USB MSC para macOS

Firmware que expone la microSD del M5Stack CardPuter ADV como **pendrive USB** en macOS (y también en Windows).

## ¿Por qué el firmware original no funciona en Mac?

macOS es mucho más estricto que Windows con el protocolo USB MSC:

| Problema | Efecto en macOS |
|---|---|
| Orden de inicialización incorrecto | macOS descarta el dispositivo al enumerar |
| Strings del descriptor USB vacíos | Finder no muestra el volumen |
| Sin `syncDevice()` en STOP Unit | macOS marca el volumen como corrupto |
| Flag `ARDUINO_USB_MODE` incorrecto | El ESP32-S3 aparece como CDC, no como MSC |

Este firmware corrige los 4 problemas.

---

## Opción A — Compilar en la nube con GitHub Actions (sin instalar nada)

1. Crea una cuenta en [github.com](https://github.com) si no tienes
2. Crea un repositorio nuevo (puede ser privado)
3. Sube todos los archivos de esta carpeta manteniendo la estructura
4. Ve a la pestaña **Actions** de tu repositorio
5. El workflow se lanzará automáticamente al subir el código
6. Cuando termine (2-3 minutos), haz clic en el workflow → **Artifacts** → descarga `cardputer_adv_usb_msc_mac.zip`
7. Dentro encontrarás `firmware.bin`, `bootloader.bin` y `partitions.bin`

---

## Opción B — Compilar localmente con PlatformIO

```bash
# Instalar PlatformIO (una sola vez)
pip install platformio

# Compilar
cd cardputer_adv_msc_mac
pio run

# El .bin estará en:
# .pio/build/cardputer_adv/firmware.bin
```

---

## Flashear el firmware

### Con esptool (recomendado, multiplataforma)

```bash
pip install esptool

# Poner el CardPuter ADV en modo descarga:
# 1. Apagar (switch lateral a OFF)
# 2. Mantener pulsado BtnG0
# 3. Encender (switch a ON)
# 4. Soltar BtnG0

# Flashear (reemplaza /dev/cu.usbmodem* por tu puerto)
esptool.py --chip esp32s3 --port /dev/cu.usbmodem* --baud 921600 \
  write_flash \
  0x0000  bootloader.bin \
  0x8000  partitions.bin \
  0x10000 firmware.bin
```

En macOS el puerto suele llamarse `/dev/cu.usbmodem*` o `/dev/cu.SLAB_USBtoUART`.
En Windows será `COM3`, `COM4`, etc.

### Con M5Burner

M5Burner no permite flashear `.bin` externos directamente.
Usa esptool o Arduino IDE (ver abajo).

### Con Arduino IDE

1. Instala Arduino IDE 2.x
2. Añade el URL de placas ESP32:
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. Instala **esp32 by Espressif** desde el Board Manager
4. Instala la librería **SdFat by Bill Greiman** desde el Library Manager
5. Selecciona placa: `ESP32S3 Dev Module`
6. Configura:
   - Flash Size: `8MB`
   - USB Mode: `USB-OTG (TinyUSB)`  ← **CRÍTICO para macOS**
   - USB CDC On Boot: `Disabled`
7. Abre `src/main.cpp`, compila y sube

---

## Pines SD (CardPuter ADV / Stamp-S3A)

| Señal | GPIO |
|-------|------|
| SCK   | 40   |
| MISO  | 39   |
| MOSI  | 14   |
| CS    | 12   |

---

## Uso

1. Flashea el firmware
2. Conecta el CardPuter ADV por USB al Mac
3. Aparecerá en el Finder como un disco externo
4. Para expulsar: arrastra el icono a la papelera (importante antes de desconectar)
5. Para volver al firmware original: reflashea el firmware de fábrica con M5Burner

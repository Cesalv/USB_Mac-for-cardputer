/**
 * ╔══════════════════════════════════════════════════════════════════════╗
 * ║   CardPuter ADV — USB MSC SD Card (macOS + Windows compatible)      ║
 * ║                                                                      ║
 * ║   Expone la microSD como pendrive USB en macOS y Windows.           ║
 * ║                                                                      ║
 * ║   Pines SD (Cardputer ADV / Stamp-S3A):                             ║
 * ║     SCK  → GPIO40                                                    ║
 * ║     MISO → GPIO39                                                    ║
 * ║     MOSI → GPIO14                                                    ║
 * ║     CS   → GPIO12                                                    ║
 * ║                                                                      ║
 * ║   ¿Por qué esto funciona en Mac y el original no?                   ║
 * ║   ─────────────────────────────────────────────────────────────────  ║
 * ║   macOS es estricto con 3 cosas que el firmware original falla:     ║
 * ║   1. Orden de init: SD → MSC.begin() → USB.begin() (en ese orden)  ║
 * ║   2. Los strings del descriptor USB deben estar antes de USB.begin() ║
 * ║   3. El callback SCSI START/STOP debe hacer flush de la SD          ║
 * ╚══════════════════════════════════════════════════════════════════════╝
 */

#include "Arduino.h"
#include "USB.h"
#include "USBMSC.h"
#include <SPI.h>
#include <SdFat.h>

// ── Pines SD ─────────────────────────────────────────────────────────────────
#define SD_SCK   40
#define SD_MISO  39
#define SD_MOSI  14
#define SD_CS    12
#define SD_SPEED SD_SCK_MHZ(20)

// ── Objetos globales ──────────────────────────────────────────────────────────
USBMSC   MSC;
SdFat    sd;
SPIClass sdSPI(FSPI);

static bool     sdReady     = false;
static uint32_t sectorCount = 0;
static const uint32_t SECTOR_SIZE = 512;

// ── LED de estado (pantalla off, usamos el LED interno si lo hay) ─────────────
// El Cardputer ADV no tiene LED de usuario accesible, se usa la pantalla
// en una versión futura. Por ahora el estado se refleja en el display G0.

// ═════════════════════════════════════════════════════════════════════════════
// CALLBACKS USB MSC
// Estas funciones las invoca TinyUSB en respuesta a comandos SCSI del host.
// ═════════════════════════════════════════════════════════════════════════════

/**
 * Lectura de sectores.
 * macOS verifica que la función devuelva exactamente bufsize bytes.
 */
static int32_t msc_read_cb(uint32_t lba, uint32_t offset,
                            void* buffer, uint32_t bufsize)
{
    if (!sdReady) return -1;

    uint32_t count = bufsize / SECTOR_SIZE;
    for (uint32_t i = 0; i < count; i++) {
        if (!sd.card()->readSector(lba + i,
                (uint8_t*)buffer + (i * SECTOR_SIZE))) {
            return -1;
        }
    }
    return (int32_t)bufsize;
}

/**
 * Escritura de sectores.
 * Se escribe sector a sector; no se usa caché interna.
 */
static int32_t msc_write_cb(uint32_t lba, uint32_t offset,
                             uint8_t* buffer, uint32_t bufsize)
{
    if (!sdReady) return -1;

    uint32_t count = bufsize / SECTOR_SIZE;
    for (uint32_t i = 0; i < count; i++) {
        if (!sd.card()->writeSector(lba + i,
                (const uint8_t*)buffer + (i * SECTOR_SIZE))) {
            return -1;
        }
    }
    return (int32_t)bufsize;
}

/**
 * START/STOP Unit — macOS lo usa al montar y al expulsar.
 * FIX CLAVE: syncDevice() antes de la expulsión evita corrupción
 * y es lo que hace que macOS marque el volumen como "sano".
 */
static bool msc_start_stop_cb(uint8_t power_condition,
                               bool start, bool load_eject)
{
    if (!start && load_eject && sdReady) {
        // El usuario ha pulsado "Expulsar" en el Finder
        sd.card()->syncDevice();
    }
    return true;
}

// ═════════════════════════════════════════════════════════════════════════════
// SETUP — Orden crítico para compatibilidad con macOS
// ═════════════════════════════════════════════════════════════════════════════
void setup()
{
    // ── Paso 1: inicializar SD ────────────────────────────────────────────
    //    Necesitamos el sectorCount antes de llamar a MSC.begin()
    sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (sd.begin(SdSpiConfig(SD_CS, DEDICATED_SPI, SD_SPEED, &sdSPI))) {
        sectorCount = sd.card()->sectorCount();
        sdReady     = true;
    }
    // Si la SD falla seguimos adelante: el host verá el dispositivo pero
    // mediaPresent=false, lo cual es correcto y no cuelga macOS.

    // ── Paso 2: strings del descriptor USB ───────────────────────────────
    //    macOS usa estos strings para identificar el dispositivo.
    //    DEBEN estar antes de USB.begin().
    USB.manufacturerName("M5Stack");
    USB.productName("CardputerADV SD");
    USB.serialNumber("CADV-SD-001");

    // ── Paso 3: configurar MSC y registrar callbacks ──────────────────────
    MSC.vendorID    ("M5Stack");      // máx 8 chars (SCSI INQUIRY)
    MSC.productID   ("CardputerSD");  // máx 16 chars
    MSC.productRevision("1.00");      // máx 4 chars
    MSC.onRead      (msc_read_cb);
    MSC.onWrite     (msc_write_cb);
    MSC.onStartStop (msc_start_stop_cb);
    MSC.mediaPresent(sdReady);

    // ── Paso 4: begin() con el número real de sectores ────────────────────
    MSC.begin(SECTOR_SIZE, sdReady ? sectorCount : 0);

    // ── Paso 5: arrancar USB — SIEMPRE el último paso ─────────────────────
    //    Si se llama antes, macOS recibe descriptores incompletos y aborta.
    USB.begin();
}

// ═════════════════════════════════════════════════════════════════════════════
// LOOP — TinyUSB gestiona todo en background, aquí no hace falta nada
// ═════════════════════════════════════════════════════════════════════════════
void loop()
{
    delay(1000);
}

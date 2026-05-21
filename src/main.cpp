/**
 * CardPuter ADV — USB MSC SD Card (macOS + Windows compatible)
 * Pines SD: SCK=40  MISO=39  MOSI=14  CS=12
 */

#include "Arduino.h"
#include "USB.h"
#include "USBMSC.h"
#include <SPI.h>
#include <SdFat.h>

#define SD_SCK   40
#define SD_MISO  39
#define SD_MOSI  14
#define SD_CS    12
#define SD_SPEED SD_SCK_MHZ(20)

USBMSC   MSC;
SdFat    sd;
SPIClass sdSPI(FSPI);

static bool     sdReady     = false;
static uint32_t sectorCount = 0;
static const uint32_t SECTOR_SIZE = 512;

static int32_t sd_read(uint32_t lba, uint32_t offset,
                        void* buffer, uint32_t bufsize)
{
    if (!sdReady) return -1;
    uint32_t count = bufsize / SECTOR_SIZE;
    for (uint32_t i = 0; i < count; i++) {
        if (!sd.card()->readSector(lba + i,
                (uint8_t*)buffer + (i * SECTOR_SIZE))) return -1;
    }
    return (int32_t)bufsize;
}

static int32_t sd_write(uint32_t lba, uint32_t offset,
                         uint8_t* buffer, uint32_t bufsize)
{
    if (!sdReady) return -1;
    uint32_t count = bufsize / SECTOR_SIZE;
    for (uint32_t i = 0; i < count; i++) {
        if (!sd.card()->writeSector(lba + i,
                (const uint8_t*)buffer + (i * SECTOR_SIZE))) return -1;
    }
    return (int32_t)bufsize;
}

static bool sd_start_stop(uint8_t power_condition,
                           bool start, bool load_eject)
{
    if (!start && load_eject && sdReady)
        sd.card()->syncDevice();
    return true;
}

void setup()
{
    sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (sd.begin(SdSpiConfig(SD_CS, DEDICATED_SPI, SD_SPEED, &sdSPI))) {
        sectorCount = sd.card()->sectorCount();
        sdReady     = true;
    }

    USB.manufacturerName("M5Stack");
    USB.productName("CardputerADV SD");
    USB.serialNumber("CADV-SD-001");

    MSC.vendorID("M5Stack");
    MSC.productID("CardputerSD");
    MSC.productRevision("1.00");
    MSC.onRead(sd_read);
    MSC.onWrite(sd_write);
    MSC.onStartStop(sd_start_stop);
    MSC.mediaPresent(sdReady);
    MSC.begin(SECTOR_SIZE, sdReady ? sectorCount : 0);

    USB.begin();
}

void loop()
{
    delay(1000);
}

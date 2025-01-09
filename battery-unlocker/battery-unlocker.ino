#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

// if your board has hardware SPI pins, just use these:
// #define PN532_SCK SCK
// #define PN532_MISO MISO
// #define PN532_MOSI MOSI
// #define PN532_SS SS

// Adafruit_PN532 nfc(PN532_SS);

// to force software SPI, define any pins and use the 4-arg constructor:
#define PN532_SCK 6
#define PN532_MISO 7
#define PN532_MOSI 8
#define PN532_SS 9

Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

// (to connect multiple devices, use the same SCK/MISO/MOSI for all and a separate CS for each)

// BATTERY_ON command
uint8_t batteryOn[] = { 0x50, 0x50, 0x50, 0x50 };
// SEATBOX_CLOSED command
uint8_t closed[] = { 0x4B, 0x4D, 0x4B, 0x4D };
// BATTERY_INSERTED_SEATBOX_OPENED?
uint8_t insertedOpened[] = { 0x41, 0x4E, 0x41, 0x44 };
// SEATBOX_OPENED
uint8_t opened[] = { 0x59, 0x52, 0x52, 0x48 };

// iteration counter
int it = 0;

void setup(void) {
  Serial.begin(115200);
  delay(200);
  
  Serial.println("Setting up PN532 with CS pin " + String(PN532_SS));

  nfc.begin();
  
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.print("No PN532 board detected, check connection, SPI mode, and pinout!");
    while (1)
      ;
  }
  Serial.print("Found chip: PN5");
  Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("firmware v");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);

  nfc.SAMConfig();
}

void loop(void) {
  uint8_t write;
  uint8_t tag;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;
  int rptTimeout = 5000;

  tag = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, rptTimeout);

  if (it%10 == 0) Serial.print("\n" + String(it, DEC));
  if (tag) {
    nfc.PrintHex(uid, uidLength);
    
    delay(500);

    Serial.print(" -> CLOSED:");
    write = nfc.ntag2xx_WritePage(0xCC, closed);
    if (write) {
      Serial.print("OK");
    } else {
      Serial.print("failed");
    }
    delay(500);

    Serial.print(" -> BATTERY_ON:");
    write = nfc.ntag2xx_WritePage(0xCC, batteryOn);
    if (write) {
      Serial.print("OK");
    } else {
      Serial.print("failed");
    }
    delay(500);
  }
  delay(5000);
  it++;
}

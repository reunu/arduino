#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

#define PN532_SCK  (2)
#define PN532_MOSI (3)
#define PN532_SS   (4)
#define PN532_MISO (5)

Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

uint8_t batteryOn[] = {0x50, 0x50, 0x50, 0x50};
uint8_t closed[] = {0x4B, 0x4D, 0x4B, 0x4D};
uint8_t insertedOpened[] = {0x41, 0x4E, 0x41, 0x44};
uint8_t opened[] = {0x59, 0x52, 0x52, 0x48};

void setup(void) {
  Serial.begin(115200);
  Serial.println("Setting up");

  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.print("No board detected");
    while (1);
  }

  nfc.SAMConfig();
  Serial.println("Waiting for an NFC tag...");
}

void loop(void) {
  uint8_t write;
  uint8_t tag;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;
  
  tag = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (tag) {
    Serial.println("Found NFC tag");

    write = nfc.ntag2xx_WritePage(0xCC, closed);
    
    if (write) {
      Serial.println("closed command written successfully.");
    } else {
      Serial.println("Failed to write closed command.");
    }
    delay(1000);

    write = nfc.ntag2xx_WritePage(0xCC, batteryOn);
    
    if (write) {
      Serial.println("batteryOn command written successfully.");
    } else {
      Serial.println("Failed to write batteryOn command.");
    }

  }
  
  delay(5000);
}

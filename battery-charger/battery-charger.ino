#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

// Initialize both NFC readers - different CS pins
Adafruit_PN532 nfc1(D5, D6, D7, D8);
Adafruit_PN532 nfc2(D5, D6, D7, D4);
Adafruit_PN532* activeNfc = nullptr;

#define DEBUG

// Commands
uint8_t YOURE_ON_THE_CHARGER_NOW[] = { 0x56, 0x58, 0x41, 0x4d };
uint8_t ARE_YOU_STILL_THERE[] = { 0x4c, 0x49, 0x55, 0x47 };
uint8_t OH_HAI_THIS_IS_BATTERY[] = { 0x49, 0x52, 0x48, 0x4d };
uint8_t BATTERY_ON[] = { 0x50, 0x50, 0x50, 0x50 };

enum BatteryState {
  BatterySleep = 0xA4983474,
  BatteryIdle = 0xB9164828, 
  BatteryActive = 0xC6583518,
  BatteryStateUnknown = 0
};

#ifdef DEBUG
void debug(String s) { Serial.println(s); }
#else
void debug(String s) {}
#endif

bool initializeNfcReader(Adafruit_PN532& nfc, const char* readerName) {
  nfc.begin();

  Serial.print(readerName);

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println(" not detected! Check PN532 connection and SPI mode!");
    return false;
  }

  Serial.print(" detected: PN5");
  Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("firmware v");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);

  return nfc.SAMConfig();
}

void setup() {
  Serial.begin(115200);
  delay(500);

  bool nfc1_ok = initializeNfcReader(nfc1, "NFC1");
  bool nfc2_ok = initializeNfcReader(nfc2, "NFC2");

  if (!nfc1_ok && !nfc2_ok) {
    Serial.println("No PN532 boards detected, check connections!");
    while (1);
  }

  Serial.println("System initialized!");
}

bool sendCommand(Adafruit_PN532& nfc, uint8_t* cmd) {
  return nfc.ntag2xx_WritePage(0xCC, cmd);
}

bool readResponse(Adafruit_PN532& nfc, uint8_t* resp) {
  return nfc.ntag2xx_ReadPage(0xCC, resp);
}

bool readBatteryStatus(Adafruit_PN532& nfc, uint16_t* voltage, int16_t* current, uint16_t* availableCurrent, uint16_t* remainingCharge) {
  uint8_t data[4];
  if (!nfc.ntag2xx_ReadPage(0xC0 + 0, data)) {
    return false;
  }
  *voltage = (data[1] << 8) | data[0];
  *current = (data[3] << 8) | data[2];
  if (!nfc.ntag2xx_ReadPage(0xC0 + 1, data)) {
    return false;
  }
  *availableCurrent = (data[1] << 8) | data[0];
  *remainingCharge = (data[3] << 8) | data[2];
  return true;
}

uint16_t readFullCharge(Adafruit_PN532& nfc) {
  uint8_t data[4];
  if (!nfc.ntag2xx_ReadPage(0xC0 + 2, data)) {
    return 0;
  }
  return (data[1] << 8) | data[0];
}

uint16_t readFaultCode(Adafruit_PN532& nfc) {
  uint8_t data[4];
  if (!nfc.ntag2xx_ReadPage(0xC0 + 2, data)) {
    return 0;
  }
  return (data[3] << 8) | data[2];
}

bool readTemperatureAndHealth(Adafruit_PN532& nfc, int8_t* temperatureA, int8_t* temperatureB, int8_t* stateOfHealth) {
  uint8_t data[4];
  if (!nfc.ntag2xx_ReadPage(0xC0 + 3, data)) {
    return false;
  }
  *temperatureA = data[0];
  *temperatureB = data[1];
  *stateOfHealth = data[2];
  // data[3] is unused
  return true;
}

BatteryState readBatteryState(Adafruit_PN532& nfc) {
  uint8_t data[4];
  for (int attempt = 0; attempt < 3; attempt++) {
    if (nfc.ntag2xx_ReadPage(0xC0 + 4, data)) {
      return (BatteryState)((data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0]);
    }
    delay(10);
  }
  return BatteryStateUnknown;
}

bool detectTag(Adafruit_PN532& nfc, uint8_t* uid, uint8_t* uidLength, uint16_t timeout) {
  return nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, uidLength, timeout);
}

bool isBufferEmpty(const uint8_t* buffer) {
  return buffer[0] == 0 && buffer[1] == 0 && buffer[2] == 0 && buffer[3] == 0;
}

bool waitForResponse(Adafruit_PN532& nfc, uint8_t* resp, const uint8_t* expected = nullptr) {
  unsigned long startTime = millis();
  uint8_t buffer[4];
  uint16_t timeout = 500;
  
  while (millis() - startTime < timeout) {
    if (!nfc.ntag2xx_ReadPage(0xCC, buffer)) {
      delay(5);
      continue;
    }

    // debug("response: " + String(buffer[0], HEX) + " " + String(buffer[1], HEX) + " " + String(buffer[2], HEX) + " " + String(buffer[3], HEX) + " @ " + String(millis()));
    
    if (expected != nullptr && memcmp(buffer, expected, 4) == 0) {
      memcpy(resp, buffer, 4);
      return true;
    }
    
    if (expected == nullptr && isBufferEmpty(buffer)) {
      memcpy(resp, buffer, 4);
      return true;
    }
    
    delay(5);
  }
  
  return false;
}

void handleBatteryConnection(Adafruit_PN532& nfc) {
  uint8_t uid[7] = {0};
  uint8_t uidLength;
  uint8_t data[4];
  uint16_t voltage, availableCurrent, remainingCharge, fullCharge;
  int16_t current;
  int8_t temperatureA, temperatureB, soh;
  BatteryState state;

  // debug("Battery detected: " + String(uid, HEX));

  state = readBatteryState(nfc);
  switch (state) {
    case BatteryIdle:
      debug("Battery idle"); break;
    case BatterySleep:
      debug("Battery asleep"); break;
    case BatteryActive:
      debug("Battery active"); break;
    case BatteryStateUnknown:
    default:
      debug("Battery state unknown"); break;
  }
  // debug("Battery state: " + String(state, HEX));
  readTemperatureAndHealth(nfc, &temperatureA, &temperatureB, &soh);
  debug("Temperature " + String(temperatureA) + " / " + String(temperatureB) + "; State of Health: " + String(soh));

  readBatteryStatus(nfc, &voltage, &current, &availableCurrent, &remainingCharge);
  Serial.println("Battery Status: Voltage = " + String(voltage) + ", Current = " + String(current) + ", availableCurrent = " + String(availableCurrent) + ", remainingCharge = " + String(remainingCharge));

  debug("Sending Charger Hello");
  if (sendCommand(nfc, YOURE_ON_THE_CHARGER_NOW)) {
    debug("Sent Charger Hello");
    
    // Wait for either empty buffer or OH_HAI response
    if (waitForResponse(nfc, data, OH_HAI_THIS_IS_BATTERY)) {
      if (memcmp(data, OH_HAI_THIS_IS_BATTERY, 4) == 0) {
        debug("Battery acknowledged charger");
        
        if (sendCommand(nfc, BATTERY_ON)) {
          debug("Battery told to turn on");
          // // Wait for empty buffer after BATTERY_ON
          // if (!waitForResponse(nfc, data)) {
          //   debug("Battery did not acknowledge ON command");
          //   return;
          // }
          
          while (detectTag(nfc, uid, &uidLength, 100)) {
            state = readBatteryState(nfc);
            switch (state) {
              case BatteryIdle:
                debug("Battery idle"); break;
              case BatterySleep:
                debug("Battery asleep"); break;
              case BatteryActive:
                debug("Battery active"); break;
              case BatteryStateUnknown:
              default:
                debug("Battery state unknown"); break;
            }

            if (!sendCommand(nfc, ARE_YOU_STILL_THERE)) {
              debug("Keepalive failed.");
              break;
            }

            if (state == BatteryIdle || state == BatterySleep) {
              debug("Battery went idle or to sleep, restarting handshake.");
              break;
            }
            
            // // Wait for empty buffer after ARE_YOU_STILL_THERE
            // if (!waitForResponse(nfc, data)) {
            //   debug("Battery not responding to keepalive");
            //   break;
            // }
            
            debug("Keepalive.");
            delay(200);
          }
        } else {
          debug("Failed sending battery on command.");
        }
      }
    } else {
      debug("No valid response received within timeout.");
    }
  }
}

void loop() {
  uint8_t uid[7] = {0};
  uint8_t uidLength;

  // If no active NFC reader is selected, try both readers
  if (activeNfc == nullptr) {
    // Try NFC1 first
    if (detectTag(nfc1, uid, &uidLength, 100)) {
      activeNfc = &nfc1;
      debug("NFC1 detected tag!");
    }
    // If NFC1 didn't detect anything, try NFC2
    else if (detectTag(nfc2, uid, &uidLength, 100)) {
      activeNfc = &nfc2;
      debug("NFC2 detected tag!");
    }
    
    // If we found a tag, handle the battery connection
    if (activeNfc != nullptr) {
      handleBatteryConnection(*activeNfc);
    }
  }
  // If we have an active reader, continue using it until connection is lost
  else {
    if (!detectTag(*activeNfc, uid, &uidLength, 100)) {
      debug("Tag lost, resetting active reader");
      activeNfc = nullptr;
    } else {
      handleBatteryConnection(*activeNfc);
    }
  }

  delay(50);
}
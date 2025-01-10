# reunu Battery Tools

A collection of Arduino-based tools for interfacing with unu Scooter Pro batteries via NFC communication. This project provides implementations for both charging and general battery unlocking operations.

## ⚠️ Safety Warnings

- **High Current Path Activation**: Both tools activate the battery's high-current path during operation. Always observe appropriate safety precautions.
- **Handling Delay**: After terminating NFC communication, wait 15-30 seconds before handling the battery to ensure the high-current path has fully deactivated.
- **Continuous Communication**: The battery requires regular communication to maintain its active state:
  - Charger mode: Requires keepalive messages every ~500ms
  - Unlocker mode: Requires command renewal every ~15 seconds

## Project Components

### Battery Charger
Implementation for emulating the official charger's NFC communication protocol. Features:
- Dual PN532 reader support for redundant communication
- Continuous battery status monitoring
- Comprehensive fault detection
- Automatic reader failover

### Battery Unlocker
Simplified implementation that emulates scooter presence detection. Features:
- Single PN532 reader support
- Basic battery unlocking functionality
- Seatbox closed state emulation

## Hardware Requirements

- Arduino-compatible microcontroller
- PN532 NFC reader(s) configured for SPI communication
- Charger implementation assumes ESP8266 with the following SPI setup:
  - Primary reader: CS on D8
  - Secondary reader: CS on D4
  - Shared SPI bus (D5: SCK, D6: MISO, D7: MOSI)

### High-Speed Microcontroller Considerations

When using ESP8266/ESP32 or other high-speed microcontrollers, the Adafruit_PN532 library requires timing modifications:

1. Locate Adafruit_PN532.cpp in your library directory
2. Modify line 336 to include consistent delay(SLOWDOWN)
3. This ensures reliable communication timing across all supported platforms

## Technical Details

### Communication Protocol Overview

Both implementations utilize NFC communication to manage the battery's state:

1. Battery Detection
   - Continuous polling for battery presence
   - UID verification and handshake initiation

2. State Management
   - Charger: Requires frequent keepalive messages
   - Unlocker: Requires periodic command renewal

3. Safety Features
   - Response timeout validation
   - Connection loss detection
   - Automatic deactivation safeguards

### Battery Memory Structure

The battery has an EEPROM (888 bytes) with limited writes, and a 64-byte SRAM with unlimited writes.
The SRAM is mapped to NFC pages:

| NFC Page | Byte 0-3 | Description |
|----------|----------|-------------|
| 0xC0 | voltage (uint16_t, 0-65535 mV) + current (int16_t, -32.768-32.767 A) | Battery voltage and current measurements |
| 0xC1 | availableCurrent (uint16_t, 0-65535 mA) + remainingCharge (uint16_t, 0-65535 mAh) | Available current limit and remaining charge |
| 0xC2 | fullCharge (uint16_t, 0-65535 mAh) + faultCode (uint16_t, 0-65535) | Total battery capacity and fault status |
| 0xC3 | temperatureA (int8_t, -128-127°C) + temperatureB (int8_t, -128-127°C) + stateOfHealth (uint8_t, 0-100%) + unused (uint8_t) | Temperature sensors, health indicator |
| 0xC4 | batteryState (enum: 0xA4983474=Sleep/0xB9164828=Idle/0xC6583518=Active) | Current battery operational state |
| 0xC5-0xCB | Unknown/unused | Unused status message area? |
| 0xCC | command/response (32-bit) | Communication buffer for commands and responses |
| 0xCD | data[0-3] | Command additional data bytes 0-3 |
| 0xCE | data[4-7] | Command additional data bytes 4-7 |
| 0xCF | data[8-11] | Command additional data bytes 8-11 |

Note: Each NFC page represents 4 bytes of the SRAM region, with the status message buffer spanning pages 0xC0-0xCB and the communication buffer occupying pages 0xCC-0xCF.

## Contributing

When contributing to either implementation, please:
1. Maintain consistent safety checks and timeout handling
2. Document any timing-critical sections
3. Test thoroughly with both standard Arduino and high-speed microcontrollers

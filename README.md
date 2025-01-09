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

Key memory pages for battery interaction:
- 0xC0: Voltage and current readings
- 0xC1: Available current and charge status
- 0xC2: Capacity and fault information
- 0xC3: Temperature and health data
- 0xC4: Battery state
- 0xCC: Command interface

## Contributing

When contributing to either implementation, please:
1. Maintain consistent safety checks and timeout handling
2. Document any timing-critical sections
3. Test thoroughly with both standard Arduino and high-speed microcontrollers

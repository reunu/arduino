# reunu Battery Unlocker

Arduino sketch to tell the main drive battery of a unu Scooter Pro that it is inside the scooter and ready to scoot, which will get the internal BMS to unlock the high-current path and allow charging or discharging the battery.

<details>
<summary>⚠️ WARNING</summary>

> **Note:** Battery will turn off after ~30 seconds if it does not receive additional messages.
</details>

<details>
<summary>🚨 DANGER</summary>

> **Warning:** Battery is not safe to handle while high current path is unlocked. Allow 15-30 seconds after removing tag writer before handling battery.
</details>

unu Scooter Pro batteries have NFC chips in them to allow the battery to communicate with devices such as the charger and the scooter. The battery will not allow charging or discharging until unlocked by one of those devices, at which point the integrated BMS will enable the GND pole and allow current to flow.

This sketch unlocks the high current path by telling the battery that it is inside the scooter and the seatbox is closed. At that point, the battery will allow drawing the full voltage and current on the discharge path, or accept charge. Note that you will not get the LED strip animation, because the battery thinks it's inside the closed seatbox. See the `battery-charger` sketch for that.

## Sequence diagram

```mermaid
sequenceDiagram
    participant NR as Battery Unlocker (PN532)
    participant B as unu Pro Battery
    
    Note over NR,B: WARNING: Commands must be re-sent every 15s or battery turns off after ~30s
    Note over NR,B: DANGER: Battery is not safe to handle while high current path is unlocked
    Note over NR,B: Allow 15-30s after removing tag writer before handling battery
    
    loop Every iteration
        NR->>B: wait for battery detection (1000ms timeout)
        B-->>NR: Battery UID
        B-->>NR: Field detection wakes up battery: Off → Idle
        
        alt Battery Detected
            Note over NR: Wait 500ms
            
            NR->>B: send BATTERY_ON command (0x50505050)
            B-->>NR: Battery unlocks high current path
            
            Note over NR: Wait 500ms

            NR->>B: send SEATBOX_CLOSED command (0x4b4d4b4d)
            B-->>NR: Battery turns off LED strip
            
            Note over NR: Wait 500ms
        end
    end
```

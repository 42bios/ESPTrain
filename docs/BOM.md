# Bill Of Materials

This list separates the parts used in the current simple build from optional upgrades for a cleaner later revision.

## Built Into The Current Simple Version

| Qty | Part | Notes |
| --- | --- | --- |
| 1 | ESP32-C3 Development Board ESP32 SuperMini | Main BLE controller |
| 1 | Amazon aftermarket SNES-style controller PCB and shell | Clone controller PCB with SNES-style serial interface |
| 1 | 803040 3.7 V LiPo battery | Fits the current controller shell layout; capacity depends on the exact cell |
| 1 | USB-C LiPo charger/protection module | HW-373 / TP4056-style module or similar |
| 1 | LEGO DUPLO 10874 train base | Stays completely original |
| 1 | Small connector for battery/module wiring | Optional but helpful for serviceability |
| as needed | Thin hookup wire or AWG30 wire-wrap wire | For SNES `VDD`, `GND`, `CK`, `PL`, `DO`, power, and ground |
| as needed | Heat-shrink, Kapton tape, or insulation tape | Prevents shorts inside the controller |
| as needed | Hot glue or double-sided mounting tape | Used for the current quick internal mounting |

## Optional Upgrade Parts

| Part | Purpose | Notes |
| --- | --- | --- |
| Passive buzzer, 5020 or 9042 style | Button clicks and status tones | Use a transistor or MOSFET driver for low-impedance buzzers |
| NPN transistor or small logic-level MOSFET | Buzzer driver | Do not drive a 16 ohm buzzer directly from an ESP32 GPIO |
| Base/gate resistor | Buzzer driver support | Typical values depend on the chosen driver |
| Flyback diode | Protection for electromagnetic buzzers | Needed for magnetic buzzers, not for pure piezo elements |
| SK6812 MINI-E RGB LED | Optional status light | Previously planned, currently not installed |
| 330 ohm resistor | RGB LED data-line resistor | For ESP32 GPIO to SK6812 `DIN` |
| 100 uF / 10 V capacitor | RGB LED supply buffer | Optional supply smoothing near the LED |
| Simple status LED | Connection and activity feedback | Alternative to the RGB LED; keep it on a separate GPIO from the buzzer |
| 2 x 100k 1% resistors | Battery voltage measurement | Firmware expects measured ADC voltage multiplied by 2 |
| Solder bridge, jumper, or high-side switch | Battery divider disconnect | Prevents divider drain during deep sleep |
| Power switch | Hard power-off for storage | Useful for a child-proof handheld controller |
| Piggyback PCB | Cleaner and repeatable internal wiring | Recommended next hardware revision |
| Test pads | Easier debugging and flashing | Suggested pads: `3V3`, `5V`, `GND`, `CK`, `PL`, `DO`, `BOOT`, `EN/RST` |
| 3D printed or foam spacer | Mechanical support | Keeps the ESP32 and battery from moving inside the shell |

## Firmware Feature Flags For Optional Parts

| Feature | Config flag | Default |
| --- | --- | --- |
| Buzzer | `Config::kUseBuzzer` | `false` |
| Status LED | `Config::kUseStatusLed` | `false` |
| Battery monitor | `Config::kUseBatteryMonitor` | `false` |
| Deep sleep | `Config::kEnableDeepSleep` | `false` |
| START wakeup pin | `Config::kUseWakeupPin` | `false` |

## Notes From The Earlier Planning Phase

- The status RGB LED was planned first, then intentionally left out of the simple working build.
- The earlier plan also mentioned `L + R + Select` pairing combinations. The current firmware uses the simpler Start/Select behavior documented in `docs/CONTROLS.md`.
- Battery capacity and charger current should be checked against the exact cell and charger module used in the final build.

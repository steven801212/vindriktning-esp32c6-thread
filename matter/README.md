# Phase 2 — Matter over Thread (v0.2.0-dev2)

[繁體中文](README.zh-TW.md)

## Status

- `dev1` was flashed to the XIAO ESP32-C6. Apple Home successfully commissioned the device; a later serial log showed two stored Matter fabrics. This is not a validation of the new sensor wiring.
- `dev2` adds a **read-only I²C sensor diagnostic task**. Its actual build, hardware readings, and subsequent Matter connectivity still require local testing.
- Matter endpoint topology and **fixed** test values are unchanged: temperature 25 °C, humidity 50 %RH, PM2.5 10 µg/m³, Air Quality Good.
- No pressure endpoint exists yet. BMP280 pressure appears in serial logs only. Do not expect real data in Apple Home or Home Assistant at this stage.

## Sensor wiring (XIAO ESP32-C6)

| AHT20 + BMP280 module | XIAO |
| --- | --- |
| VDD | 3V3 (not 5 V) |
| GND | GND |
| SDA | D4 / GPIO22 |
| SCL | D5 / GPIO23 |

Both sensors share the bus. The probe expects AHT20 `0x38` and BMP280 `0x76` or `0x77`. The latter must identify itself as BMP280 (`0x58`), not BME280 (`0x60`). AHT20 CRC is checked; BMP280 uses its factory calibration coefficients for pressure compensation. Pressure is **absolute station pressure**, not sea-level-corrected pressure. If a sensor is absent on startup, reboot after correcting wiring.

## Safe local build and serial test

Use Ubuntu WSL, not Windows PowerShell, for the following commands. Attach the USB device from a separate Windows PowerShell (BUSID may change):

```powershell
usbipd list
usbipd attach --wsl --busid 1-2
```

Then in Ubuntu:

```bash
cd ~/vindriktning-esp32c6-thread
git status --short
git switch phase2-matter-thread
git pull --ff-only
source ~/esp/esp-idf/export.sh
source ~/esp/esp-matter/export.sh
cd matter
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

If `git status --short` reports local edits, preserve them and resolve before pulling. Exit a running serial monitor with `Ctrl+]` before flashing. **Do not use `erase-flash`, `fullclean` is not required, and do not change `partitions.csv` or use `set-target` for this update.** Ordinary `flash` should preserve existing NVS and Matter fabric data, provided the partition table is unchanged. Verify Apple Home and HA remain reachable afterward.

Expected diagnostic format, **illustrative rather than measured**:

```text
I (...) SENSOR_DIAG: I2C ACK: 0x38
I (...) SENSOR_DIAG: I2C ACK: 0x76
I (...) SENSOR_DIAG: I2C scan complete: AHT20=ACK BMP280=ACK
I (...) SENSOR_DIAG: AHT20: 26.42 C / 58.20 %RH
I (...) SENSOR_DIAG: BMP280: 1008.63 hPa (absolute station pressure)
```

If scanning finds neither device, check VDD/GND, the connector pin order, SDA/SCL, and I²C pull-ups. Upload the first `SENSOR_DIAG` lines, including any error, before integrating actual readings into Matter.

## Toolchain

ESP-Matter `release/v1.4.2`; ESP-IDF `v5.4.1`; target `esp32c6`; native Matter over Thread, BLE commissioning, no Wi-Fi. This project deliberately does not reuse Phase-1 PlatformIO / IDF 5.5.

## Security

Never commit household Thread datasets/keys, PSKc, DAC private keys, or generated production factory-data images.

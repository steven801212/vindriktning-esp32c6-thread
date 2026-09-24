# Phase 2 — Matter over Thread (v0.2.0-dev3)

[繁體中文](README.zh-TW.md)

## Status

- `dev1` was flashed to the XIAO ESP32-C6. Apple Home successfully commissioned the device; a later serial log showed two stored Matter fabrics. This is not a validation of the new sensor wiring.
- `dev3` turns the verified AHT20 acquisition into Matter reports: its real temperature and humidity update the existing endpoints 2 and 3. The report is scheduled on the CHIP system layer, so a controller with an active subscription receives standard Matter attribute reports.
- Temperature and humidity begin as **unknown** until the first CRC-checked AHT20 reading; after three consecutive failed reads (15 seconds) they are reported as unknown again rather than left stale. A later valid reading recovers them.
- A new standard Matter Pressure Sensor endpoint is created after the existing air-quality, temperature, and humidity endpoints. BMP280 pressure is rounded to whole hPa and is **absolute station pressure**, not sea-level corrected. It follows the same three-failure stale policy.
- PM2.5 remains the development 10 µg/m³ value and Air Quality remains Good until the PM1006 receive-only wiring is installed. The existing endpoint IDs 1/2/3 do not move.
- Build and field validation remain required; this repository does not claim that Apple Home, Home Assistant, Thread, or a physical device has been operated by this change.

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
I (...) SENSOR_DIAG: Matter AHT20 report: updated
I (...) SENSOR_DIAG: Matter BMP280 report: updated
```

If scanning finds neither device, check VDD/GND, the connector pin order, SDA/SCL, and I²C pull-ups. Upload the first `SENSOR_DIAG` lines, including any error, before integrating actual readings into Matter.

## Toolchain

ESP-Matter `release/v1.4.2`; ESP-IDF `v5.4.1`; target `esp32c6`; native Matter over Thread, BLE commissioning, no Wi-Fi. This project deliberately does not reuse Phase-1 PlatformIO / IDF 5.5.

## Apple Home to Home Assistant multi-admin troubleshooting

The two stored Fabric entries in the serial log only prove that two fabrics were once persisted. They do **not** prove that Home Assistant can reach the device, that a commissioning window is open, or that a Thread Border Router is routing correctly. Do not erase the device to test this path.

1. Keep the iPhone and HomePod mini updated, on the same Apple Home, and ensure the iPhone is signed in as the **home owner**. Apple Home supports Matter temperature and humidity sensors, but Apple does not list pressure among the Matter sensor types it exposes in Home. Treat pressure display in Apple Home as unsupported/unverified even though the firmware exposes the standard cluster.
2. In Apple Home, open this accessory's settings and use its Matter sharing / `Turn On Pairing Mode` action to generate the temporary setup code. This action asks the already-paired fabric to open an enhanced commissioning window; it is not the BLE advertisement shown at a fresh factory commission. If the action is absent, update iOS/HomePod software and verify owner status before changing firmware.
3. In the **Home Assistant Companion app** (not the browser), use **Settings → Connectivity → Matter → Add device**, choose **Yes, it’s already in use**, choose Apple Home, then follow the handoff prompts for the temporary code. Keep the app open while it hands the code to the Matter integration.
4. HAOS on Proxmox must use a bridged NIC on the same L2 LAN as the HomePod and iPhone (not ordinary NAT). Allow IPv6 ICMP/ND, multicast DNS UDP 5353, and Matter UDP 5540 across the bridge/firewall. Do not filter IPv6 multicast. Confirm HAOS has a global/ULA IPv6 address and default route; DNS-SD discovery needs both multicast and IPv6 routing.
5. During one pairing attempt collect, without sharing Thread credentials: `ha addons logs core_matter_server`, `ha core logs`, and the ESP serial log from `Matter commissioning session started` through completion/failure. A successful session needs a commissioning-window event and successful CASE traffic; `SRP update timed out`, repeated CASE Sigma1 retransmissions, and `unknown session` point to reachability/discovery, not an AHT20 problem.

Home Assistant may expose the standard pressure endpoint if its Matter Server/device mapping supports it. That is a HA field test, not a promise; use the HA entity/device page after commissioning as the evidence. Apple Home pressure display is explicitly not claimed.

## Security

Never commit household Thread datasets/keys, PSKc, DAC private keys, or generated production factory-data images.

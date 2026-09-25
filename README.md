# IKEA VINDRIKTNING × ESP32-C6 Thread Router

[繁體中文](README.zh-TW.md)

A staged IKEA VINDRIKTNING retrofit based on the **Seeed Studio XIAO ESP32-C6**. Phase 1 validated native IEEE 802.15.4 / OpenThread FTD and router capability on real hardware. Phase 2 now moves to **Matter over Thread** so Apple Home can commission the device normally and expose air-quality, PM2.5, temperature and humidity data.

> Stable baseline: `0.1.7-phase1-heartbeat` on `main`
>
> Current development: `0.2.0-dev3` on `phase2-matter-thread`

## Status

- [x] Native IEEE 802.15.4 / OpenThread bring-up
- [x] FTD + router-eligible operation
- [x] Standalone partition formation: `detached → leader`
- [x] Phase 1 hardware validation
- [x] Phase 2 Matter-over-Thread scaffold
- [x] PM1006 UART protocol / 5 V level-shifting design documented
- [x] Build/flash `v0.2.0-dev1` on XIAO ESP32-C6
- [x] iPhone BLE Matter commissioning
- [x] Join existing HomePod Thread mesh
- [x] Apple Home sensor visibility confirmed by user
- [x] Confirm live AHT20 and BMP280 readings on the wired XIAO ESP32-C6
- [x] Build and app-flash `dev3` without erasing NVS or the Apple Home fabric
- [x] Update Matter temperature/humidity endpoints 2/3 and pressure endpoint 4 from live sensors; serial log confirms a resumed controller subscription
- [ ] Complete Home Assistant Multi-Admin commissioning and verify its entities
- [ ] Replace the PM2.5 development value after PM1006 receive-only wiring

## Phase 2 architecture

The normal onboarding path is now:

```text
iPhone / Apple Home
      │ BLE Matter commissioning
      ▼
ESP32-C6 receives Thread credentials
      │
      ▼
HomePod mini Thread mesh
      │
      ▼
Matter sensor endpoints
```

Manual Thread Active Operational Dataset injection is retained only as a diagnostics/recovery path. Household Thread credentials must never be committed to this repository.

On the tested `dev3` hardware, AHT20 temperature and humidity update Matter endpoints 2/3; BMP280 absolute station pressure updates the standard Pressure Measurement cluster on endpoint 4. The serial log verifies attribute writes and a controller's successful subscription response, but Apple Home display and Home Assistant pairing still need field confirmation. Pressure display in Apple Home is not claimed. The following values remain development placeholders until PM1006 is connected:

- PM2.5: 10 µg/m³
- Air quality: Good

See [`matter/README.md`](matter/README.md) for safe build/flash steps, sensor failure behavior, and the Apple Home → Home Assistant sharing procedure.

## VINDRIKTNING PM2.5 integration

PM2.5 will be acquired by **passively listening to the PM1006-like sensor TX line**, leaving the IKEA MCU's original polling, fan and LED behavior intact.

Protocol currently targeted:

```text
9600 baud
20-byte frame
header = 16 11 0B
PM2.5 = (byte[5] << 8) | byte[6]
checksum = sum(byte[0..19]) mod 256 == 0
```

The ESP32-C6 implementation will use a hardware UART and a non-blocking 20-byte parser rather than the older ESP8266 SoftwareSerial/delay pattern.

> **Electrical warning:** published VINDRIKTNING measurements report approximately 5 V UART logic. The ESP32-C6 GPIO should not be connected directly to a 5 V TX signal. The initial design uses a `10 kΩ + 15 kΩ` divider to reduce a 5 V RX signal to about 3.0 V.

Full design notes: [`docs/pm1006.md`](docs/pm1006.md)

## Phase 1 verified result

The real XIAO ESP32-C6 reached:

```text
RouterTable---: Allocate router id 16
Mle-----------: RLOC16 fffe -> 4000
Mle-----------: Role detached -> leader
THREAD ROLE CHANGE => leader
THREAD HEARTBEAT: uptime=35s role=leader
PASS: ESP32-C6 formed a Thread network.
PASS: Native IEEE 802.15.4 radio is working.
PASS: FTD / Router / Leader capability works.
```

The Phase 1 PlatformIO project remains in the repository root as a stable hardware-validation baseline.

## Toolchains

### Phase 1

- PlatformIO `espressif32 @ 6.12.0`
- ESP-IDF 5.5.0 package
- XIAO ESP32-C6

### Phase 2

- ESP-Matter `release/v1.4.2`
- ESP-IDF `v5.4.1`
- Native ESP-IDF build flow
- Matter over Thread + BLE commissioning
- Wi-Fi disabled
- OpenThread CLI disabled

## Matter model

- Air Quality Sensor device type `0x002C`
- Air Quality cluster
- PM2.5 Concentration Measurement cluster
- Temperature Measurement
- Relative Humidity Measurement
- Pressure Measurement (absolute station pressure; Home Assistant display depends on integration support)

## Security

Never commit:

- household Thread Active Operational Dataset
- Thread Network Key or PSKc
- DAC private keys
- production factory-data images

## Disclaimer

Experimental DIY retrofit. Not affiliated with or endorsed by IKEA, Espressif, Seeed Studio, Apple, Connectivity Standards Alliance, or the Thread Group.

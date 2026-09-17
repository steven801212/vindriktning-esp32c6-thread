# IKEA VINDRIKTNING × ESP32-C6 Thread Router

[繁體中文](README.zh-TW.md)

A staged IKEA VINDRIKTNING retrofit based on the **Seeed Studio XIAO ESP32-C6**. Phase 1 validated native IEEE 802.15.4 / OpenThread FTD and router capability on real hardware. Phase 2 now moves to **Matter over Thread** so Apple Home can commission the device normally and expose air-quality, PM2.5, temperature and humidity data.

> Stable baseline: `0.1.7-phase1-heartbeat` on `main`
>
> Current development: `0.2.0-dev1` on `phase2-matter-thread`

## Status

- [x] Native IEEE 802.15.4 / OpenThread bring-up
- [x] FTD + router-eligible operation
- [x] Standalone partition formation: `detached → leader`
- [x] Phase 1 hardware validation
- [x] Phase 2 Matter-over-Thread scaffold
- [ ] Build/flash `v0.2.0-dev1` on XIAO ESP32-C6
- [ ] iPhone BLE Matter commissioning
- [ ] Join existing HomePod Thread mesh
- [ ] Apple Home sensor visibility
- [ ] Replace fixed values with VINDRIKTNING PM2.5 + SHTC3

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

The first Matter build deliberately uses fixed values:

- Temperature: 25.00 °C
- Relative humidity: 50.00 %
- PM2.5: 10 µg/m³
- Air quality: Good

See [`matter/README.md`](matter/README.md) for the Phase 2 build and Apple Home test procedure.

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

## Planned Matter model

- Air Quality Sensor device type `0x002C`
- Air Quality cluster
- PM2.5 Concentration Measurement cluster
- Temperature Measurement
- Relative Humidity Measurement

## Security

Never commit:

- household Thread Active Operational Dataset
- Thread Network Key or PSKc
- DAC private keys
- production factory-data images

## Disclaimer

Experimental DIY retrofit. Not affiliated with or endorsed by IKEA, Espressif, Seeed Studio, Apple, Connectivity Standards Alliance, or the Thread Group.

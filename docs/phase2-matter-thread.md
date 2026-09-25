# Phase 2: Matter over Thread

Status: **development / hardware validation pending**

## Decision

Normal product onboarding will use Matter commissioning rather than manually injecting a household Thread Active Operational Dataset.

The manual dataset-provisioning branch is retained only as an engineering/diagnostic fallback.

## v0.2.0-dev1 scope

- ESP32-C6 native Thread radio
- Full Thread Device capable hardware from Phase 1
- BLE Matter commissioning
- Air Quality Sensor endpoint
- PM2.5 concentration cluster (µg/m³)
- Temperature sensor endpoint
- Humidity sensor endpoint
- fixed fake measurements
- no Wi-Fi
- no OpenThread CLI
- no physical sensor drivers yet

## Test values

- Air Quality: Good
- PM2.5: 10 µg/m³
- Temperature: 25.00 °C
- Relative Humidity: 50.00 %

## Gate before Phase 3

Do not integrate VINDRIKTNING UART or SHTC3 until Apple Home commissioning and basic Matter attribute reads work reliably on real hardware.

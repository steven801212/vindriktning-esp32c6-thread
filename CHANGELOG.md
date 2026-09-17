# Changelog

## 0.1.7-phase1-heartbeat

- Validated on real XIAO ESP32-C6 hardware: `detached -> leader` at ~35 s.
- Router ID 16 allocated; RLOC16 became `0x4000`.
- Phase 1 native IEEE 802.15.4 / FTD / Router-Leader validation passed.
- Print current Thread role every 5 seconds.
- Phase-1 validation no longer depends on catching boot or role-transition logs.
- Preserve autonomous temporary-network test from v0.1.6.

## 0.1.6-phase1-autotest

- Bypass the ESP-IDF 5.x OpenThread USB CLI path entirely for Phase 1.
- Disable `esp_openthread_cli_create_task()` and interactive OpenThread CLI.
- Use `HOST_CONNECTION_MODE_NONE`.
- Automatically create a temporary Thread network using `esp_openthread_auto_start(NULL)`.
- Configure FTD / `rdn` / router eligibility before starting Thread.
- Add a role monitor which reports `disabled`, `detached`, `child`, `router`, or `leader`.
- Print an explicit PASS banner when the C6 becomes Thread Leader.

## 0.1.4-phase1

- Return to the ESP-IDF v5.5 initial-release API supported by PlatformIO `framework-espidf 3.50500.0`.
- Keep `esp_openthread_init()`, `esp_openthread_cli_init()`, `esp_openthread_cli_create_task()` and `esp_openthread_launch_mainloop()`.
- Set OpenThread host transport to `HOST_CONNECTION_MODE_NONE`.
- Add an early boot banner and initialization stage markers.

## 0.1.2-phase1

- Add automatic PlatformIO pre-build workaround for the ESP-IDF 5.5 OpenThread `OPENTHREAD_BUILD_DATETIME` quoting failure on Windows.
- Preserve a backup of the framework OpenThread CMakeLists before patching.
- No Thread protocol or radio behavior changes.

## 0.1.1-phase1

- Fix PlatformIO standalone build by bundling `src/esp_ot_config.h`.
- Explicitly declare required ESP-IDF components in `src/CMakeLists.txt`.

## 0.1.0-phase1

- Initial XIAO ESP32-C6 OpenThread bring-up.
- Native 802.15.4 radio.
- Full Thread Device.
- Router-eligible `rdn` mode.
- Initial USB Serial/JTAG CLI experiments.
- VINDRIKTNING and SHTC3 component placeholders.
- PlatformIO / Windows build helpers.

# IKEA VINDRIKTNING × ESP32-C6 Thread Router

[繁體中文](README.zh-TW.md)

A staged IKEA VINDRIKTNING retrofit based on the **Seeed Studio XIAO ESP32-C6**. The project first validates the ESP32-C6 as a native IEEE 802.15.4 **OpenThread Full Thread Device (FTD) / router-capable node**, then plans to add VINDRIKTNING PM2.5, SHTC3 temperature/humidity, Home Assistant integration, and Matter over Thread.

> Current firmware: `0.1.7-phase1-heartbeat`
>
> Phase 1 status: **validated successfully on real hardware**.

## Project goals

- Use the ESP32-C6 native IEEE 802.15.4 radio; no external Thread radio.
- Operate as a Thread router-capable mesh node, **not** as a Thread Border Router.
- Keep an existing HomePod mini / other Border Router responsible for Thread-to-LAN connectivity in the final deployment.
- Add IKEA VINDRIKTNING PM2.5 data and SHTC3 temperature/humidity later.
- Expose sensors through Matter over Thread / Home Assistant in later phases.
- Avoid Wi-Fi in the normal deployed firmware unless it is needed temporarily for provisioning.

## Phase 1 — Thread radio / FTD validation

The current firmware intentionally disables the interactive OpenThread USB CLI. During bring-up, ESP-IDF 5.x OpenThread USB CLI behavior on ESP32-C6 caused a tight-loop/watchdog condition in the `ot_cli` task. For Phase 1 we bypass that path completely and use the ESP-IDF OpenThread APIs directly.

At boot, firmware automatically:

1. initializes the native IEEE 802.15.4 radio,
2. initializes OpenThread,
3. configures `rdn` link mode,
4. enables router eligibility,
5. creates a temporary operational dataset,
6. starts Thread,
7. reports the Thread role every 5 seconds.

A single-node validation network should transition from `detached` to `leader`.

### Verified hardware result

The real XIAO ESP32-C6 test reached:

```text
RouterTable---: Allocate router id 16
Mle-----------: RLOC16 fffe -> 4000
Mle-----------: Role detached -> leader
Mle-----------: Partition ID 0x278acb93

THREAD ROLE CHANGE => leader
THREAD HEARTBEAT: uptime=35s role=leader
PASS: ESP32-C6 formed a Thread network.
PASS: Native IEEE 802.15.4 radio is working.
PASS: FTD / Router / Leader capability works.
Phase 1 autonomous Thread test SUCCESS.
```

The node remained `leader` for the rest of the captured test and continued transmitting MLE advertisements.

## Hardware

- IKEA VINDRIKTNING air quality sensor
- Seeed Studio XIAO ESP32-C6
- ESP32-C6 native IEEE 802.15.4 radio
- Planned: SHTC3 temperature/humidity sensor

## Toolchain

- VS Code + PlatformIO
- PlatformIO `espressif32 @ 6.12.0`
- ESP-IDF framework (`framework-espidf 5.5.0` in the validated environment)
- Board: `seeed_xiao_esp32c6`
- USB Serial/JTAG monitor: `115200`

Build:

```bash
pio run
```

Upload:

```bash
pio run -t upload --upload-port COM4
```

Monitor:

```bash
pio device monitor -p COM4 -b 115200
```

On Windows, the repository also includes:

```text
CLEAN_REBUILD_WINDOWS.bat
FLASH_MONITOR_WINDOWS.bat
```

Use `CLEAN_REBUILD_WINDOWS.bat` after changing ESP-IDF/OpenThread Kconfig options so an old generated `sdkconfig` does not mask the new defaults.

## Expected Phase 1 output

After flashing, the monitor should eventually show:

```text
THREAD HEARTBEAT: uptime=30s role=detached
...
THREAD ROLE CHANGE => leader
THREAD HEARTBEAT: uptime=35s role=leader
```

Once `role=leader` is observed, Phase 1 has validated:

- native IEEE 802.15.4 TX/RX,
- OpenThread stack initialization,
- Full Thread Device operation,
- router eligibility,
- Router/Leader capability.

## Important: this is not the final home Thread network

`0.1.7-phase1-heartbeat` creates a **temporary standalone Thread network for hardware validation**. It does not join the Apple/HomePod Thread mesh yet.

The next phase will provision the existing Active Operational Dataset into the ESP32-C6 and verify that it joins the existing mesh and becomes a router when the Thread topology elects it to that role.

### Security note

An Active Operational Dataset contains sensitive Thread credentials, including the Network Key. **Never commit a real household dataset, Network Key, PSKc, or provisioning dump to this repository.**

Future provisioning should write credentials locally into NVS and redact secrets from serial logs.

## PlatformIO / Windows OpenThread timestamp workaround

ESP-IDF 5.5 OpenThread builds may inject `OPENTHREAD_BUILD_DATETIME` through CMake. In the validated PlatformIO + Windows environment, quoting could break and produce errors such as:

```text
invalid digit "9" in octal constant
```

`scripts/patch_openthread_datetime.py` disables only that problematic compile definition before the build. It does not disable OpenThread or alter Thread protocol behavior.

## Roadmap

- [x] Phase 1: native IEEE 802.15.4 / OpenThread bring-up
- [x] Phase 1: FTD + router-eligible configuration
- [x] Phase 1: standalone partition formation and `leader` validation
- [ ] Phase 2: secure provisioning of the existing Apple/HomePod Active Dataset
- [ ] Phase 2: join the existing Thread mesh and verify router promotion/topology
- [ ] Phase 2: measure dead-zone / link-quality improvement
- [ ] Phase 3: IKEA VINDRIKTNING PM2.5 UART integration
- [ ] Phase 3: SHTC3 temperature/humidity integration
- [ ] Phase 4: Matter over Thread sensor endpoints
- [ ] Phase 4: Home Assistant integration

## Repository layout

```text
src/                         ESP-IDF application
scripts/                     PlatformIO/ESP-IDF build workaround
components/sensors/          planned sensor modules
docs/validation.md           Phase 1 validation notes
platformio.ini               PlatformIO configuration
sdkconfig.defaults           ESP-IDF/OpenThread defaults
partitions.csv               partition table
```

## Disclaimer

This is an experimental DIY retrofit project and is not affiliated with or endorsed by IKEA, Espressif, Seeed Studio, Apple, or the Thread Group.

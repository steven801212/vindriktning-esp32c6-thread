# Phase 1 Validation Record

Firmware: `0.1.7-phase1-heartbeat`  
Hardware: Seeed Studio XIAO ESP32-C6  
Framework: ESP-IDF 5.5.0 via PlatformIO `espressif32 @ 6.12.0`

## Purpose

Validate the ESP32-C6 native IEEE 802.15.4 radio and OpenThread Full Thread Device / router-capable behavior independently of Matter, Wi-Fi, and sensors.

## Test method

The firmware disables the interactive OpenThread USB CLI and starts a temporary standalone Thread network automatically using ESP-IDF OpenThread APIs. It configures:

- native IEEE 802.15.4 radio,
- FTD,
- `rdn` link mode,
- router eligibility,
- temporary Active Operational Dataset,
- Thread IPv6 interface and Thread protocol.

A heartbeat task prints the current role every five seconds.

## Observed result

At approximately 34.9 seconds after boot:

```text
RouterTable---: Allocate router id 16
Mle-----------: RLOC16 fffe -> 4000
Mle-----------: Role detached -> leader
Mle-----------: Partition ID 0x278acb93
RouterTable---: 16 0x4000 - me - leader
```

At approximately 35.4 seconds:

```text
THREAD ROLE CHANGE => leader
THREAD HEARTBEAT: uptime=35s role=leader
PASS: ESP32-C6 formed a Thread network.
PASS: Native IEEE 802.15.4 radio is working.
PASS: FTD / Router / Leader capability works.
Phase 1 autonomous Thread test SUCCESS.
```

The device remained `leader` through the rest of the captured log, at least through the 175-second heartbeat, and continued sending MLE advertisements.

The radio also received nearby unsecured IEEE 802.15.4/Thread multicast frames at approximately `-88 dBm`. Those frames do **not** prove membership in the household Apple/HomePod Thread network and are not used as Phase 1 success criteria.

## Conclusion

Phase 1 passed. The tested XIAO ESP32-C6 can:

- initialize the native IEEE 802.15.4 radio,
- run OpenThread as a Full Thread Device,
- operate in `rdn` mode,
- be router-eligible,
- allocate a Router ID,
- form a Thread partition,
- become Leader,
- transmit and receive IEEE 802.15.4 traffic.

## Known bring-up issue

During earlier experiments, enabling the ESP-IDF 5.x OpenThread USB CLI path caused `ot_cli` to enter a tight loop and starve the CPU0 idle task, triggering the task watchdog. Phase 1 therefore intentionally avoids the interactive OT CLI and uses API-driven autonomous validation.

## Next validation

Phase 2 will securely provision the household Active Operational Dataset, join the existing Apple/HomePod Thread mesh, and observe whether the ESP32-C6 remains a child or is promoted to Router according to the network topology.

Do not commit a real Active Dataset, Network Key, PSKc, or other Thread credentials to GitHub.

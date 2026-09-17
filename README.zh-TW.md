# IKEA VINDRIKTNING × ESP32-C6 Thread Router

[English](README.md)

這是一個使用 **Seeed Studio XIAO ESP32-C6** 改裝 IKEA VINDRIKTNING 的分階段專案。Phase 1 已在實機驗證原生 IEEE 802.15.4 / OpenThread FTD 與 Router 能力；Phase 2 現在改走 **Matter over Thread**，讓 Apple Home 用標準 Matter commissioning 加入裝置，並呈現空氣品質、PM2.5、溫度與濕度。

> 穩定基線：`0.1.7-phase1-heartbeat`（`main`）
>
> 目前開發版：`0.2.0-dev1`（`phase2-matter-thread`）

## 目前進度

- [x] 原生 IEEE 802.15.4 / OpenThread bring-up
- [x] FTD + Router Eligible
- [x] 單節點 Thread partition：`detached → leader`
- [x] Phase 1 實機驗證成功
- [x] Phase 2 Matter-over-Thread 專案骨架
- [ ] 在 XIAO ESP32-C6 build/flash `v0.2.0-dev1`
- [ ] iPhone BLE Matter commissioning
- [ ] 加入現有 HomePod Thread mesh
- [ ] Apple Home 顯示／讀取 sensor
- [ ] 把固定假資料換成 VINDRIKTNING PM2.5 + SHTC3

## Phase 2 正式架構

```text
iPhone / Apple Home
      │ BLE Matter commissioning
      ▼
ESP32-C6 自動取得 Thread credentials
      │
      ▼
HomePod mini Thread mesh
      │
      ▼
Matter sensor endpoints
```

因此正式 onboarding **不需要手動把家中的 Active Operational Dataset 寫進 firmware**。手動 dataset provisioning 只保留作工程診斷／救援用途，而且家庭 Thread credentials 永遠不能 commit 到 GitHub。

第一個 Matter 版本先固定輸出：

- 溫度：25.00 °C
- 相對濕度：50.00 %
- PM2.5：10 µg/m³
- Air Quality：Good

先把 Apple Home commissioning 整條鏈跑通，再接真正感測器。

詳細 build 與 Apple Home 測試流程請看 [`matter/README.zh-TW.md`](matter/README.zh-TW.md)。

## Phase 1 已驗證結果

實機 XIAO ESP32-C6 曾正常出現：

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

repo 根目錄的 PlatformIO 專案仍保留為 Phase 1 穩定硬體驗證基線。

## Toolchain

### Phase 1

- PlatformIO `espressif32 @ 6.12.0`
- ESP-IDF 5.5.0 package
- XIAO ESP32-C6

### Phase 2

- ESP-Matter `release/v1.4.2`
- ESP-IDF `v5.4.1`
- Native ESP-IDF build flow
- Matter over Thread + BLE commissioning
- Wi-Fi 關閉
- OpenThread CLI 關閉

## Matter data model

- Air Quality Sensor Device Type `0x002C`
- Air Quality cluster
- PM2.5 Concentration Measurement cluster
- Temperature Measurement
- Relative Humidity Measurement

## Security

不要 commit：

- 家庭 Thread Active Operational Dataset
- Thread Network Key / PSKc
- DAC private key
- production factory-data image

## Disclaimer

本專案為實驗性 DIY 改裝，與 IKEA、Espressif、Seeed Studio、Apple、Connectivity Standards Alliance 或 Thread Group 無隸屬或官方合作關係。

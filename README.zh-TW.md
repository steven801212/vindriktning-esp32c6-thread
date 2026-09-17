# IKEA VINDRIKTNING × ESP32-C6 Thread Router

[English](README.md)

這是一個以 **Seeed Studio XIAO ESP32-C6** 改裝 IKEA VINDRIKTNING 的分階段專案。第一階段先驗證 ESP32-C6 能以原生 IEEE 802.15.4 運作為 **OpenThread Full Thread Device（FTD）／具 Router 能力的節點**；後續再加入 VINDRIKTNING PM2.5、SHTC3 溫濕度、Home Assistant 與 Matter over Thread。

> 目前韌體：`0.1.7-phase1-heartbeat`
>
> Phase 1 狀態：**已在實機驗證成功**。

## 專案目標

- 直接使用 ESP32-C6 原生 IEEE 802.15.4，不使用外接 Thread radio。
- 最終定位為 Thread Router-capable mesh node，**不是 Thread Border Router**。
- 家中既有 HomePod mini／其他 Border Router 繼續負責 Thread 與 LAN 之間的連線。
- 後續整合 IKEA VINDRIKTNING PM2.5 與 SHTC3 溫濕度。
- 再透過 Matter over Thread / Home Assistant 暴露感測器資料。
- 正式部署時原則上不依賴 Wi-Fi；若 provisioning 階段需要，可暫時使用後再關閉。

## Phase 1 — Thread radio / FTD 驗證

目前版本刻意**關閉互動式 OpenThread USB CLI**。Bring-up 過程中，ESP32-C6 + ESP-IDF 5.x 的 OpenThread USB CLI 曾讓 `ot_cli` task 進入 tight loop 並觸發 watchdog，因此 Phase 1 直接繞過這條路徑，改用 ESP-IDF OpenThread API 自動測試。

開機後韌體會自動：

1. 初始化原生 IEEE 802.15.4 radio；
2. 初始化 OpenThread；
3. 設成 `rdn` link mode；
4. 啟用 router eligibility；
5. 建立臨時 Operational Dataset；
6. 啟動 Thread；
7. 每 5 秒回報目前 Thread role。

單節點測試網路正常會從 `detached` 轉成 `leader`。

### 實機驗證結果

XIAO ESP32-C6 實測成功出現：

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

之後整段測試持續維持 `leader`，並持續送出 MLE Advertisement。

因此 Phase 1 已驗證：

- ESP32-C6 native IEEE 802.15.4 TX/RX 正常；
- OpenThread stack 可正常啟動；
- Full Thread Device 正常；
- Router eligible 正常；
- 可以取得 Router ID、建立 partition 並成為 Leader。

## 硬體

- IKEA VINDRIKTNING 空氣品質感測器
- Seeed Studio XIAO ESP32-C6
- ESP32-C6 原生 IEEE 802.15.4 radio
- 預計加入：SHTC3 溫濕度感測器

## 開發環境

- VS Code + PlatformIO
- PlatformIO `espressif32 @ 6.12.0`
- ESP-IDF framework（實測環境為 `framework-espidf 5.5.0`）
- Board：`seeed_xiao_esp32c6`
- USB Serial/JTAG：`115200`

Build：

```bash
pio run
```

燒錄：

```bash
pio run -t upload --upload-port COM4
```

Monitor：

```bash
pio device monitor -p COM4 -b 115200
```

Windows 也提供：

```text
CLEAN_REBUILD_WINDOWS.bat
FLASH_MONITOR_WINDOWS.bat
```

如果更改過 ESP-IDF / OpenThread Kconfig，建議先執行 `CLEAN_REBUILD_WINDOWS.bat`，避免舊的 generated `sdkconfig` 蓋掉新設定。

## Phase 1 預期輸出

燒錄後不需要依賴 RESET 或 CLI。Serial Monitor 最後應看到：

```text
THREAD HEARTBEAT: uptime=30s role=detached
...
THREAD ROLE CHANGE => leader
THREAD HEARTBEAT: uptime=35s role=leader
```

看到 `role=leader` 即代表第一階段通過。

## 重要：目前不是家中的正式 Thread 網路

`0.1.7-phase1-heartbeat` 會建立一個**只供硬體驗證的臨時獨立 Thread network**。目前尚未加入 Apple / HomePod 的 Thread mesh。

下一階段會把家中既有 Thread network 的 Active Operational Dataset 以安全方式 provisioning 到 ESP32-C6，確認它能加入同一張 mesh，並在 Thread topology 需要時升成 Router。

### 安全注意事項

Active Operational Dataset 含有敏感的 Thread credential，包括 Network Key。**不要把家中的真實 Dataset、Network Key、PSKc 或完整 provisioning dump commit 到 GitHub。**

之後的設計會讓 credential 只在本機 provisioning，寫入 ESP32-C6 NVS；Serial log 也不輸出 secret。

## PlatformIO / Windows OpenThread timestamp workaround

ESP-IDF 5.5 OpenThread build 會透過 CMake 注入 `OPENTHREAD_BUILD_DATETIME`。在本專案實測的 PlatformIO + Windows 環境中，命令列 quoting 可能被錯誤處理，導致：

```text
invalid digit "9" in octal constant
```

`scripts/patch_openthread_datetime.py` 只會在 build 前停用這一個有問題的 compile definition，不會停用 OpenThread，也不會改變 Thread protocol 行為。

## Roadmap

- [x] Phase 1：native IEEE 802.15.4 / OpenThread bring-up
- [x] Phase 1：FTD + router-eligible
- [x] Phase 1：建立 standalone partition 並驗證 `leader`
- [ ] Phase 2：安全 provisioning Apple / HomePod Active Dataset
- [ ] Phase 2：加入家中既有 Thread mesh，觀察 child / router / topology
- [ ] Phase 2：實測 Thread 死角與 link quality 改善
- [ ] Phase 3：整合 IKEA VINDRIKTNING PM2.5 UART
- [ ] Phase 3：整合 SHTC3 溫濕度
- [ ] Phase 4：Matter over Thread sensor endpoints
- [ ] Phase 4：Home Assistant integration

## 專案結構

```text
src/                         ESP-IDF 主程式
scripts/                     PlatformIO/ESP-IDF build workaround
components/sensors/          預留感測器模組
docs/validation.md           Phase 1 驗證紀錄
platformio.ini               PlatformIO 設定
sdkconfig.defaults           ESP-IDF/OpenThread 預設設定
partitions.csv               partition table
```

## Disclaimer

這是實驗性 DIY 改裝專案，與 IKEA、Espressif、Seeed Studio、Apple 或 Thread Group 無官方關係或背書。

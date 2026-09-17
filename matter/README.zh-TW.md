# Phase 2 — Matter over Thread（v0.2.0-dev1）

[English](README.md)

這個子專案把正式加入家庭 Thread 網路的方式，從「手動灌 Active Operational Dataset」改成標準的 **Matter over Thread commissioning**。

## 這版提供的測試裝置

目前全部使用固定假資料：

- Matter Air Quality Sensor，Device Type `0x002C`
- Air Quality = Good
- PM2.5 = 10 µg/m³
- Temperature = 25.00 °C
- Relative Humidity = 50.00 %

`dev1` 的目的不是測真正的 VINDRIKTNING 感測器，而是先確認整條鏈：

```text
iPhone → BLE Matter commissioning → Thread credentials
       → HomePod mini Thread mesh → Apple Home Matter fabric
       → sensor endpoints
```

成功之後才把假資料替換成 VINDRIKTNING PM2.5 + SHTC3。

## 開發環境

Phase 2 不直接沿用 Phase 1 的 PlatformIO + ESP-IDF 5.5。

本 branch 固定：

- ESP-Matter：`release/v1.4.2`
- ESP-IDF：`v5.4.1`
- Target：`esp32c6`
- Matter transport：Thread
- Commissioning：BLE
- Wi-Fi：關閉
- OpenThread CLI：關閉

ESP-Matter 1.4.2 官方就是以 ESP-IDF 5.4.1 為建議版本，因此先使用這個組合降低版本相容性問題。

## Build / Flash

準備好 ESP-IDF v5.4.1 與 ESP-Matter `release/v1.4.2` 的環境後，進入本 repo 的 `matter` 目錄：

```powershell
idf.py -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.defaults.esp32c6" set-target esp32c6
idf.py build
idf.py -p COM4 flash monitor
```

第一次測 Matter 建議把 Phase 1 與舊實驗留下的 NVS 清掉一次：

```powershell
idf.py -p COM4 erase-flash
```

再重新 flash。

## Apple Home 測試流程

1. HomePod mini 保持在線，作為 Thread Border Router。
2. 將 `v0.2.0-dev1` 燒進 XIAO ESP32-C6。
3. iPhone 打開「家庭」→ 加入配件。
4. 使用 ESP-Matter development commissioning payload / QR code 加入。
5. 若未被 factory data 覆寫，ESP-Matter 常用 development setup passcode 為 `20202021`、discriminator `3840`。
6. Apple Home 會透過 Matter commissioning 把 Thread credentials 傳給 ESP32-C6；不需要把家裡的 Thread Network Key 寫進 source code。
7. Serial 若出現：

```text
Matter commissioning COMPLETE
```

代表 Matter fabric commissioning 已完成。

## 成功標準

- iPhone 能透過 BLE 發現裝置。
- Matter commissioning 能自動配置 Thread network。
- ESP32-C6 加入 HomePod 所在的 Thread mesh。
- Apple Home 完成 Matter commissioning。
- 能讀到固定的 25°C、50% RH、Air Quality Good；PM2.5 cluster 可被 Matter controller 讀取。
- 全程不需要手動複製 Active Operational Dataset。

> Apple Home 的 UI 不保證一定把所有 optional concentration cluster 的數字都直接顯示出來。因此 PM2.5 若 UI 沒出現，之後會再用 Home Assistant / chip-tool 驗證 Matter cluster 本身。

## Security

不要把以下內容 commit 到 GitHub：

- 家庭 Thread Active Operational Dataset
- Thread Network Key / PSKc
- DAC private key
- production factory-data image

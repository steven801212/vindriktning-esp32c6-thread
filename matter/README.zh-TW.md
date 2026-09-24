# Phase 2 — Matter over Thread（v0.2.0-dev2）

[English](README.md)

## 目前狀態

- `dev1` 已在 XIAO ESP32-C6 實機燒錄並成功加入 Apple Home；後續序列埠 log 顯示兩個 Matter Fabric。這不等同新感測器已通過驗證。
- `dev2` 新增**唯讀 I²C 感測器診斷任務**；尚需使用者在 WSL 編譯、燒錄及實測數值。
- Matter 端點和固定測試數值**不變**：25°C、50%RH、PM2.5 10 µg/m³、Air Quality Good。
- 尚未新增氣壓 Matter 端點；BMP280 氣壓只輸出到序列埠，Apple Home／HA 此階段不會看到真實氣壓或真實溫濕度。

## 接線：XIAO ESP32-C6

| AHT20 + BMP280 模組 | XIAO |
| --- | --- |
| VDD | 3V3（不要接 5V） |
| GND | GND |
| SDA | D4 / GPIO22 |
| SCL | D5 / GPIO23 |

兩顆感測器共用 I²C；預期掃描到 AHT20 `0x38`、BMP280 `0x76` 或 `0x77`。程式會檢查 BMP280 晶片識別 `0x58`（BME280 的 `0x60` 不會冒充通過），驗證 AHT20 CRC，並使用 BMP280 原廠校正係數計算氣壓。氣壓是**所在地絕對氣壓**，不是海平面校正氣壓。若開機時未偵測到感測器，修好接線後請重啟。

## 安全更新與測試

在另一個 **Windows PowerShell** 中掛載 USB，先用 `usbipd list` 查實際 BUSID：

```powershell
usbipd list
usbipd attach --wsl --busid 1-2
```

然後在 **Ubuntu WSL** 執行：

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

若 `git status --short` 有本地修改，請先保留並處理，勿直接覆蓋。刷寫前先以 `Ctrl+]` 離開既有 monitor。**不要執行 `erase-flash`、不要變更 `partitions.csv` 或重新 `set-target`。** 一般 `flash` 在分割表不變時應保留原有 NVS／Matter 配對資料。刷寫後也確認 Apple Home／HA 連線正常。

預期格式（**以下數值是範例，不是你的實測**）：

```text
I (...) SENSOR_DIAG: I2C ACK: 0x38
I (...) SENSOR_DIAG: I2C ACK: 0x76
I (...) SENSOR_DIAG: I2C scan complete: AHT20=ACK BMP280=ACK
I (...) SENSOR_DIAG: AHT20: 26.42 C / 58.20 %RH
I (...) SENSOR_DIAG: BMP280: 1008.63 hPa (absolute station pressure)
```

若找不到裝置，先確認 3V3、GND、接頭腳位順序、SDA/SCL 與上拉電阻。請回傳含錯誤的第一段 `SENSOR_DIAG` log，再接下一階段的 Matter 真實數值更新。

## 工具鏈

ESP-Matter `release/v1.4.2`、ESP-IDF `v5.4.1`、`esp32c6`、原生 Matter over Thread／BLE commissioning、關閉 Wi-Fi。Phase 1 的 PlatformIO／IDF 5.5 不直接沿用。

## 資安

勿提交家庭 Thread dataset／金鑰、PSKc、DAC private key 或 production factory-data image。

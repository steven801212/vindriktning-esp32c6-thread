# Phase 2 — Matter over Thread（v0.2.0-dev3）

[English](README.md)

## 目前狀態

- `dev1` 已在 XIAO ESP32-C6 實機燒錄並成功加入 Apple Home；後續序列埠 log 顯示兩個 Matter Fabric。這不等同新感測器已通過驗證。
- `dev3` 把已驗證的 AHT20 讀值送至既有 Matter 端點 2（溫度）和 3（濕度）。更新會排程到 CHIP system layer，既有 Matter 訂閱會收到標準屬性報告。
- 溫濕度在第一次通過 CRC 的 AHT20 讀值前是**未知值**；連續三次讀取失敗（目前週期為 5 秒，即 15 秒）後也會重新報告未知，避免控制器持續顯示舊數值。讀取恢復後會重新更新。
- 新增一個排在既有空品、溫度與濕度端點之後的標準 Matter Pressure Sensor 端點。BMP280 以整數 hPa 回報**所在地絕對氣壓**，不做海平面校正，且同樣採三次失敗後標示未知的策略。
- PM2.5 仍維持開發用 10 µg/m³、Air Quality 仍為 Good，直到接上 PM1006 的被動 RX。既有端點 1/2/3 不會換號。
- 此次修改尚未操作實機、Apple Home、HA 或 Thread；仍須編譯與現場驗證，不宣稱已完成配對。

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
idf.py -B build-dev3 -D SDKCONFIG=sdkconfig.dev3 build
grep '^CONFIG_ESP_MATTER_MAX_DYNAMIC_ENDPOINT_COUNT=' sdkconfig.dev3
# 預期是 5；先從序列埠開機紀錄確認目前從 OTA 0 啟動。
idf.py -B build-dev3 -D SDKCONFIG=sdkconfig.dev3 -p /dev/ttyACM0 app-flash monitor
```

若 `git status --short` 有本地修改，請先保留並處理，勿直接覆蓋。刷寫前先以 `Ctrl+]` 離開既有 monitor。獨立的 `sdkconfig.dev3` 很重要：既有 dev2 `sdkconfig` 仍可能保留舊端點上限 4，不會因為 defaults 已改為 5 而自動更新。**執行上述 app-only 刷寫前，先從目前序列埠開機紀錄確認裝置由 OTA 0 啟動。**先前 dev2 log 是 OTA 0，但仍需現在再確認。`app-flash` 只寫入 `0x20000` 的應用程式，不改 bootloader、分割表、OTA 選擇及 NVS。不要執行 `erase-flash` 或 `set-target`。若目前從 OTA 1 啟動，先停止並選正確更新方式，不要盲刷 OTA 0。重啟後再確認 Apple Home／HA。

預期格式（**以下數值是範例，不是你的實測**）：

```text
I (...) SENSOR_DIAG: I2C ACK: 0x38
I (...) SENSOR_DIAG: I2C ACK: 0x76
I (...) SENSOR_DIAG: I2C scan complete: AHT20=ACK BMP280=ACK
I (...) SENSOR_DIAG: AHT20: 26.42 C / 58.20 %RH
I (...) SENSOR_DIAG: BMP280: 1008.63 hPa (absolute station pressure)
I (...) SENSOR_DIAG: Matter AHT20 report: updated
I (...) SENSOR_DIAG: Matter BMP280 report: updated
```

若找不到裝置，先確認 3V3、GND、接頭腳位順序、SDA/SCL 與上拉電阻。請回傳含錯誤的第一段 `SENSOR_DIAG` log，再接下一階段的 Matter 真實數值更新。

## 工具鏈

ESP-Matter `release/v1.4.2`、ESP-IDF `v5.4.1`、`esp32c6`、原生 Matter over Thread／BLE commissioning、關閉 Wi-Fi。Phase 1 的 PlatformIO／IDF 5.5 不直接沿用。

## Apple Home 分享到 Home Assistant（Multi-Admin）排查

序列埠裡的兩個 Fabric 只表示曾有兩個 fabric 被保存；**不**代表 HA 已經能存取裝置、commissioning window 已開啟，或 Thread Border Router 的路由正常。測試此流程不要 erase。

1. iPhone 與 HomePod mini 都先更新、確認在同一個 Apple Home，iPhone 使用的是此家庭的**擁有者**帳號。Apple 列出的 Home 對 Matter 感測器支援包含溫度與濕度，沒有列氣壓；即使本韌體有標準 Pressure Measurement cluster，也不可宣稱 Apple Home 會顯示氣壓。
2. 在 Apple Home 開啟這個配件的設定，使用 Matter 分享／`Turn On Pairing Mode` 產生暫時配對碼。這是由已配對 fabric 要求開啟 enhanced commissioning window，不是未配對時的 BLE 廣播。若沒有這個選項，先更新 iOS/HomePod 並確認擁有者權限，先不要改韌體或清 NVS。
3. 在 **Home Assistant Companion 手機 App**（不是瀏覽器）依序選 **設定 → Connectivity → Matter → Add device**，選 **Yes, it’s already in use**，再選 Apple Home，並依畫面交接暫時碼；交接時保持手機 App 開啟。
4. PVE 上的 HAOS NIC 必須 bridge 到 HomePod 所在的 IoT VLAN，不能是一般 NAT；iPhone 可在另一個可路由的 LAN，但裝置探索與控制器交接必須正常。PVE bridge/防火牆要允許 IPv6 ICMP/ND、mDNS UDP 5353 multicast 與 Matter UDP 5540。除了 IPv6 位址，更要確認 HAOS **對 Matter Server 發現的 Thread 裝置 IPv6 位址有可用路由**（在 HAOS shell 執行 `ip -6 route get <裝置 IPv6>`）。另一張網卡有 IPv6 預設路由，不等於 Thread 前綴可達；尚未確認路由前不要猜測 gateway 或關閉隔離。
5. 只做一次配對測試時，收集但不要貼出 Thread 金鑰：`ha addons logs core_matter_server`、`ha core logs`，以及 ESP 從 `Matter commissioning session started` 到成功/失敗的 serial log。成功必須看到 commissioning-window 事件與可建立 CASE；`SRP update timed out`、反覆 CASE Sigma1 重傳、`unknown session` 值得檢查路由與探索，但單靠這些訊息無法確定哪段網路故障，更不能指向 AHT20。

如果 HA Matter Server 對該標準 endpoint 有對應，HA 有可能出現氣壓實體；這需要以實際 HA 裝置頁為準，不是保證。Apple Home 氣壓顯示則明確不保證。

## 資安

勿提交家庭 Thread dataset／金鑰、PSKc、DAC private key 或 production factory-data image。

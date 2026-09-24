// Read-only hardware diagnostics. Does not modify Matter attributes or NVS.
#include "sensor_diag.h"

#include <cmath>
#include <cstdint>
#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace {
constexpr char TAG[] = "SENSOR_DIAG";
constexpr gpio_num_t SDA = GPIO_NUM_22;  // XIAO D4
constexpr gpio_num_t SCL = GPIO_NUM_23;  // XIAO D5
constexpr uint8_t AHT20_ADDR = 0x38;
constexpr uint8_t BMP280_ADDR_A = 0x76;
constexpr uint8_t BMP280_ADDR_B = 0x77;
constexpr int TIMEOUT_MS = 100;
constexpr TickType_t PERIOD = pdMS_TO_TICKS(5000);

i2c_master_bus_handle_t bus = nullptr;
i2c_master_dev_handle_t aht = nullptr;
i2c_master_dev_handle_t bmp = nullptr;

struct BmpCalibration {
    uint16_t t1, p1;
    int16_t t2, t3, p2, p3, p4, p5, p6, p7, p8, p9;
} cal{};

uint16_t u16le(const uint8_t *p)
{
    return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

int16_t s16le(const uint8_t *p)
{
    return static_cast<int16_t>(u16le(p));
}

esp_err_t read_regs(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t *out, size_t n)
{
    return i2c_master_transmit_receive(dev, &reg, 1, out, n, TIMEOUT_MS);
}

esp_err_t write_regs(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t value)
{
    const uint8_t cmd[] = {reg, value};
    return i2c_master_transmit(dev, cmd, sizeof(cmd), TIMEOUT_MS);
}

uint8_t crc8(const uint8_t *bytes, size_t n)
{
    uint8_t crc = 0xff;
    for (size_t i = 0; i < n; ++i) {
        crc ^= bytes[i];
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x31)
                               : static_cast<uint8_t>(crc << 1);
        }
    }
    return crc;
}

esp_err_t add_device(uint8_t address, i2c_master_dev_handle_t *out)
{
    i2c_device_config_t cfg = {};
    cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    cfg.device_address = address;
    cfg.scl_speed_hz = 100000;
    return i2c_master_bus_add_device(bus, &cfg, out);
}

esp_err_t init_aht()
{
    uint8_t status = 0;
    esp_err_t err = i2c_master_receive(aht, &status, 1, TIMEOUT_MS);
    if (err != ESP_OK) return err;
    if ((status & 0x08) == 0) {
        const uint8_t init[] = {0xbe, 0x08, 0x00};
        err = i2c_master_transmit(aht, init, sizeof(init), TIMEOUT_MS);
        if (err != ESP_OK) return err;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    return ESP_OK;
}

esp_err_t read_aht(float *temperature_c, float *humidity_pct)
{
    const uint8_t trigger[] = {0xac, 0x33, 0x00};
    esp_err_t err = i2c_master_transmit(aht, trigger, sizeof(trigger), TIMEOUT_MS);
    if (err != ESP_OK) return err;
    vTaskDelay(pdMS_TO_TICKS(85));

    uint8_t data[7] = {};
    for (int attempt = 0; attempt < 4; ++attempt) {
        err = i2c_master_receive(aht, data, sizeof(data), TIMEOUT_MS);
        if (err != ESP_OK) return err;
        if ((data[0] & 0x80) == 0) break;
        if (attempt == 3) return ESP_ERR_TIMEOUT;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    if ((data[0] & 0x08) == 0 || crc8(data, 6) != data[6]) return ESP_ERR_INVALID_CRC;

    const uint32_t raw_h = (static_cast<uint32_t>(data[1]) << 12) |
                           (static_cast<uint32_t>(data[2]) << 4) | (data[3] >> 4);
    const uint32_t raw_t = (static_cast<uint32_t>(data[3] & 0x0f) << 16) |
                           (static_cast<uint32_t>(data[4]) << 8) | data[5];
    *humidity_pct = 100.0f * static_cast<float>(raw_h) / 1048576.0f;
    *temperature_c = 200.0f * static_cast<float>(raw_t) / 1048576.0f - 50.0f;
    if (!std::isfinite(*temperature_c) || !std::isfinite(*humidity_pct) ||
        *humidity_pct < 0 || *humidity_pct > 100 ||
        *temperature_c < -40 || *temperature_c > 85) return ESP_ERR_INVALID_RESPONSE;
    return ESP_OK;
}

esp_err_t init_bmp(uint8_t address)
{
    esp_err_t err = add_device(address, &bmp);
    if (err != ESP_OK) return err;
    uint8_t chip_id = 0;
    err = read_regs(bmp, 0xd0, &chip_id, 1);
    if (err != ESP_OK) return err;
    if (chip_id != 0x58) {
        ESP_LOGE(TAG, "0x%02x has chip ID 0x%02x, expected BMP280 0x58", address, chip_id);
        return ESP_ERR_NOT_SUPPORTED;
    }

    uint8_t raw[24] = {};
    err = read_regs(bmp, 0x88, raw, sizeof(raw));
    if (err != ESP_OK) return err;
    cal.t1 = u16le(raw);       cal.t2 = s16le(raw + 2);  cal.t3 = s16le(raw + 4);
    cal.p1 = u16le(raw + 6);   cal.p2 = s16le(raw + 8);  cal.p3 = s16le(raw + 10);
    cal.p4 = s16le(raw + 12); cal.p5 = s16le(raw + 14); cal.p6 = s16le(raw + 16);
    cal.p7 = s16le(raw + 18); cal.p8 = s16le(raw + 20); cal.p9 = s16le(raw + 22);
    if (cal.p1 == 0 || cal.p1 == 0xffff || cal.t1 == 0 || cal.t1 == 0xffff) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    // Normal mode, x1 temperature and pressure oversampling.
    return write_regs(bmp, 0xf4, 0x27);
}

esp_err_t read_bmp(double *pressure_hpa)
{
    uint8_t raw[6] = {};
    esp_err_t err = read_regs(bmp, 0xf7, raw, sizeof(raw));
    if (err != ESP_OK) return err;
    const int32_t adc_p = (static_cast<int32_t>(raw[0]) << 12) |
                          (static_cast<int32_t>(raw[1]) << 4) | (raw[2] >> 4);
    const int32_t adc_t = (static_cast<int32_t>(raw[3]) << 12) |
                          (static_cast<int32_t>(raw[4]) << 4) | (raw[5] >> 4);
    if (adc_p == 0x80000 || adc_t == 0x80000) return ESP_ERR_INVALID_RESPONSE;

    // Bosch BMP280 datasheet floating-point compensation. Temperature is used
    // only to obtain t_fine; the AHT20 remains the room-temperature source.
    const double t1 = (static_cast<double>(adc_t) / 16384.0 - cal.t1 / 1024.0) * cal.t2;
    const double t2_base = static_cast<double>(adc_t) / 131072.0 - cal.t1 / 8192.0;
    const double t_fine = t1 + t2_base * t2_base * cal.t3;
    double v1 = t_fine / 2.0 - 64000.0;
    double v2 = v1 * v1 * cal.p6 / 32768.0;
    v2 = v2 + v1 * cal.p5 * 2.0;
    v2 = v2 / 4.0 + cal.p4 * 65536.0;
    v1 = (cal.p3 * v1 * v1 / 524288.0 + cal.p2 * v1) / 524288.0;
    v1 = (1.0 + v1 / 32768.0) * cal.p1;
    if (std::fabs(v1) < 1e-9) return ESP_ERR_INVALID_RESPONSE;
    double p = 1048576.0 - adc_p;
    p = (p - v2 / 4096.0) * 6250.0 / v1;
    v1 = cal.p9 * p * p / 2147483648.0;
    v2 = p * cal.p8 / 32768.0;
    p += (v1 + v2 + cal.p7) / 16.0;  // Pa
    *pressure_hpa = p / 100.0;
    if (!std::isfinite(*pressure_hpa) || *pressure_hpa < 300 || *pressure_hpa > 1100) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    return ESP_OK;
}

void sensor_task(void *)
{
    i2c_master_bus_config_t cfg = {};
    cfg.i2c_port = I2C_NUM_0;
    cfg.sda_io_num = SDA;
    cfg.scl_io_num = SCL;
    cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    cfg.glitch_ignore_cnt = 7;
    cfg.flags.enable_internal_pullup = true;

    esp_err_t err = i2c_new_master_bus(&cfg, &bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C init failed: %s", esp_err_to_name(err));
        vTaskDelete(nullptr);
        return;
    }
    ESP_LOGI(TAG, "I2C bus SDA=GPIO22 D4, SCL=GPIO23 D5, 100 kHz, 3.3 V");
    bool have_aht = false;
    uint8_t bmp_address = 0;
    bool have_bmp = false;
    for (uint8_t address = 0x08; address <= 0x77; ++address) {
        err = i2c_master_probe(bus, address, TIMEOUT_MS);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "I2C ACK: 0x%02X", address);
            if (address == AHT20_ADDR) have_aht = true;
            if (address == BMP280_ADDR_A || address == BMP280_ADDR_B) {
                if (!bmp_address) bmp_address = address;
            }
        } else if (err != ESP_ERR_NOT_FOUND) {
            ESP_LOGW(TAG, "I2C probe 0x%02X: %s (check wires/pull-ups)",
                     address, esp_err_to_name(err));
        }
    }
    ESP_LOGI(TAG, "I2C scan complete: AHT20=%s BMP280=%s",
             have_aht ? "ACK" : "NOT FOUND", bmp_address ? "ACK" : "NOT FOUND");

    if (have_aht) {
        err = add_device(AHT20_ADDR, &aht);
        if (err == ESP_OK) err = init_aht();
        if (err != ESP_OK) ESP_LOGE(TAG, "AHT20 init: %s", esp_err_to_name(err));
        have_aht = (err == ESP_OK);
    }
    if (bmp_address) {
        err = init_bmp(bmp_address);
        if (err != ESP_OK) ESP_LOGE(TAG, "BMP280 init: %s", esp_err_to_name(err));
        have_bmp = (err == ESP_OK);
    }
    if (!have_aht && !have_bmp) {
        ESP_LOGE(TAG, "No readable sensors. Confirm VDD=3V3, GND, D4=SDA, D5=SCL.");
    }

    while (true) {
        if (have_aht) {
            float temp = 0, humidity = 0;
            err = read_aht(&temp, &humidity);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "AHT20: %.2f C / %.2f %%RH", temp, humidity);
            } else {
                ESP_LOGW(TAG, "AHT20 read failed: %s", esp_err_to_name(err));
            }
        }
        if (have_bmp) {
            double pressure = 0;
            err = read_bmp(&pressure);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "BMP280: %.2f hPa (absolute station pressure)", pressure);
            } else {
                ESP_LOGW(TAG, "BMP280 read failed: %s", esp_err_to_name(err));
            }
        }
        vTaskDelay(PERIOD);
    }
}
} // namespace

void start_sensor_diagnostics()
{
    // Do not block Matter/Thread startup. Diagnostics run at a lower-priority task.
    if (xTaskCreate(sensor_task, "sensor_diag", 4096, nullptr, 4, nullptr) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create sensor diagnostics task");
    }
}

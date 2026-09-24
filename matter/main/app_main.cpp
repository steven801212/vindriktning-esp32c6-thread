#include <esp_err.h>
#include <esp_log.h>
#include <esp_matter.h>
#include <nvs_flash.h>
#include "sensor_diag.h"

#include <app_openthread_config.h>
#include <common_macros.h>
#include <platform/ESP32/OpenthreadLauncher.h>

static const char *TAG = "VINDRIKTNING_MATTER";

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

static esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id,
                                       uint8_t effect_id, uint8_t effect_variant, void *priv_data)
{
    ESP_LOGI(TAG, "Identify: endpoint=%u type=%u effect=%u variant=%u",
             endpoint_id, type, effect_id, effect_variant);
    return ESP_OK;
}

static esp_err_t app_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id,
                                         uint32_t cluster_id, uint32_t attribute_id,
                                         esp_matter_attr_val_t *val, void *priv_data)
{
    return ESP_OK;
}

static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg)
{
    switch (event->Type) {
    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStarted:
        ESP_LOGI(TAG, "Matter commissioning session started");
        break;
    case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
        ESP_LOGI(TAG, "Matter commissioning COMPLETE");
        break;
    case chip::DeviceLayer::DeviceEventType::kFailSafeTimerExpired:
        ESP_LOGW(TAG, "Matter commissioning failed: fail-safe timer expired");
        break;
    case chip::DeviceLayer::DeviceEventType::kInterfaceIpAddressChanged:
        ESP_LOGI(TAG, "Interface IP address changed");
        break;
    case chip::DeviceLayer::DeviceEventType::kBLEDeinitialized:
        ESP_LOGI(TAG, "BLE commissioning transport deinitialized");
        break;
    default:
        break;
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "VINDRIKTNING Matter-over-Thread v0.2.0-dev2");
    ESP_LOGI(TAG, "Fake sensor data: 25.00 C / 50.00 %%RH / PM2.5 10 ug/m3 / Air Quality Good");

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    node::config_t node_config;
    node_t *node = node::create(&node_config, app_attribute_update_cb, app_identification_cb);
    ABORT_APP_ON_FAILURE(node != nullptr, ESP_LOGE(TAG, "Failed to create Matter node"));

    air_quality_sensor::config_t aq_config;
    aq_config.air_quality.air_quality = static_cast<uint8_t>(AirQuality::AirQualityEnum::kGood);
    endpoint_t *aq_ep = air_quality_sensor::create(node, &aq_config, ENDPOINT_FLAG_NONE, nullptr);
    ABORT_APP_ON_FAILURE(aq_ep != nullptr, ESP_LOGE(TAG, "Failed to create Air Quality endpoint"));

    cluster::pm25_concentration_measurement::config_t pm25_config;
    pm25_config.measurement_medium = static_cast<uint8_t>(chip::app::Clusters::detail::MeasurementMediumEnum::kAir);
    pm25_config.feature_flags = cluster::concentration_measurement::feature::numeric_measurement::get_id();
    pm25_config.features.numeric_measurement.measured_value = nullable<float>(10.0f);
    pm25_config.features.numeric_measurement.min_measured_value = nullable<float>(0.0f);
    pm25_config.features.numeric_measurement.max_measured_value = nullable<float>(1000.0f);
    pm25_config.features.numeric_measurement.measurement_unit =
        static_cast<uint8_t>(chip::app::Clusters::detail::MeasurementUnitEnum::kUgm3);
    cluster_t *pm25_cluster = cluster::pm25_concentration_measurement::create(
        aq_ep, &pm25_config, CLUSTER_FLAG_SERVER);
    ABORT_APP_ON_FAILURE(pm25_cluster != nullptr, ESP_LOGE(TAG, "Failed to add PM2.5 cluster"));

    temperature_sensor::config_t temp_config;
    temp_config.temperature_measurement.measured_value = nullable<int16_t>(2500);
    temp_config.temperature_measurement.min_measured_value = nullable<int16_t>(-4000);
    temp_config.temperature_measurement.max_measured_value = nullable<int16_t>(12500);
    endpoint_t *temp_ep = temperature_sensor::create(node, &temp_config, ENDPOINT_FLAG_NONE, nullptr);
    ABORT_APP_ON_FAILURE(temp_ep != nullptr, ESP_LOGE(TAG, "Failed to create Temperature endpoint"));

    humidity_sensor::config_t humidity_config;
    humidity_config.relative_humidity_measurement.measured_value = nullable<uint16_t>(5000);
    humidity_config.relative_humidity_measurement.min_measured_value = nullable<uint16_t>(0);
    humidity_config.relative_humidity_measurement.max_measured_value = nullable<uint16_t>(10000);
    endpoint_t *humidity_ep = humidity_sensor::create(node, &humidity_config, ENDPOINT_FLAG_NONE, nullptr);
    ABORT_APP_ON_FAILURE(humidity_ep != nullptr, ESP_LOGE(TAG, "Failed to create Humidity endpoint"));

    ESP_LOGI(TAG, "Endpoints: air-quality=%u temperature=%u humidity=%u",
             endpoint::get_id(aq_ep), endpoint::get_id(temp_ep), endpoint::get_id(humidity_ep));

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
    esp_openthread_platform_config_t ot_config = {
        .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
        .port_config = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
    };
    set_openthread_platform_config(&ot_config);
#endif

    err = esp_matter::start(app_event_cb);
    ABORT_APP_ON_FAILURE(err == ESP_OK, ESP_LOGE(TAG, "Failed to start Matter: %d", err));

    ESP_LOGI(TAG, "Matter started. Device is ready for BLE commissioning into a Thread network.");
    ESP_LOGI(TAG, "Development setup passcode is normally 20202021 / discriminator 3840 unless factory data overrides it.");
    start_sensor_diagnostics();
}

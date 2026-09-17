#include <assert.h>
#include <stdio.h>

#include "sdkconfig.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_types.h"
#include "esp_openthread.h"
#include "esp_openthread_lock.h"
#include "esp_openthread_netif_glue.h"
#include "esp_openthread_types.h"
#include "esp_vfs_eventfd.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "openthread/instance.h"
#include "openthread/thread.h"
#include "openthread/thread_ftd.h"

#include "esp_ot_config.h"

#define TAG "VINDRIKTNING_OT"
#define FW_VERSION "0.1.7-phase1-heartbeat"

static const char *role_to_string(otDeviceRole role)
{
    switch (role) {
        case OT_DEVICE_ROLE_DISABLED: return "disabled";
        case OT_DEVICE_ROLE_DETACHED: return "detached";
        case OT_DEVICE_ROLE_CHILD:    return "child";
        case OT_DEVICE_ROLE_ROUTER:   return "router";
        case OT_DEVICE_ROLE_LEADER:   return "leader";
        default:                      return "unknown";
    }
}

static esp_netif_t *init_openthread_netif(const esp_openthread_platform_config_t *config)
{
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_OPENTHREAD();
    esp_netif_t *netif = esp_netif_new(&cfg);
    assert(netif != NULL);
    ESP_ERROR_CHECK(esp_netif_attach(netif, esp_openthread_netif_glue_init(config)));
    return netif;
}

static void role_monitor_task(void *context)
{
    (void)context;
    otDeviceRole last_role = (otDeviceRole)0xff;
    uint32_t seconds = 0;
    bool pass_banner_printed = false;

    for (;;) {
        otDeviceRole role;

        esp_openthread_lock_acquire(portMAX_DELAY);
        role = otThreadGetDeviceRole(esp_openthread_get_instance());
        esp_openthread_lock_release();

        if (role != last_role) {
            ESP_LOGI(TAG, "THREAD ROLE CHANGE => %s", role_to_string(role));
            last_role = role;
        }

        if ((seconds % 5) == 0) {
            ESP_LOGI(TAG, "THREAD HEARTBEAT: uptime=%lus role=%s",
                     (unsigned long)seconds, role_to_string(role));
        }

        if (role == OT_DEVICE_ROLE_LEADER && !pass_banner_printed) {
            ESP_LOGI(TAG, "==============================================");
            ESP_LOGI(TAG, "PASS: ESP32-C6 formed a Thread network.");
            ESP_LOGI(TAG, "PASS: Native IEEE 802.15.4 radio is working.");
            ESP_LOGI(TAG, "PASS: FTD / Router / Leader capability works.");
            ESP_LOGI(TAG, "Phase 1 autonomous Thread test SUCCESS.");
            ESP_LOGI(TAG, "==============================================");
            pass_banner_printed = true;
        }

        seconds++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void configure_router_capable_mode(void)
{
    otInstance *instance = esp_openthread_get_instance();

    esp_openthread_lock_acquire(portMAX_DELAY);

    otLinkModeConfig mode = {0};
    mode.mRxOnWhenIdle = true;
    mode.mDeviceType = true;
    mode.mNetworkData = true;

    otError err = otThreadSetLinkMode(instance, mode);
    ESP_LOGI(TAG, "Set Link Mode rdn => %d", (int)err);

    err = otThreadSetRouterEligible(instance, true);
    ESP_LOGI(TAG, "Set Router Eligible => %d", (int)err);

    esp_openthread_lock_release();
}

static void ot_task_worker(void *context)
{
    (void)context;

    esp_openthread_platform_config_t config = {
        .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
        .port_config = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
    };

    ESP_LOGI(TAG, "[1/6] esp_openthread_init()");
    ESP_ERROR_CHECK(esp_openthread_init(&config));
    ESP_LOGI(TAG, "[2/6] OpenThread init OK");

    esp_netif_t *openthread_netif = init_openthread_netif(&config);
    esp_netif_set_default_netif(openthread_netif);
    ESP_LOGI(TAG, "[3/6] OpenThread netif attached");

    configure_router_capable_mode();
    ESP_LOGI(TAG, "[4/6] FTD / rdn / router-eligible configured");

    ESP_LOGI(TAG, "[5/6] Creating temporary Thread network...");
    ESP_ERROR_CHECK(esp_openthread_auto_start(NULL));
    ESP_LOGI(TAG, "[6/6] Thread enabled; waiting for role transition");

    BaseType_t ok = xTaskCreate(
        role_monitor_task,
        "thread_role_monitor",
        4096,
        NULL,
        4,
        NULL
    );
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "Failed to create role monitor task");
        abort();
    }

    ESP_ERROR_CHECK(esp_openthread_launch_mainloop());

    esp_openthread_netif_glue_deinit();
    esp_netif_destroy(openthread_netif);
    esp_vfs_eventfd_unregister();
    vTaskDelete(NULL);
}

static void init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing incompatible/full NVS");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
}

void app_main(void)
{
    printf("\n\n");
    printf("==================================================\n");
    printf("VINDRIKTNING ESP32-C6 Thread Router\n");
    printf("Firmware: %s\n", FW_VERSION);
    printf("Phase 1: autonomous Thread radio/router validation\n");
    printf("OpenThread CLI: DISABLED (ESP-IDF 5.x USB CLI workaround)\n");
    printf("System console: USB Serial/JTAG\n");
    printf("==================================================\n");
    fflush(stdout);

    esp_vfs_eventfd_config_t eventfd_config = {
        .max_fds = 3,
    };

    init_nvs();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_vfs_eventfd_register(&eventfd_config));

    BaseType_t ok = xTaskCreate(
        ot_task_worker,
        "ot_main",
        10240,
        NULL,
        5,
        NULL
    );

    if (ok != pdPASS) {
        ESP_LOGE(TAG, "Failed to create OpenThread main task");
        abort();
    }
}

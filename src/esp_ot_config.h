/*
 * IKEA VINDRIKTNING ESP32-C6 Thread Router
 * Phase 1 autonomous Thread validation
 * ESP-IDF v5.5 initial-release compatible.
 */
#pragma once

#include "sdkconfig.h"
#include "soc/soc_caps.h"
#include "esp_openthread_types.h"

#if SOC_IEEE802154_SUPPORTED
#define ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG() \
    {                                         \
        .radio_mode = RADIO_MODE_NATIVE,      \
    }
#else
#error "This firmware requires native IEEE 802.15.4 support."
#endif

#define ESP_OPENTHREAD_DEFAULT_HOST_CONFIG() \
    {                                        \
        .host_connection_mode = HOST_CONNECTION_MODE_NONE, \
    }

#define ESP_OPENTHREAD_DEFAULT_PORT_CONFIG() \
    {                                        \
        .storage_partition_name = "nvs",     \
        .netif_queue_size = 10,              \
        .task_queue_size = 10,               \
    }

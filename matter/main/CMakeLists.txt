idf_component_register(SRCS "app_main.cpp" "sensor_diag.cpp"
                       PRIV_INCLUDE_DIRS "." "${ESP_MATTER_PATH}/examples/common/utils"
                       PRIV_REQUIRES esp_matter chip openthread nvs_flash esp_driver_i2c esp_driver_gpio)

target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")

#pragma once

#include <cstdint>

// Endpoint IDs are assigned by ESP-Matter at boot. Passing them into the I2C
// task keeps the driver independent of the data-model construction in app_main.
struct SensorMatterEndpoints {
    uint16_t temperature;
    uint16_t humidity;
    uint16_t pressure;
};

// Starts periodic AHT20/BMP280 acquisition and reports valid samples to Matter.
// AHT20 failures are published as null after three consecutive failed reads so a
// controller does not keep presenting an indefinitely stale measurement.
void start_sensor_diagnostics(const SensorMatterEndpoints &endpoints);

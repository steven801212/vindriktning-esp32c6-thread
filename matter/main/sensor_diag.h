#pragma once

// Starts periodic read-only AHT20 / BMP280 diagnostics on XIAO ESP32-C6 I2C.
// Existing Matter endpoint values are deliberately left unchanged.
void start_sensor_diagnostics();

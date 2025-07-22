/**
    @title     ESP32-LedsDriver
    @file      ESP32-LedsDriverESP32P4.cpp
    @repo      https://github.com/ewowi/ESP32-LedsDriver, submit changes to this file as PRs
    @Authors   https://github.com/ewowi/ESP32-LedsDriver/commits/main
    @Doc       https://github.com/ewowi/ESP32-LedsDriver/
    @Copyright © 2025 Yves BAZIN
    @license   The MIT License (MIT)
    @license   For non MIT usage, commercial licenses must be purchased. Contact us for more information.
**/

//in this .cpp only functions specific for P4, both Physical and Virtual: LedsDriverESP32P4 (not yet), PhysicalDriverESP32P4 and VirtualDriverESP32P4

#ifdef CONFIG_IDF_TARGET_ESP32P4

#include "ESP32-LedsDriver.h"

// #include "driver/gpio.h"
// #include "soc/soc.h"
// // #include "soc/io_mux_reg.h"
// #include "hal/gpio_hal.h"
// // #include "soc/gpio_periph.h"
// #include "hal/gpio_ll.h"
// #include "soc/gpio_struct.h"
// #include "rom/gpio.h"

void PhysicalDriverESP32P4::setPins() {
}

void VirtualDriverESP32P4::setPins() {
}

#endif //CONFIG_IDF_TARGET_ESP32P4
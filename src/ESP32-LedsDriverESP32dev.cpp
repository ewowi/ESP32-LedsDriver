/**
    @title     ESP32-LedsDriver
    @file      ESP32-LedsDriverESP32dev.cpp
    @repo      https://github.com/ewowi/ESP32-LedsDriver, submit changes to this file as PRs
    @Authors   https://github.com/ewowi/ESP32-LedsDriver/commits/main
    @Doc       https://github.com/ewowi/ESP32-LedsDriver/
    @Copyright © 2025 Github ESP32-LedsDriver Commit Authors
    @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
    @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
**/

#ifdef CONFIG_IDF_TARGET_ESP32

#include "ESP32-LedsDriver.h"

#include "driver/gpio.h"
#include "soc/soc.h"
// #include "soc/io_mux_reg.h"
#include "hal/gpio_hal.h"
// #include "soc/gpio_periph.h"
#include "hal/gpio_ll.h"
#include "soc/gpio_struct.h"
#include "rom/gpio.h"

void LedsDriverESP32dev::setPinsM()
{
    for (int i = 0; i < numPins; i++)
    {
        PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pinConfig[i].gpio], PIN_FUNC_GPIO);
        gpio_set_direction((gpio_num_t)pinConfig[i].gpio, (gpio_mode_t)GPIO_MODE_DEF_OUTPUT);
        gpio_matrix_out(pinConfig[i].gpio, deviceBaseIndex[I2S_DEVICE] + i + 8, false, false);
    }
}

void PhysicalDriverESP32dev::setPins()
{
    setPinsM();
}

void VirtualDriverESP32dev::setPins() {
    setPinsM();

    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[latchPin], PIN_FUNC_GPIO);
    gpio_set_direction((gpio_num_t)latchPin, (gpio_mode_t)GPIO_MODE_DEF_OUTPUT);
    gpio_matrix_out(latchPin, deviceBaseIndex[I2S_DEVICE] + numPins + 8, false, false);
    gpio_set_direction((gpio_num_t)clockPin, (gpio_mode_t)GPIO_MODE_DEF_OUTPUT);
    gpio_matrix_out(clockPin, deviceClockIndex[I2S_DEVICE], false, false);
}

#endif //CONFIG_IDF_TARGET_ESP32
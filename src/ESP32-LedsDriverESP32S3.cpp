/**
    @title     ESP32-LedsDriver
    @file      ESP32-LedsDriverESP32S3.cpp
    @repo      https://github.com/ewowi/ESP32-LedsDriver, submit changes to this file as PRs
    @Authors   https://github.com/ewowi/ESP32-LedsDriver/commits/main
    @Doc       https://github.com/ewowi/ESP32-LedsDriver/
    @Copyright © 2025 Github ESP32-LedsDriver Commit Authors
    @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
    @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
**/

#ifdef CONFIG_IDF_TARGET_ESP32S3

#include "ESP32-LedsDriver.h"

#include "driver/gpio.h"
#include "soc/soc.h"
// #include "soc/io_mux_reg.h"
#include "hal/gpio_hal.h"
// #include "soc/gpio_periph.h"
#include "hal/gpio_ll.h"
#include "soc/gpio_struct.h"
#include "rom/gpio.h"

void PhysicalDriverESP32S3::setPins() {
    // using bus_config and io_config ...
    // for (int i = 0; i < numstrip; i++) {
    //     bus_config.data_gpio_nums[i] = pins[i];
    // }
    // if (numstrip < 16) {
    //     for (int i = numstrip; i < 16; i++) {
    //         bus_config.data_gpio_nums[i] = 0;
    //     }
    // }
}

void VirtualDriverESP32S3::setPins() {
    for (int i = 0; i < numPins; i++)
    {
        esp_rom_gpio_connect_out_signal(pinConfig[i].gpio, signalsID[i], false, false);
        gpio_hal_iomux_func_sel(GPIO_PIN_MUX_REG[pinConfig[i].gpio], PIN_FUNC_GPIO);
        gpio_set_drive_capability((gpio_num_t)pinConfig[i].gpio, (gpio_drive_cap_t)3);
    }
    esp_rom_gpio_connect_out_signal(latchPin, signalsID[numPins], false, false);
    gpio_hal_iomux_func_sel(GPIO_PIN_MUX_REG[latchPin], PIN_FUNC_GPIO);
    gpio_set_drive_capability((gpio_num_t)latchPin, (gpio_drive_cap_t)3);

    esp_rom_gpio_connect_out_signal(clockPin, LCD_PCLK_IDX, false, false);
    gpio_hal_iomux_func_sel(GPIO_PIN_MUX_REG[clockPin], PIN_FUNC_GPIO);
    gpio_set_drive_capability((gpio_num_t)clockPin, (gpio_drive_cap_t)3);
}

#endif //CONFIG_IDF_TARGET_ESP32S3
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
    for (int i = 0; i < numPins; i++) {
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

void VirtualDriverESP32S3::initDMABuffers() {
    initDMABuffersVirtual(); // for all Virtual drivers, used by non S3 and S3

    // S3 specific
    for (int buff_num = 0; buff_num < __NB_DMA_BUFFER - 1; buff_num++) {

        DMABuffersTampon[buff_num]->next = DMABuffersTampon[buff_num + 1];
    }

    DMABuffersTampon[__NB_DMA_BUFFER - 1]->next = DMABuffersTampon[0];
    DMABuffersTampon[__NB_DMA_BUFFER]->next = DMABuffersTampon[0];
    // memset(DMABuffersTampon[__NB_DMA_BUFFER]->buffer,0,WS2812_DMA_DESCRIPTOR_BUFFER_MAX_SIZE);
    // memset(DMABuffersTampon[__NB_DMA_BUFFER+1]->buffer,0,WS2812_DMA_DESCRIPTOR_BUFFER_MAX_SIZE);
    DMABuffersTampon[__NB_DMA_BUFFER + 1]->next = NULL;
    DMABuffersTampon[__NB_DMA_BUFFER]->dw0.suc_eof = 0;
}

LedDriverDMABuffer *VirtualDriverESP32S3::allocateDMABuffer(int bytes) {
    LedDriverDMABuffer *b = allocateDMABufferVirtual(bytes);

    //S3 specific
    b->dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_DMA;
    b->dw0.size = bytes;
    b->dw0.length = bytes;
    b->dw0.suc_eof = 1;
    return b;
}

void VirtualDriverESP32S3::putdefaultones(uint16_t *buffer) {
    putdefaultonesVirtual(buffer); // for all Virtual drivers, S3 and non S3

    //virtual S3 specific:
    uint16_t mas = 0xFFFF & (~(0xffff << (numPins)));
    // printf("mas%d\n",mas);
    for (int j = 0; j < 8 * channelsPerLed; j++) {
        buffer[0 + j * (3 * (NUM_VIRT_PINS + 1))] = 0xFFFF;
        buffer[1 + j * (3 * (NUM_VIRT_PINS + 1))] = mas;
    }
}

#endif //CONFIG_IDF_TARGET_ESP32S3
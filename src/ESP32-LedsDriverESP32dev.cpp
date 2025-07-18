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

void LedsDriverESP32dev::setPinsDev()
{
    for (int i = 0; i < numPins; i++)
    {
        PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pinConfig[i].gpio], PIN_FUNC_GPIO);
        gpio_set_direction((gpio_num_t)pinConfig[i].gpio, (gpio_mode_t)GPIO_MODE_DEF_OUTPUT);
        gpio_matrix_out(pinConfig[i].gpio, deviceBaseIndex[I2S_DEVICE] + i + 8, false, false);
    }
}

LedDriverDMABuffer *PhysicalDriverESP32dev::allocateDMABuffer(int bytes)
{
    LedDriverDMABuffer *b = (LedDriverDMABuffer *)heap_caps_malloc(sizeof(LedDriverDMABuffer), MALLOC_CAP_DMA);
    if (!b)
    {
        ESP_LOGE(TAG, "No more memory\n");
        return NULL;
    }

    b->buffer = (uint8_t *)heap_caps_malloc(bytes, MALLOC_CAP_DMA);
    if (!b->buffer)
    {
        ESP_LOGE(TAG, "No more memory\n");
        return NULL;
    }
    memset(b->buffer, 0, bytes);

    b->descriptor.length = bytes;
    b->descriptor.size = bytes;
    b->descriptor.owner = 1;
    b->descriptor.sosf = 1;
    b->descriptor.buf = b->buffer;
    b->descriptor.offset = 0;
    b->descriptor.empty = 0;
    b->descriptor.eof = 1;
    b->descriptor.qe.stqe_next = 0;

    return b;
}

void PhysicalDriverESP32dev::setPins()
{
    setPinsDev();
}

void PhysicalDriverESP32dev::initDMABuffers() {
    DMABuffersTampon[0] = allocateDMABuffer(channelsPerLed * 8 * 2 * 3); //the buffers for the
    DMABuffersTampon[1] = allocateDMABuffer(channelsPerLed * 8 * 2 * 3);
    DMABuffersTampon[2] = allocateDMABuffer(channelsPerLed * 8 * 2 * 3);
    DMABuffersTampon[3] = allocateDMABuffer(channelsPerLed * 8 * 2 * 3 * 4);

    putdefaultones((uint16_t *)DMABuffersTampon[0]->buffer);
    putdefaultones((uint16_t *)DMABuffersTampon[1]->buffer);
}

void PhysicalDriverESP32dev::putdefaultones(uint16_t *buffer) {
    /*order to push the data to the pins
    0:D7
    1:1
    2:1
    3:0
    4:0
    5:D6
    6:D5
    7:1
    8:1
    9:0
    10:0
    11:D4
    12:D3
    13:1
    14:1
    15:0
    16:0
    17:D2
    18:D1
    19:1
    20:1
    21:0
    22:0
    23:D0
    */
    for (int i = 0; i < channelsPerLed * 8 / 2; i++)
    {
        buffer[i * 6 + 1] = 0xffff;
        buffer[i * 6 + 2] = 0xffff;
    }
}

void VirtualDriverESP32dev::setPins() {
    setPinsDev();

    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[latchPin], PIN_FUNC_GPIO);
    gpio_set_direction((gpio_num_t)latchPin, (gpio_mode_t)GPIO_MODE_DEF_OUTPUT);
    gpio_matrix_out(latchPin, deviceBaseIndex[I2S_DEVICE] + numPins + 8, false, false);
    gpio_set_direction((gpio_num_t)clockPin, (gpio_mode_t)GPIO_MODE_DEF_OUTPUT);
    gpio_matrix_out(clockPin, deviceClockIndex[I2S_DEVICE], false, false);
}

void VirtualDriverESP32dev::initDMABuffers() {
    initDMABuffersVirtual(); // for all Virtual drivers, S3 and non S3
}

LedDriverDMABuffer *VirtualDriverESP32dev::allocateDMABuffer(int bytes) {
    LedDriverDMABuffer * b = allocateDMABufferVirtual(bytes);

    //dev specific
    b->descriptor.length = bytes;
    b->descriptor.size = bytes;
    b->descriptor.owner = 1;
    b->descriptor.sosf = 1;
    b->descriptor.buf = b->buffer;
    b->descriptor.offset = 0;
    b->descriptor.empty = 0;
    b->descriptor.eof = 1;
    b->descriptor.qe.stqe_next = 0;
    return b;
}

void VirtualDriverESP32dev::putdefaultones(uint16_t *buff)
{
    putdefaultonesVirtual(buff); // for all Virtual drivers, S3 and non S3

    //virtual dev specific: 
    uint16_t mas = 0xFFFF & (~(0xffff << (numPins)));

    for (int j = 0; j < 8 * channelsPerLed; j++)
    {
        buff[1 + j * (3 * (NUM_VIRT_PINS + 1))] = 0xFFFF;
        buff[0 + j * (3 * (NUM_VIRT_PINS + 1))] = mas;
    }
}

#endif //CONFIG_IDF_TARGET_ESP32
/**
    @title     ESP32-LedsDriver
    @file      ESP32-LedsDriverVirtual.cpp
    @repo      https://github.com/ewowi/ESP32-LedsDriver, submit changes to this file as PRs
    @Authors   https://github.com/ewowi/ESP32-LedsDriver/commits/main
    @Doc       https://github.com/ewowi/ESP32-LedsDriver/
    @Copyright © 2025 Yves BAZIN
    @license   The MIT License (MIT)
    @license   For non MIT usage, commercial licenses must be purchased. Contact us for more information.
**/

//in this .cpp only functions specific for virtual, for all boards: class VirtualDriver

#include "ESP32-LedsDriver.h"

void VirtualDriver::setLatchAndClockPin(uint8_t latchPin, uint8_t clockPin) {
    this->latchPin = latchPin;
    this->clockPin = clockPin;
}
void VirtualDriver::setClockSpeed(clock_speed clockSpeed) {
    this->clockSpeed = clockSpeed;
}

void VirtualDriver::startDriver() {
    // to do
}

void VirtualDriver::initDMABuffersVirtual() {
    DMABuffersTampon = (LedDriverDMABuffer **)heap_caps_malloc(sizeof(LedDriverDMABuffer *) * (__NB_DMA_BUFFER + 2), MALLOC_CAP_DMA);

    for (int num_buff = 0; num_buff < __NB_DMA_BUFFER + 2; num_buff++) {
        #ifdef CONFIG_IDF_TARGET_ESP32
            int WS2812_DMA_DESCRIPTOR_BUFFER_MAX_SIZE = ((NUM_VIRT_PINS + 1) * channelsPerLed * 8 * 3 * 2 + _DMA_EXTENSTION * 4);
        #else
            int WS2812_DMA_DESCRIPTOR_BUFFER_MAX_SIZE = (576 * 2);
        #endif
        DMABuffersTampon[num_buff] = allocateDMABuffer(WS2812_DMA_DESCRIPTOR_BUFFER_MAX_SIZE);
        putdefaultlatch((uint16_t *)DMABuffersTampon[num_buff]->buffer);
    } // the buffers for the

    for (int num_buff = 0; num_buff < __NB_DMA_BUFFER; num_buff++) {
        putdefaultones((uint16_t *)DMABuffersTampon[num_buff]->buffer);
    }
}

LedDriverDMABuffer *VirtualDriver::allocateDMABufferVirtual(int bytes) {
    LedDriverDMABuffer *b = (LedDriverDMABuffer *)heap_caps_malloc(sizeof(LedDriverDMABuffer), MALLOC_CAP_DMA);
    if (!b)
    {
        ESP_LOGE(TAG, "No more memory\n");
        return NULL;
    }

    b->buffer = (uint8_t *)heap_caps_malloc(bytes, MALLOC_CAP_DMA);

    if (!b->buffer) {
        ESP_LOGE(TAG, "No more memory\n");
        return NULL;
    }

    memset(b->buffer, 0, bytes);

    return b;
}

void VirtualDriver::putdefaultonesVirtual(uint16_t *buffer) {
    uint16_t mas = 0xFFFF & (~(0xffff << (numPins)));
    // printf("mas%d\n",mas);
    for (int j = 0; j < 8 * channelsPerLed; j++) {
        buffer[3 + j * (3 * (NUM_VIRT_PINS + 1))] = mas;
        buffer[2 + j * (3 * (NUM_VIRT_PINS + 1))] = mas;
        buffer[5 + j * (3 * (NUM_VIRT_PINS + 1))] = mas;
        buffer[4 + j * (3 * (NUM_VIRT_PINS + 1))] = mas;
        buffer[7 + j * (3 * (NUM_VIRT_PINS + 1))] = mas;
        buffer[6 + j * (3 * (NUM_VIRT_PINS + 1))] = mas;
    }
}
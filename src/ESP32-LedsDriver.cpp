/**
    @title     ESP32-LedsDriver
    @file      ESP32-LedsDriver.cpp
    @repo      https://github.com/ewowi/ESP32-LedsDriver, submit changes to this file as PRs
    @Authors   https://github.com/ewowi/ESP32-LedsDriver/commits/main
    @Doc       https://github.com/ewowi/ESP32-LedsDriver/
    @Copyright © 2025 Yves BAZIN
    @license   The MIT License (MIT)
    @license   For non MIT usage, commercial licenses must be purchased. Contact us for more information.
**/

#include "ESP32-LedsDriver.h"

void LedsDriver::initLeds(uint8_t *leds, PinConfig *pinConfig, size_t numPins, uint8_t channelsPerLed, uint8_t offsetRed, uint8_t offsetGreen, uint8_t offsetBlue, uint8_t offsetWhite) {
    this->leds = leds;
    this->pinConfig = pinConfig;
    this->numPins = numPins;
    this->channelsPerLed = channelsPerLed;
    this->offsetRed = offsetRed;
    this->offsetGreen = offsetGreen;
    this->offsetBlue = offsetBlue;
    this->offsetWhite = offsetWhite;

    for (size_t pin = 0; pin < numPins; pin++)
        ESP_LOGD(TAG, "gpio:%d #:%d", pinConfig[pin].gpio, pinConfig[pin].nrOfLeds);
    ESP_LOGD(TAG, "#: %d r:%d g:%d b:%d w:%d", channelsPerLed, offsetRed, offsetGreen, offsetBlue, offsetWhite);

    //overridden in derived classes:
    setPins();
    i2sInit();
    initDMABuffers();
}

void LedsDriver::setBrightness(uint8_t brightness) {
    ESP_LOGD(TAG, "%d", brightness);
    this->brightness = brightness;
}

void LedsDriver::setColorCorrection(uint8_t red, uint8_t green, uint8_t blue) {
    ESP_LOGD(TAG, "r:%d g:%d b:%d", red, green, blue);
    this->correctionRed = red;
    this->correctionGreen = green;
    this->correctionBlue = blue;
}

void LedsDriver::setPins() {
    ESP_LOGW(TAG, "This function should be overriden for the specific ESP32 you are compiling for!");
}

void LedsDriver::i2sInit() {
    ESP_LOGW(TAG, "This function should be overriden for the specific ESP32 you are compiling for!");
    // i2sReset();
    // i2sReset_DMA();
    // i2sReset_FIFO();
}

void LedsDriver::initDMABuffers() {
    ESP_LOGW(TAG, "This function should be overriden for the specific ESP32 you are compiling for!");
}

LedDriverDMABuffer *LedsDriver::allocateDMABuffer(int bytes) {
    ESP_LOGW(TAG, "This function should be overriden for the specific ESP32 you are compiling for!");
    return nullptr;
}

void LedsDriver::putdefaultlatch(uint16_t *buffer) {
    ESP_LOGW(TAG, "This function should be overriden for the specific ESP32 you are compiling for!");
}

void LedsDriver::putdefaultones(uint16_t *buffer) {
    ESP_LOGW(TAG, "This function should be overriden for the specific ESP32 you are compiling for!");
}

void LedsDriver::show() {
    // to do
}

void LedsDriver::setPixel(uint16_t ledNr, uint8_t red, uint8_t green, uint8_t blue, uint8_t white) {
    uint16_t channelNr = ledNr * channelsPerLed;
    leds[channelNr + offsetRed] = red;
    leds[channelNr + offsetGreen] = green;
    leds[channelNr + offsetBlue] = blue;
    if (offsetWhite != UINT8_MAX)
        leds[channelNr + offsetWhite] = white;
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
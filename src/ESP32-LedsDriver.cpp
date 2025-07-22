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

    maxNrOfLedsPerPin = 0;
    for (size_t pin = 0; pin < numPins; pin++) {
        ESP_LOGD(TAG, "gpio:%d #:%d", pinConfig[pin].gpio, pinConfig[pin].nrOfLeds);
        if (pinConfig[pin].nrOfLeds > maxNrOfLedsPerPin) maxNrOfLedsPerPin = pinConfig[pin].nrOfLeds;
    }
    ESP_LOGD(TAG, "#: %d r:%d g:%d b:%d w:%d max:%d", channelsPerLed, offsetRed, offsetGreen, offsetBlue, offsetWhite, maxNrOfLedsPerPin);

    //overridden in derived classes:
    startDriver();
    setPins();
    i2sInit();
    initDMABuffers();
}

void LedsDriver::setBrightness(uint8_t brightness) {
    ESP_LOGD(TAG, "%d", brightness);
    this->brightness = brightness;
    float tmp;
    for (int i = 0; i < 256; i++) {
        tmp = powf((float)i / 255, 1 / _gammag);
        __green_map[i] = (uint8_t)(tmp * brightness);
        tmp = powf((float)i / 255, 1 / _gammab);
        __blue_map[i] = (uint8_t)(tmp * brightness);
        tmp = powf((float)i / 255, 1 / _gammar);
        __red_map[i] = (uint8_t)(tmp * brightness);
        tmp = powf((float)i / 255, 1 / _gammaw);
        __white_map[i] = (uint8_t)(tmp * brightness);
    }
}

uint8_t LedsDriver::getBrightness() {
    return brightness;
}

void LedsDriver::setColorCorrection(uint8_t red, uint8_t green, uint8_t blue, uint8_t white) {
    ESP_LOGD(TAG, "r:%d g:%d b:%d", red, green, blue);
    this->correctionRed = red;
    this->correctionGreen = green;
    this->correctionBlue = blue;
    this->correctionWhite = white;

    _gammag = green/255.0;
    _gammar = red/255.0;
    _gammaw = white/255.0;
    _gammab = blue/255.0;
    setBrightness(brightness); //force brightness to correct the rgb map tables
}

void LedsDriver::getColorCorrection(uint8_t &red, uint8_t &green, uint8_t &blue, uint8_t &white) {
    red = correctionRed;
    green = correctionGreen;
    blue = correctionBlue;
    white = correctionWhite;
}

void LedsDriver::startDriver() {
    ESP_LOGD(TAG, "This function is optional for a specific driver!"); // currently only Virtual
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

#define AA (0x00AA00AAL)
#define CC (0x0000CCCCL)
#define FF (0xF0F0F0F0L)
#define FF2 (0x0F0F0F0FL)

void IRAM_ATTR LedsDriver::transpose16x1_noinline2(unsigned char *A, uint16_t *B) {

    uint32_t x, y, x1, y1, t;

    y = *(unsigned int *)(A);
    if (numPins > 4)
        x = *(unsigned int *)(A + 4);
    else
        x = 0;

    if (numPins > 8)
        y1 = *(unsigned int *)(A + 8);
    else
        y1 = 0;


    if (numPins > 12)
        x1 = *(unsigned int *)(A + 12);
    else
        x1 = 0;

    // pre-transform x
    if (numPins > 4) {
        t = (x ^ (x >> 7)) & AA;
        x = x ^ t ^ (t << 7);
        t = (x ^ (x >> 14)) & CC;
        x = x ^ t ^ (t << 14);
    }

    if (numPins > 12) {
        t = (x1 ^ (x1 >> 7)) & AA;
        x1 = x1 ^ t ^ (t << 7);
        t = (x1 ^ (x1 >> 14)) & CC;
        x1 = x1 ^ t ^ (t << 14);
    }

    // pre-transform y
    t = (y ^ (y >> 7)) & AA;
    y = y ^ t ^ (t << 7);
    t = (y ^ (y >> 14)) & CC;
    y = y ^ t ^ (t << 14);

    if (numPins > 8) {
        t = (y1 ^ (y1 >> 7)) & AA;
        y1 = y1 ^ t ^ (t << 7);
        t = (y1 ^ (y1 >> 14)) & CC;
        y1 = y1 ^ t ^ (t << 14);
    }

    // final transform
    t = (x & FF) | ((y >> 4) & FF2);
    y = ((x << 4) & FF) | (y & FF2);
    x = t;

    t = (x1 & FF) | ((y1 >> 4) & FF2);
    y1 = ((x1 << 4) & FF) | (y1 & FF2);
    x1 = t;

    *((uint16_t *)(B)) =
        (uint16_t)(((x & 0xff000000) >> 8 | ((x1 & 0xff000000))) >> 16);
    *((uint16_t *)(B + 3)) =
        (uint16_t)(((x & 0xff0000) >> 16 | ((x1 & 0xff0000) >> 8)));
    *((uint16_t *)(B + 6)) =
        (uint16_t)(((x & 0xff00) | ((x1 & 0xff00) << 8)) >> 8);
    *((uint16_t *)(B + 9)) = (uint16_t)((x & 0xff) | ((x1 & 0xff) << 8));
    *((uint16_t *)(B + 12)) =
        (uint16_t)(((y & 0xff000000) >> 8 | ((y1 & 0xff000000))) >> 16);
    *((uint16_t *)(B + 15)) =
        (uint16_t)(((y & 0xff0000) | ((y1 & 0xff0000) << 8)) >> 16);
    *((uint16_t *)(B + 18)) =
        (uint16_t)(((y & 0xff00) | ((y1 & 0xff00) << 8)) >> 8);
    *((uint16_t *)(B + 21)) = (uint16_t)((y & 0xff) | ((y1 & 0xff) << 8));
}
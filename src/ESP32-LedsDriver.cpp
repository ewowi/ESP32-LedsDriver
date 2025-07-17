/**
    @title     ESP32-LedsDriver
    @file      ESP32-LedsDriver.cpp
    @repo      https://github.com/ewowi/ESP32-LedsDriver, submit changes to this file as PRs
    @Authors   https://github.com/ewowi/ESP32-LedsDriver/commits/main
    @Doc       https://github.com/ewowi/ESP32-LedsDriver/
    @Copyright © 2025 Github ESP32-LedsDriver Commit Authors
    @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
    @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
**/

#include "ESP32-LedsDriver.h"

void ESP32LedsDriver::initLeds(uint8_t *leds, PinConfig *pinConfig, size_t numPins, uint8_t channelsPerLed, uint8_t offsetRed, uint8_t offsetGreen, uint8_t offsetBlue, uint8_t offsetWhite) {
    this->leds = leds;
    // to do: save pinConfig
    this->channelsPerLed = channelsPerLed;
    this->offsetRed = offsetRed;
    this->offsetGreen = offsetGreen;
    this->offsetBlue = offsetBlue;
    this->offsetWhite = offsetWhite;

    for (size_t pin = 0; pin < numPins; pin++)
        ESP_LOGD(TAG, "gpio:%d #:%d", pinConfig[pin].gpio, pinConfig[pin].nrOfLeds);
    ESP_LOGD(TAG, "#: %d r:%d g:%d b:%d w:%d", channelsPerLed, offsetRed, offsetGreen, offsetBlue, offsetWhite);
}

void ESP32LedsDriver::show() {
    // to do
}

void ESP32LedsDriver::setPixel(uint16_t ledNr, uint8_t red, uint8_t green, uint8_t blue, uint8_t white) {
    uint16_t channelNr = ledNr * channelsPerLed;
    leds[channelNr + offsetRed] = red;
    leds[channelNr + offsetGreen] = green;
    leds[channelNr + offsetBlue] = blue;
    if (offsetWhite != UINT8_MAX)
        leds[channelNr + offsetWhite] = white;
}
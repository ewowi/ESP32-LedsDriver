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

void ESP32LedsDriver::initLeds(uint8_t *leds, uint8_t *pins, uint16_t *lengths,  size_t num_pins, ColorArrangment colorArrangement) {
    ESP_LOGD(TAG, "");
}

void ESP32LedsDriver::show() {
}
/**
    @title     ESP32-LedsDriver
    @file      ESP32-LedsDriver.h
    @repo      https://github.com/ewowi/ESP32-LedsDriver, submit changes to this file as PRs
    @Authors   https://github.com/ewowi/ESP32-LedsDriver/commits/main
    @Doc       https://github.com/ewowi/ESP32-LedsDriver/
    @Copyright © 2025 Github ESP32-LedsDriver Commit Authors
    @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
    @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
**/

#include <Arduino.h>

#undef TAG
#define TAG "🐸"

enum ColorArrangment
{
    ORDER_GRBW,
    ORDER_RGB,
    ORDER_RBG,
    ORDER_GRB,
    ORDER_GBR,
    ORDER_BRG,
    ORDER_BGR,
};

class ESP32LedsDriver {
public:
    //initialize the leds array, pins, ledsPerPin, number of pins and the color arrangement of LEDs
    void initLeds(uint8_t *leds, uint8_t *pins, uint16_t *ledsPerPin,  size_t numPins, ColorArrangment colorArrangement);

    //sends leds array to physical LEDs
    void show();
};
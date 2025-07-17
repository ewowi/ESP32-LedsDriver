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

struct PinConfig { //default 256 LEDs on pin 2 (see channelsPerLed to get total channels per pin)
    uint8_t gpio = 2;
    uint16_t nrOfLeds = 256;
};

class ESP32LedsDriver {
private:
    uint8_t *leds;
    uint8_t channelsPerLed = 3;
    uint8_t offsetRed = 1;
    uint8_t offsetGreen = 0;
    uint8_t offsetBlue = 2;
    uint8_t offsetWhite = UINT8_MAX;

public:

    //initialize the leds array, pins, ledsPerPin, number of pins and the color arrangement of LEDs
    void initLeds(uint8_t *leds, PinConfig *pinConfig, size_t numPins, uint8_t channelsPerLed = 3, uint8_t offsetRed = 1, uint8_t offsetGreen = 0, uint8_t offsetBlue = 2, uint8_t offsetWhite = UINT8_MAX);

    //sends leds array to physical LEDs
    void show();

    //sets RGB(W) values of a LED
    void setPixel(uint16_t ledNr, uint8_t red, uint8_t green, uint8_t blue, uint8_t white = UINT8_MAX);
};
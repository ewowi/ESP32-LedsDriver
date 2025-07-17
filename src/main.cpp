/**
    @title     ESP32-LedsDriver
    @file      main.cpp
    @repo      https://github.com/ewowi/ESP32-LedsDriver, submit changes to this file as PRs
    @Authors   https://github.com/ewowi/ESP32-LedsDriver/commits/main
    @Doc       https://github.com/ewowi/ESP32-LedsDriver/
    @Copyright © 2025 Github ESP32-LedsDriver Commit Authors
    @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
    @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
**/

#include <Arduino.h>

#include "ESP32-LedsDriver.h"

#define NUM_PINS 10
#define NUM_LEDSPERPIN 256
#define CHANNELS_PER_LED 3

ESP32LedsDriver ledsDriver;
PinConfig pinConfig[NUM_PINS];
uint8_t leds[NUM_PINS * NUM_LEDSPERPIN * CHANNELS_PER_LED];

void setup() {
  uint8_t gpio[NUM_PINS] = {22, 21, 14, 18, 5, 4, 2, 15, 13, 12};

  for (size_t pin = 0; pin < NUM_PINS; pin++) {
    pinConfig[pin].gpio = gpio[pin];
    pinConfig[pin].nrOfLeds = NUM_LEDSPERPIN;
  }

  ledsDriver.initLeds(leds, pinConfig, NUM_PINS, CHANNELS_PER_LED, 1, 0, 2); //102 is GRB
}

void loop() {
  for (size_t ledNr = 0; ledNr < NUM_PINS * NUM_LEDSPERPIN; ledNr++) {
    ledsDriver.setPixel(ledNr, random(255), random(255), random(255));
  }

  ledsDriver.show();
}


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
#define NUM_LEDSPERPIN 400

ESP32LedsDriver ledsDriver;
uint8_t leds[3 * NUM_LEDSPERPIN * NUM_PINS];

void setup() {
  uint8_t pins[NUM_PINS] = {22, 21, 14, 18, 5, 4, 2, 15, 13, 12};
  uint16_t ledsPerPin[NUM_PINS];
  for (size_t i = 0; i < NUM_PINS; i++) ledsPerPin[i] = NUM_LEDSPERPIN;

  ledsDriver.initLeds(leds, pins, ledsPerPin, NUM_PINS, ORDER_GRB);
}

void loop() {
  for (size_t i = 0; i < 3 * NUM_LEDSPERPIN * NUM_PINS; i++) leds[i] = random(255);

  ledsDriver.show();
}


/**
    @title     ESP32-LedsDriver
    @file      main.cpp
    @repo      https://github.com/ewowi/ESP32-LedsDriver, submit changes to this file as PRs
    @Authors   https://github.com/ewowi/ESP32-LedsDriver/commits/main
    @Doc       https://github.com/ewowi/ESP32-LedsDriver/
    @Copyright © 2025 Yves BAZIN
    @license   The MIT License (MIT)
    @license   For non MIT usage, commercial licenses must be purchased. Contact us for more information.
**/

#include <Arduino.h>

#include "ESP32-LedsDriver.h"

#define CHANNELS_PER_LED 3
#define Virtual 0

#if Virtual == 0
  #define NUM_PINS 10
  #define NUM_LEDSPERPIN 256
  #if CONFIG_IDF_TARGET_ESP32
    PhysicalDriverESP32dev ledsDriver;
  #elif CONFIG_IDF_TARGET_ESP32S3
    PhysicalDriverESP32S3 ledsDriver;
  #elif CONFIG_IDF_TARGET_ESP32P4
    PhysicalDriverESP32P4 ledsDriver;
  #endif
#else //Virtual
  #define NUM_PINS 6
  #define NUM_LEDSPERPIN 256 * 8 // one pin uses shiftregister to drive 8 panels
  #if CONFIG_IDF_TARGET_ESP32
    VirtualDriverESP32dev ledsDriver;
  #elif CONFIG_IDF_TARGET_ESP32S3
    VirtualDriverESP32S3 ledsDriver;
  #elif CONFIG_IDF_TARGET_ESP32P4
    VirtualDriverESP32P4 ledsDriver;
  #endif
#endif

PinConfig pinConfig[NUM_PINS];
uint8_t leds[NUM_PINS * NUM_LEDSPERPIN * CHANNELS_PER_LED];

void setup() {

  #if Virtual == 0
    uint8_t gpio[NUM_PINS] = {22, 21, 14, 18, 5, 4, 2, 15, 13, 12};
  #else
    #if CONFIG_IDF_TARGET_ESP32
      uint8_t gpio[NUM_PINS] = {14,12,13,25,33,32};
      ledsDriver.setLatchAndClockPin(27,26);
    #elif CONFIG_IDF_TARGET_ESP32S3
      uint8_t gpio[NUM_PINS] = {9,10,12,8,18,17};
      ledsDriver.setLatchAndClockPin(46,3);
    #elif CONFIG_IDF_TARGET_ESP32P4
      uint8_t gpio[NUM_PINS] = {9,10,12,8,18,17};
      ledsDriver.setLatchAndClockPin(46,3);
    #endif
    ledsDriver.setClockSpeed(clock_1000KHZ); // a bit of overclocking 🔥
  #endif

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


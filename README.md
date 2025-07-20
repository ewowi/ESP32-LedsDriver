# ESP32-LedsDriver

Physical and Virtual LedsDriver for ESP32-dev, ESP32-wrover, ESP32-S3, ESP32-P4.

* Physical: LEDs are connected to GPIO pins of an ESP32 board directly. 
    * Max 16 pins (or more...)
* Virtual: Between the ESP32 and the ledstrips there are a number of IC's:
    * 74HCT245 : this is a bus used as a level shifter (you will need only one of them for LATCH and CLOCK)
    * 74HCT595 : this is an 8 bit shift register (you will need one 74HC595 for each Virtual pin)
    * The T in the chipname is important! Maximal 15 74HCT595's possible resulting in a stunning 120 pin config 🔥. But a 6 chips / 48 pins setup is a nice sweet spot.

## Introduction

This library is a new take on a new take on driving LEDs and combines the following repos into one:

* [I2SClocklessLedDriver](https://github.com/hpwit/I2SClocklessLedDriver)
* [I2SClocklessLedDriveresp32s3](https://github.com/hpwit/I2SClocklessLedDriveresp32s3)
* [I2SClocklessVirtualLedDriver](https://github.com/hpwit/I2SClocklessVirtualLedDriver)
* [I2SClocklessVirtualLedDriveresp32s3](https://github.com/hpwit/I2SClocklessVirtualLedDriveresp32s3)

This has a number of advantages:

* Splitting .h into .h and .cpp libraries allowing for faster compile and no duplicate definition errors. It also makes things easier to read.
* Replacing #define variables where possible into class variables so they become runtime configurable (e.g. num strips, num leds per strip, color order, pins, dmaBuffer etc etc)
* Sharing code used in all libraries (which were in above repos sometimes slightly different) -> easier maintenance.
* Allow for new drivers e.g. physical and virtual for ESP32-P4 without creating too many repo's
* Currently this repo is not depending on FastLED, e.g. no CRGB struct or leds array, just uint8_t. Not sure if this is an advantage but it sounds okay-ish
* Have a unified interface for all of these libraries so it is easy to switch driver and #ifdef with different ESP32's:

Definition:

```cpp
PhysicalDriverESP32dev ledsDriver;
VirtualDriverESP32dev ledsDriver;

PhysicalDriverESP32S3 ledsDriver;
VirtualDriverESP32S3 ledsDriver;

PhysicalDriverESP32P4 ledsDriver;
VirtualDriverESP32P4 ledsDriver;
```

Interface:

```cpp
void initLeds(uint8_t *leds, PinConfig *pinConfig, size_t numPins, uint8_t channelsPerLed = 3, uint8_t offsetRed = 1, uint8_t offsetGreen = 0, uint8_t offsetBlue = 2, uint8_t offsetWhite = UINT8_MAX);

void setBrightness(uint8_t brightness);
uint8_t getBrightness();

void setColorCorrection(uint8_t red, uint8_t green, uint8_t blue, uint8_t white = UINT8_MAX);
void getColorCorrection(uint8_t &red, uint8_t &green, uint8_t &blue, uint8_t &white);

//sends leds array to physical LEDs
virtual void show();

//sets RGB(W) values of a LED
void setPixel(uint16_t ledNr, uint8_t red, uint8_t green, uint8_t blue, uint8_t white = UINT8_MAX);
```

Depending on the driver chosen additional interface functions are available:

* Virtual driver

```cpp
void setLatchAndClockPin(uint8_t latchPin, uint8_t clockPin);
void setClockSpeed( clock_speed clockSpeed);
```

See [main.cpp](https://github.com/ewowi/ESP32-LedsDriver/blob/main/src/main.cpp) for examples of using this library.

Behind the scenes the following functions are implemented for each driver:

* initLeds
    * startDriver();
    * setPins();
    * i2sInit();
    * initDMABuffers();
        * allocateDMABuffer
        * putdefaultlatch
        * putdefaultones
* show
    * transposeAll ... 


This repo is tuned to be easy to include in other repo's:

 - [MoonLight](https://github.com/MoonModules/MoonLight): See also [2025-07-16-MoonLightv057](https://moonmodules.org/2025-07-16-MoonLightv057) - this was the initial driver to start this project.
 - [FastLED](https://github.com/FastLED/FastLED): FastLED now uses [I2SClocklessLedDriverESP32S3](https://github.com/hpwit/I2SClocklessLedDriverESP32S3), it actually improved it. This improved version was used back into this project. It could be a good idea, once this repo is mature enough, to include this in FastLED and not have only Physical S3, but all Physical and Virtual flavors.

This project is 🚧:

| Board    | General | Physical | Virtual |
|----------|---------|----------|---------|
| **General** | 40%  | 40% | 40% |
| **dev / wrover** | 40% | 20% | 3% |
| **S3** | 40% | 70% | 3% |
| **P4** | 1% | 0% | 0% |

Future

- The origin repos do nice things with panels, with mappings, with rotating, bufferless leds etc. Not sure yet what to implement here, and if it will be available for all combinations of ESP32 board and Physical/ Virtual.


## Code

### 🚧

* LedsDriver (abstract)
    * initLeds()
        * virtual setPins()
        * virtual i2sInit();
        * virtual initDMABuffers();
        * virtual allocateDMABuffer()
        * virtual putdefaultlatch()
        * virtual putdefaultones()
    * show()
* CONFIG_IDF_TARGET_ESP32
    * LedsDriverESP32dev
        * PhysicalDriverESP32dev
            * setPins() 60%
            * i2sInit() 80% //todo LedDriverinterruptHandler implementation
            * initDMABuffers() 100%
            * allocateDMABuffer() 80% // not for FULL_DMA_BUFFER
            * putdefaultones() 100%
        * VirtualDriverESP32dev
            * setPins() 60%
            * i2sInit() 80%
            * initDMABuffers() 100%
            * allocateDMABuffer() 100%
            * putdefaultlatch() 100%
            * putdefaultones() 100%
* CONFIG_IDF_TARGET_ESP32S3
    * LedsDriverESP32S3
        * PhysicalDriverESP32S3
            * setPins()* n/a
            * i2sInit() 80% used as a placeholder for part of the initLeds code
            * initDMABuffers() 80% used as a placeholder for part of the initLeds code
            * allocateDMABuffer() n/a
            * putdefaultlatch() n/a
            * putdefaultones() n/a
            * show() 80%
        * VirtualDriverESP32S3
            * setPins() 60%
            * i2sInit() 80%  //todo LedDriverinterruptHandler implementation
            * initDMABuffers() 100%
            * allocateDMABuffer() 100%
            * putdefaultlatch() 100%
            * putdefaultones() 100%
* CONFIG_IDF_TARGET_ESP32P4
    * LedsDriverESP32P4
        * PhysicalDriverESP32P4
        * VirtualDriverESP32P4

### main.cpp

Run Physical or Virtual driver for all defined boards:

| Environment         | Status    | Duration |
| ------------------  | --------  | ------------ |
| esp32dev            | SUCCESS   | 00:00:03.637 |
| esp-wrover-kit      | SUCCESS   | 00:00:03.098 |
| esp32-s3-devkitc-1  | SUCCESS   | 00:00:02.658 |
| esp32-p4            | SUCCESS   | 00:00:01.988 |

### Class structure:

| Board    | General | Physical | Virtual |
|----------|---------|----------|---------|
| **General** | LedsDriver* | PhysicalDriver* | VirtualDriver* |
| **dev / wrover** | LedsDriverESP32dev* | PhysicalDriverESP32dev | VirtualDriverESP32dev |
| **S3** | LedsDriverESP32S3* | PhysicalDriverESP32S3 | VirtualDriverESP32S3 |
| **P4** | LedsDriverESP32P4* | PhysicalDriverESP32P4 | VirtualDriverESP32P4 |

*: Abstract classes

### Class usage

| Board    | General | Physical | Virtual |
|----------|---------|----------|---------|
| **General** | initLeds(), setPins(), setBrightness(), setColorCorrection(), show(), setPixel() | - | setLatchAndClockPin(), setClockSpeed(), dmaBuffer |
| **dev / wrover** | deviceBaseIndex, setPinsDev() | - | deviceClockIndex |
| **S3** | - | - | signalsID |
| **P4** | - | - | - |

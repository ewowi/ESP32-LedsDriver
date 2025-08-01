# ESP32-LedsDriver

Physical and Virtual LedsDriver for ESP32-dev, ESP32-wrover, ESP32-S3, ESP32-P4.

* **Physical**: LEDs are connected to GPIO pins of an ESP32 board (preferably level shifted with 74HCT125 and a few resistors).
    * Max 16 pins (or more...)
* **Virtual**: Between the ESP32 and the ledstrips there are a number of IC's:
    * 74HCT245 : this is a bus used as a level shifter (you will need only one of them for latch and clock)
    * 74HCT595 : this is an 8 bit shift register (you will need one for each virtual pin)
    * The T in the chipname is important (for HCT the 0-part is smaller than the 1 part so it works better with 3.3v)! The 595 also does the level shifting, resistors on each pin are recommended, see [QuinLed - The Myth of the Data Signal Resistor](https://quinled.info/data-signal-cable-conditioning/). Maximal 15 74HCT595's possible resulting in a stunning 120 pin config 🔥. But a 6 chips / 48 pins setup is a nice sweet spot.
* **Why**: Driving high number of LEDs at high framerates by one ESP32: 256 leds on one pin is 130FPS, 48 * 256 leds on 48 pins is also 130FPS. (effects slow it down to 50-120 FPS normally).

## Introduction

This library is a new take on a new take on driving LEDs and combines the following repos into one:

* [I2SClocklessLedDriver](https://github.com/hpwit/I2SClocklessLedDriver)
* [I2SClocklessLedDriveresp32s3](https://github.com/hpwit/I2SClocklessLedDriveresp32s3)
* [I2SClocklessVirtualLedDriver](https://github.com/hpwit/I2SClocklessVirtualLedDriver)
* [I2SClocklessVirtualLedDriveresp32s3](https://github.com/hpwit/I2SClocklessVirtualLedDriveresp32s3)

This has a number of advantages:

* Replacing #define variables where possible into class variables so they become **runtime configurable** instead of a new compilation for each config (e.g. num strips, num leds per strip, color order, pins, dmaBuffer etc etc)
* Have a **unified interface** for all of these libraries so it is easy to switch driver and #ifdef with different ESP32's:
* Sharing code used in all libraries (which were in above repos sometimes slightly different) -> **easier maintenance**.
* **Simplification of code** e.g. by using default arguments: amount of different initLed() functions reduced to 1! (There are many initLed() functions in the different repos).
* Splitting .h into .h and .cpp libraries allowing for **faster compile and no duplicate definition errors**. It also makes things easier to read.
* Allow for **new drivers e.g. physical and virtual for ESP32-P4** without creating too many repo's
* This library is setup as a **PlatformIO library**, allowing for easy compilation in VS Code.
* Currently this repo is **not depending on FastLED**, e.g. no CRGB struct or leds array, just uint8_t. Not sure if this is an advantage but it sounds okay-ish
* **Specific classes for specific boards**, minimizing the number of #ifdef CONFIG_IDF_TARGET_ESP32 calls which makes the code more readable.

Note: normal ESP32 board are called **ESP32-D0 / ESP32D0** in this repo to distinguish it from other boards like ESP32-S3, ESP32-P4 etc. ESP32D0 covers both esp32dev and esp-wrover-kit. esp-wrover-kit is included as it is a normal ESP32 with PSRAM, making it an excellent candidate for driving lots of LEDs as the LEDs buffer is stored in PSRAM. 

Definition:

```cpp
PhysicalDriverESP32D0 ledsDriver;
VirtualDriverESP32D0 ledsDriver;

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
        * i2sReset()
        * i2sReset_DMA()
        * i2sReset_FIFO()
    * initDMABuffers();
        * allocateDMABuffer()
        * putdefaultlatch()
        * putdefaultones()
* show
    * transposeAll / loadAndTranspose ... 


This repo is tuned to be **easy to include in other repo's**:

 - [MoonLight](https://github.com/MoonModules/MoonLight): See also [2025-07-16-MoonLightv057](https://moonmodules.org/2025-07-16-MoonLightv057) - this was the initial driver to start this project.
 - [FastLED](https://github.com/FastLED/FastLED): FastLED now uses [I2SClocklessLedDriverESP32S3](https://github.com/hpwit/I2SClocklessLedDriverESP32S3), it actually improved it. This improved version was used back into this project. It could be a good idea, once this repo is mature enough, to include this in FastLED and not have only Physical S3, but all Physical and Virtual flavors.

This project is 🚧:

| Board    | General | Physical | Virtual |
|----------|---------|----------|---------|
| **General** | 40%  | 40% | 40% |
| **D0** | 40% | 20% | 3% |
| **S3** | 40% | 70% | 3% |
| **P4** | 1% | 0% | 0% |

Future

- The origin repos do nice things with panels, with mappings, with rotating, bufferless leds etc. Not sure yet what to implement here, and if it will be available for all combinations of ESP32 board and Physical/ Virtual.


## Code

### How to read the code

[ESP32-LedsDriver.h](https://github.com/ewowi/ESP32-LedsDriver/blob/main/src/ESP32-LedsDriver.h) is the main 'orchestrator' file. It contains the definition for all driver classes:

 * Class **LedsDriver** which provides the general interface for all driver classes. Most important is initLeds() and show(). All other classes inherit these functions. A more technical example is transpose16x1_noinline2() which is also used by all classes. 
* LedsDriver has 2 **type subclasses**: **PhysicalDriver** and **VirtualDriver** which provides functionality for all boards. PhysicalDriver is empty at the moment. VirtualDriver contains latchPin, clockPin and clockSpeed functions and variables and virtual specific functions like initDMABuffersVirtual().
* LedsDriver has 3 **board subclasses**: **LedsDriverESP32D0**, **LedsDriverESP32S3** and **LedsDriverESP32P4**. They define the specifics for a board: currently only LedsDriverESP32D0 contains specifics: setPinsD0(), i2sInitD0(), i2sStop(), loadAndTranspose() and LedDriverinterruptHandler(). LedsDriverESP32S3 is empty as there is a specific implementation for Physical S3, and Virtual S3 still needs to be done.
* Each specific driver combines one of the type subclasses with one of the board subclasses (multiple inheritance). E.g. class PhysicalDriverESP32D0 uses PhysicalDriver and LedsDriverESP32D0 and completes the driver functionality e.g.:
    * setPins()
    * i2sInit()
    * initDMABuffers()
    * i2sStart()
    * show()
    See Definition above for the 6 drivers which are currently in place

* LedsDriver (virtual)
    * initLeds()
        * virtual setPins()
        * virtual i2sInit();
        * virtual initDMABuffers();
        * virtual allocateDMABuffer()
        * virtual putdefaultlatch()
        * virtual putdefaultones()
    * show()
* CONFIG_IDF_TARGET_ESP32
    * LedsDriverESP32D0
        * PhysicalDriverESP32D0
            * setPins() 60%
            * i2sInit() 80% //to do LedDriverinterruptHandler implementation
            * initDMABuffers() 100%
            * allocateDMABuffer() 80% // not for FULL_DMA_BUFFER
            * putdefaultones() 100%
        * VirtualDriverESP32D0
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
            * i2sInit() 80%  //to do LedDriverinterruptHandler implementation
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
| **D0** | LedsDriverESP32D0* | PhysicalDriverESP32D0 | VirtualDriverESP32D0 |
| **S3** | LedsDriverESP32S3* | PhysicalDriverESP32S3 | VirtualDriverESP32S3 |
| **P4** | LedsDriverESP32P4* | PhysicalDriverESP32P4 | VirtualDriverESP32P4 |

*: Virtual classes

### Class usage

| Board    | General | Physical | Virtual |
|----------|---------|----------|---------|
| **General** | initLeds(), setPins(), setBrightness(), setColorCorrection(), show(), setPixel() | - | setLatchAndClockPin(), setClockSpeed(), dmaBuffer |
| **D0** | deviceBaseIndex, setPinsD0() | - | deviceClockIndex |
| **S3** | - | - | signalsID |
| **P4** | - | - | - |

# ESP32-LedsDriver

Physical and Virtual LedsDriver for ESP32-dev, ESP32-wrover, ESP32-S3, ESP32-P4

## Introduction

This library is a new take on a new take on driving LEDs and combines the following repos into one:

* https://github.com/hpwit/I2SClocklessLedDriver.git
* https://github.com/hpwit/I2SClocklessLedDriverESP32S3.git
* https://github.com/hpwit/I2SClocklessVirtualLedDriver.git
* https://github.com/hpwit/I2SClocklessVirtualLedDriverESP32S3.git

This has some advantages

* Spliting .h into .h and .cpp libraries allowing for faster compile and no duplicate definition errors. It also makes things easier to read.
* Sharing code used in all libraries (sometimes slightly different)
* Allow for new drivers e.g. physical and virtual for ESP32-P4.
* Have a unified interface for all of these libraries:

Definition:

```cpp
    PhysicalDriverESP32dev ledsDriver;
    VirtualDriverESP32dev ledsDriver;

    PhysicalDriverESP32S3 ledsDriver;
    VirtualDriverESP32S3 ledsDriver;

    PhysicalDriverESP32P4 ledsDriver;
    VirtualDriverESP32P4 ledsDriver;

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

Depending on the driver chosen additional interface :

* Virtual driver

```cpp
    void setLatchAndClockPin(uint8_t latchPin, uint8_t clockPin);
    void setClockSpeed( clock_speed clockSpeed);
```

Behind the scenes the following functions are implemented for each driver:

* initLeds
    * startDriver();
    * setPins();
    * i2sInit();
    * initDMABuffers();
        * allocateDMABuffer
        * putdefaultlatch
        * putdefaultones

See also [2025-07-16-MoonLightv057](https://moonmodules.org/2025-07-16-MoonLightv057)

This project is 🚧:

| Board    | General | Physical | Virtual |
|----------|---------|----------|---------|
| **General** | 20%  | 20% | 20% |
| **dev / wrover** | 10% | 10% | 3% |
| **S3** | 3% | 3% | 3% |
| **P4** | 1% | 0% | 0% |

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
        * PhysicalDriverESP32S3 // on hold
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

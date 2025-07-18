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

// no #define but variables where possible
// only for esp.idf 5...

#pragma once

#define I2S_DEVICE 0

#include <Arduino.h>

#undef TAG
#define TAG "🐸"

struct PinConfig { //default 256 LEDs on pin 2 (see channelsPerLed to get total channels per pin)
    uint8_t gpio = 2;
    uint16_t nrOfLeds = 256;
};

#ifdef CONFIG_IDF_TARGET_ESP32
    #include "rom/lldesc.h"
    struct LedDriverDMABuffer // the old style structure (not for S3 etc)
    {
        lldesc_t descriptor;
        uint8_t *buffer;
    };
#else
    #include "hal/dma_types.h"
    typedef dma_descriptor_t LedDriverDMABuffer; // modern architectures.
#endif

class ESP32LedsDriver {
protected:
    uint8_t *leds;
    PinConfig *pinConfig; //assuming global storage!
    uint8_t numPins;
    uint8_t channelsPerLed = 3;
    uint8_t offsetRed = 1;
    uint8_t offsetGreen = 0;
    uint8_t offsetBlue = 2;
    uint8_t offsetWhite = UINT8_MAX;

    uint8_t brightness = UINT8_MAX;
    uint8_t correctionRed = UINT8_MAX;
    uint8_t correctionGreen = UINT8_MAX;
    uint8_t correctionBlue = UINT8_MAX;

    //override in derived classes, called by initLeds
    virtual void setPins(); // public to call again when pins change?
    virtual void i2sInit();
    virtual void initDMABuffers(); // general? Physical S3 doesn't seem to do anything with dma ...

    // called by initDMABuffers
    virtual LedDriverDMABuffer *allocateDMABuffer(int bytes);
    virtual void putdefaultones(uint16_t *buffer);
public:

    //initialize the leds array, pins, ledsPerPin, number of pins and the color arrangement of LEDs
    //color arrangements: supports RGB and RGBW but also exotic setups like LED Curtains where some have 6 channels per LEDS where only 3 channels are used.
    void initLeds(uint8_t *leds, PinConfig *pinConfig, size_t numPins, uint8_t channelsPerLed = 3, uint8_t offsetRed = 1, uint8_t offsetGreen = 0, uint8_t offsetBlue = 2, uint8_t offsetWhite = UINT8_MAX);

    void setBrightness(uint8_t brightness);

    void setColorCorrection(uint8_t red, uint8_t green, uint8_t blue);

    //sends leds array to physical LEDs
    void show();

    //sets RGB(W) values of a LED
    void setPixel(uint16_t ledNr, uint8_t red, uint8_t green, uint8_t blue, uint8_t white = UINT8_MAX);
};


// define subclasses for specific driver configs 
// not all might be needed
// specifics for 16x16 panels in separate class? (e.g. snake, width, height)

// See also README for class structure

//specifics for Physical Drivers!! (all boards)
class ESP32PhysicalDriver: virtual public ESP32LedsDriver { //abstract class !
    // no specifics yet
};

//specifics for Virtual Drivers!! (all boards)
class ESP32VirtualDriver: virtual public ESP32LedsDriver { //abstract class !
protected:
    uint8_t latchPin = 46; //46 for S3, 27 for ESP32 (wrover)
    uint8_t clockPin = 3; //3 for S3, 26 for ESP32 (wrover)
    uint16_t clockSpeed = 800; //🔥 only for virtual? to do, only clock_800KHZ, clock_1000KHZ, clock_1111KHZ, clock_1123KHZ
    uint8_t dmaBuffer = 30;
    uint8_t NUM_VIRT_PINS = 7;
    uint8_t __NB_DMA_BUFFER = 20; //for S3, 16 for non s3?
    uint8_t _DMA_EXTENSTION = 0;
    LedDriverDMABuffer **DMABuffersTampon; //[__NB_DMA_BUFFER + 2];

    void initDMABuffersVirtual();

    LedDriverDMABuffer *allocateDMABufferVirtual(int bytes);

    void putdefaultonesVirtual(uint16_t *buffer);
public:
    void setLatchAndClockPin(uint8_t latchPin, uint8_t clockPin) {
        this->latchPin = latchPin;
        this->clockPin = clockPin;
    }
    void setClockSpeed( uint16_t clockSpeed) {
        this->clockSpeed = clockSpeed;
    }
};

#ifdef CONFIG_IDF_TARGET_ESP32
    //specific for ESP32 devices!! (Physical and Virtual)
    class LedsDriverESP32dev: virtual public ESP32LedsDriver { //abstract class !
    protected:
        const int deviceBaseIndex[2] = {I2S0O_DATA_OUT0_IDX, I2S1O_DATA_OUT0_IDX};
        void setPinsDev();
    };

    //https://github.com/hpwit/I2SClocklessLedDriver
    class PhysicalDriverESP32dev: public LedsDriverESP32dev, public ESP32PhysicalDriver {
        void setPins() override;

        void initDMABuffers() override; // allocateDMABuffer + putdefaultones
        LedDriverDMABuffer *DMABuffersTampon[4]; // an array of pointers!!!
        LedDriverDMABuffer *allocateDMABuffer(int bytes); //Phys dev specific
        void putdefaultones(uint16_t *buffer) override; //Phys dev specific
    };
    //https://github.com/hpwit/I2SClocklessVirtualLedDriver (S3 and non S3)
    class VirtualDriverESP32dev: public LedsDriverESP32dev, public ESP32VirtualDriver {
    protected:
        const int deviceClockIndex[2] = {I2S0O_BCK_OUT_IDX, I2S1O_BCK_OUT_IDX};

        void setPins() override;

        void initDMABuffers() override; //initDMABuffersVirtual
        LedDriverDMABuffer *allocateDMABuffer(int bytes); //allocateDMABufferVirtual + dev specific
        void putdefaultones(uint16_t *buffer) override; // putdefaultonesVirtual + dev specific
    };
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S3

    //specific for ESP32S3 devices!! (Physical and Virtual)
    class LedsDriverESP32S3: virtual public ESP32LedsDriver { //abstract class !
    };

    //https://github.com/hpwit/I2SClockLessLedDriveresp32s3 (or FastLED), will implement later!
    class PhysicalDriverESP32S3: public LedsDriverESP32S3, public ESP32PhysicalDriver {
        void setPins() override;
    };
    //https://github.com/hpwit/I2SClocklessVirtualLedDriver (S3 and non S3)
    class VirtualDriverESP32S3: public LedsDriverESP32S3, public ESP32VirtualDriver {
        uint8_t signalsID[16] = {LCD_DATA_OUT0_IDX, LCD_DATA_OUT1_IDX, LCD_DATA_OUT2_IDX, LCD_DATA_OUT3_IDX, LCD_DATA_OUT4_IDX, LCD_DATA_OUT5_IDX, LCD_DATA_OUT6_IDX, LCD_DATA_OUT7_IDX, LCD_DATA_OUT8_IDX, LCD_DATA_OUT9_IDX, LCD_DATA_OUT10_IDX, LCD_DATA_OUT11_IDX, LCD_DATA_OUT12_IDX, LCD_DATA_OUT13_IDX, LCD_DATA_OUT14_IDX, LCD_DATA_OUT15_IDX};
        void setPins() override;

        void initDMABuffers() override; //initDMABuffersVirtual + S3 specifics
        LedDriverDMABuffer *allocateDMABuffer(int bytes); //allocateDMABufferVirtual + S3 specific
        void putdefaultones(uint16_t *buffer) override; //putdefaultonesVirtual + S3 specific
    };

#endif

#ifdef CONFIG_IDF_TARGET_ESP32P4
    //specific for ESP32P4 devices!! (Physical and Virtual)
    class LedsDriverESP32P4: virtual public ESP32LedsDriver { //abstract class !
    };

    class PhysicalDriverESP32P4: public LedsDriverESP32P4, public ESP32PhysicalDriver {
        void setPins() override;
    };

    class VirtualDriverESP32P4: public LedsDriverESP32P4, public ESP32VirtualDriver {
        void setPins() override;
    };
#endif
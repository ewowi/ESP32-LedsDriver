/**
    @title     ESP32-LedsDriver
    @file      ESP32-LedsDriver.h
    @repo      https://github.com/ewowi/ESP32-LedsDriver, submit changes to this file as PRs
    @Authors   https://github.com/ewowi/ESP32-LedsDriver/commits/main
    @Doc       https://github.com/ewowi/ESP32-LedsDriver/
    @Copyright © 2025 Yves BAZIN
    @license   The MIT License (MIT)
    @license   For non MIT usage, commercial licenses must be purchased. Contact us for more information.
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

#define MAX_PINS 20 //20 as the max for now (Virtual has 1 pin for 8 led pins, 15 max)
class LedsDriver {
protected:
    uint8_t *leds;
    PinConfig pinConfig[MAX_PINS];
    uint8_t numPins;
    uint8_t channelsPerLed = 3;
    uint8_t offsetRed = 1;
    uint8_t offsetGreen = 0;
    uint8_t offsetBlue = 2;
    uint8_t offsetWhite = UINT8_MAX;
    uint16_t maxNrOfLedsPerPin = 0;

    uint8_t brightness = UINT8_MAX;
    uint8_t correctionRed = UINT8_MAX;
    uint8_t correctionGreen = UINT8_MAX;
    uint8_t correctionBlue = UINT8_MAX;
    uint8_t correctionWhite = UINT8_MAX;

    float _gammar, _gammab, _gammag, _gammaw;

    //override in derived classes, called by initLeds
    virtual void startDriver(); // called by initLeds, used in Physical D0 and Virtual (to do) ATM
    virtual void setPins(); // public to call again when pins change?
    virtual void i2sInit();
    virtual void initDMABuffers(); // general? Physical S3 doesn't seem to do anything with dma ...

    // called by initDMABuffers
    virtual LedDriverDMABuffer *allocateDMABuffer(int bytes);
    virtual void putdefaultlatch(uint16_t *buffer);
    virtual void putdefaultones(uint16_t *buffer);

    void transpose16x1_noinline2(unsigned char *A, uint16_t *B); // will be used by all boards, see loadAndTranspose and transposeAll
public:

    //used by setBrightness and setColorCorrection, but 1024 bytes of extra data!!!
    //made public as other drivers may use this (e.g. MoonLight Art-Net)    
    uint8_t __green_map[256];
    uint8_t __blue_map[256];
    uint8_t __red_map[256];
    uint8_t __white_map[256];

    //initialize the leds array, pins, ledsPerPin, number of pins and the color arrangement of LEDs
    //color arrangements: supports RGB and RGBW but also exotic setups like LED Curtains where some have 6 channels per LEDS where only 3 channels are used.
    void initLeds(uint8_t *leds, PinConfig *pinConfig, size_t numPins, uint8_t channelsPerLed = 3, uint8_t offsetRed = 1, uint8_t offsetGreen = 0, uint8_t offsetBlue = 2, uint8_t offsetWhite = UINT8_MAX);
    bool initLedsDone = false;

    //setChannels can be called runtime to change the settings
    void setChannels(uint8_t channelsPerLed = 3, uint8_t offsetRed = 1, uint8_t offsetGreen = 0, uint8_t offsetBlue = 2, uint8_t offsetWhite = UINT8_MAX);

    void setBrightness(uint8_t brightness);
    uint8_t getBrightness();

    void setColorCorrection(uint8_t red, uint8_t green, uint8_t blue, uint8_t white = UINT8_MAX);
    void getColorCorrection(uint8_t &red, uint8_t &green, uint8_t &blue, uint8_t &white);

    //sends leds array to physical LEDs
    virtual void show();

    //sets RGB(W) values of a LED
    void setPixel(uint16_t ledNr, uint8_t red, uint8_t green, uint8_t blue, uint8_t white = UINT8_MAX);

    //public so also static functions using cont can use it...
    volatile bool isDisplaying = false; // used in i2sStart and i2sStop, in showPixel, show, flush ready
    volatile bool isWaiting = false;
};


// define subclasses for specific driver configs 
// not all might be needed
// specifics for 16x16 panels in separate class? (e.g. snake, width, height)

// See also README for class structure

//specifics for Physical Drivers!! (all boards)
class PhysicalDriver: virtual public LedsDriver { //virtual class !
    // no specifics yet
    //initDMABuffers and allocateDMABuffer is done by derived classes
    //not true: LedsDriver (for all boards, physical and virtual ...)
};

typedef struct
{
    int div_num;
    int div_a;
    int div_b;
} clock_speed;

static clock_speed clock_1123KHZ = {4, 20, 9}; //{4, 20, 9};
static clock_speed clock_1111KHZ = {4, 2, 1};
static clock_speed clock_1000KHZ = {5, 1, 0};
static clock_speed clock_800KHZ = {6, 4, 1};

//specifics for Virtual Drivers!! (all boards)
class VirtualDriver: virtual public LedsDriver { //virtual class !
protected:
    uint8_t latchPin = UINT8_MAX; //46 for S3, 27 for ESP32-D0 (wrover)
    uint8_t clockPin = UINT8_MAX; //3 for S3, 26 for ESP32-D0 (wrover)
    clock_speed clockSpeed = clock_800KHZ; //🔥 only for virtual? 
    uint8_t dmaBuffer = 30;
    uint8_t NUM_VIRT_PINS = 7;
    uint8_t __NB_DMA_BUFFER = 20; //for S3, 16 for D0?
    uint8_t _DMA_EXTENSTION = 0;
    LedDriverDMABuffer **DMABuffersTampon; //[__NB_DMA_BUFFER + 2];

    void startDriver(); // to do, both for D0 and S3 - and P4?

    void initDMABuffersVirtual(); //Virtual does things slightly different with DMA e.g. use LedDriverDMABuffer ** instead of LedDriverDMABuffer *

    LedDriverDMABuffer *allocateDMABufferVirtual(int bytes);

    void putdefaultonesVirtual(uint16_t *buffer);
public:
    void setLatchAndClockPin(uint8_t latchPin, uint8_t clockPin);
    void setClockSpeed( clock_speed clockSpeed);
};

//Lines used by D0 and S3 (loadAndTranspose and transposeAll, secondPixel)
typedef union {
    uint8_t bytes[16];
    uint32_t shorts[8];
    uint32_t raw[2];
} Lines;

#ifdef CONFIG_IDF_TARGET_ESP32
    //specific for ESP32 D0 devices!! (Physical and Virtual)
    #include "soc/i2s_struct.h" // for i2s_dev_t
    #include "soc/i2s_reg.h" // for I2S_IN_RST_M etc

    class LedsDriverESP32D0: virtual public LedsDriver { //virtual class !
    protected:
        const int deviceBaseIndex[2] = {I2S0O_DATA_OUT0_IDX, I2S1O_DATA_OUT0_IDX};
        void setPinsD0();
        
        i2s_dev_t *i2s;
        void IRAM_ATTR i2sReset(); // for physical and virtual D0
        void i2sReset_DMA();
        void i2sReset_FIFO();

        volatile xSemaphoreHandle LedDriver_sem = NULL;
        volatile xSemaphoreHandle LedDriver_semSync = NULL;
        volatile xSemaphoreHandle LedDriver_semDisp = NULL;
        volatile xSemaphoreHandle LedDriver_waitDisp = NULL;

        int interruptSource;
        int i2s_base_pin_index;
        void i2sInitD0();

        intr_handle_t _gI2SClocklessDriver_intr_handle; //i2sInit / esp_intr_alloc, start and stop in i2sStart / esp_intr_enable and i2sStop / esp_intr_disable
        volatile bool __enableDriver = true; //checked in LedDriverinterruptHandler and showPixels (Yves: but never set too false!!!)
        volatile bool framesync = false; //set true by i2sStart, toggled by LedDriverinterruptHandler
        volatile bool wasWaitingtofinish = false; //set by showPixels and waitDisplay, checked by i2sStop
        volatile bool transpose = false; //set true by showPixels, checked by LedDriverinterruptHandler and loadAndTranspose
        volatile int ledToDisplay; // showPixels, loadAndTranspose and transposeAll, LedDriverinterruptHandler
        //DMABuffers used by loadAndTranspose, LedDriverinterruptHandler, also by PhysicalDriverESP32D0::initDMABuffers!!! Check if moved to here
        LedDriverDMABuffer *DMABuffersTampon[4]; // an array of pointers!!!
        LedDriverDMABuffer **DMABuffersTransposed = NULL; // pointer to pointer
        volatile int dmaBufferActive = 0; //used by loadAndTranspose, LedDriverinterruptHandler
        static void IRAM_ATTR i2sStop(LedsDriverESP32D0 *cont); // used by LedDriverinterruptHandler
        static void IRAM_ATTR loadAndTranspose(LedsDriverESP32D0 *driver); //ewowi: driver as paramater is a nice trick to use class in static function!
        static void IRAM_ATTR LedDriverinterruptHandler(void *arg); //ewowi: esp_intr_alloc requires static
    };

    enum displayMode {
        NO_WAIT,
        WAIT,
        LOOP,
        LOOP_INTERUPT,
    };

    //https://github.com/hpwit/I2SClocklessLedDriver
    class PhysicalDriverESP32D0: public PhysicalDriver, public LedsDriverESP32D0 {

        void startDriver() override; // commented most atm
        void setPins() override; // calls setPinsD0 , nothing more

        void i2sInit() override; //i2sInitD0 + Physical specific (incl LedDriverinterruptHandler)

        void initDMABuffers() override; // allocateDMABuffer + putdefaultones
        LedDriverDMABuffer *allocateDMABuffer(int bytes); //Phys D0 specific
        void putdefaultones(uint16_t *buffer) override; //Phys D0 specific

        //used in show:
        displayMode __displayMode;
        void waitDisplay();
        void __showPixels();
        void i2sStart(LedDriverDMABuffer *startBuffer);
    public:
        void show() override; //showPixel in I2SClocklessLedDriver
    };

    //https://github.com/hpwit/I2SClocklessVirtualLedDriver (D0 and S3)
    class VirtualDriverESP32D0: public VirtualDriver, public LedsDriverESP32D0 {
    protected:
        const int deviceClockIndex[2] = {I2S0O_BCK_OUT_IDX, I2S1O_BCK_OUT_IDX};
        uint8_t DELTA_OFFSET_LATCH = 0;

        void setPins() override;

        void i2sInit() override; //i2sInitD0 + Virtual specific

        void initDMABuffers() override; //initDMABuffersVirtual
        LedDriverDMABuffer *allocateDMABuffer(int bytes); //allocateDMABufferVirtual + D0 specific
        void putdefaultlatch(uint16_t *buffer) override; // D0 specific
        void putdefaultones(uint16_t *buffer) override; // putdefaultonesVirtual + D0 specific
    };
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S3

    //specific for ESP32S3 devices!! (Physical and Virtual)
    class LedsDriverESP32S3: virtual public LedsDriver { //virtual class !
        //nothing yet
    };

    #include "esp_lcd_panel_io.h" // for esp_lcd_panel_io_handle_t etc

    //https://github.com/hpwit/I2SClockLessLedDriveresp32s3 (or FastLED!, taken from FastLED!)
    class PhysicalDriverESP32S3: public PhysicalDriver, public LedsDriverESP32S3 {

        uint16_t *buffers[2]; //containing the 2 led_output buffers, set in init, used in transpose and show
        int currentframe; //index in buffers, toggling between 0 and 1

        void setPins() override; // doing nothing ATM
        void i2sInit() override; // Physical specific, currently does I2SClocklessLedDriveresp32S3::initLed() 
        void initDMABuffers() override; // Physical specific, currently does I2SClocklessLedDriveresp32S3::__initLed()
        //allocateDMABuffer not needed here

        //used in show
        void transposeAll(uint16_t *ledoutput);
        //for initLed and show:
        esp_lcd_panel_io_handle_t led_io_handle = NULL; //set by init, used in show

    public:
        void show() override;
    };
    //https://github.com/hpwit/I2SClocklessVirtualLedDriver (D0 and S3)
    #include "esp_private/gdma.h" // gdma_channel_handle_t and gdma_channel_alloc_config_t
    class VirtualDriverESP32S3: public VirtualDriver, public LedsDriverESP32S3 {
        uint8_t signalsID[16] = {LCD_DATA_OUT0_IDX, LCD_DATA_OUT1_IDX, LCD_DATA_OUT2_IDX, LCD_DATA_OUT3_IDX, LCD_DATA_OUT4_IDX, LCD_DATA_OUT5_IDX, LCD_DATA_OUT6_IDX, LCD_DATA_OUT7_IDX, LCD_DATA_OUT8_IDX, LCD_DATA_OUT9_IDX, LCD_DATA_OUT10_IDX, LCD_DATA_OUT11_IDX, LCD_DATA_OUT12_IDX, LCD_DATA_OUT13_IDX, LCD_DATA_OUT14_IDX, LCD_DATA_OUT15_IDX};
        void setPins() override;

        gdma_channel_handle_t dma_chan; //@yves, removed static
        static bool IRAM_ATTR LedDriverinterruptHandler(gdma_channel_handle_t dma_chan, gdma_event_data_t *event_data, void *user_data) {
            //to do
            return false;
        }
        void i2sInit() override; // Virtual specific

        void initDMABuffers() override; //initDMABuffersVirtual + S3 specifics
        LedDriverDMABuffer *allocateDMABuffer(int bytes); //allocateDMABufferVirtual + S3 specific
        void putdefaultlatch(uint16_t *buffer) override; // S3 specific
        void putdefaultones(uint16_t *buffer) override; //putdefaultonesVirtual + S3 specific
    };

#endif

#ifdef CONFIG_IDF_TARGET_ESP32P4
    //specific for ESP32P4 devices!! (Physical and Virtual)
    class LedsDriverESP32P4: virtual public LedsDriver { //virtual class !
    };

    class PhysicalDriverESP32P4: public PhysicalDriver, public LedsDriverESP32P4 {
        void setPins() override;
    };

    class VirtualDriverESP32P4: public VirtualDriver, public LedsDriverESP32P4 {
        void setPins() override;
    };
#endif
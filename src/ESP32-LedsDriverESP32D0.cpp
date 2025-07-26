/**
    @title     ESP32-LedsDriver
    @file      ESP32-LedsDriverESP32D0.cpp
    @repo      https://github.com/ewowi/ESP32-LedsDriver, submit changes to this file as PRs
    @Authors   https://github.com/ewowi/ESP32-LedsDriver/commits/main
    @Doc       https://github.com/ewowi/ESP32-LedsDriver/
    @Copyright © 2025 Yves BAZIN
    @license   The MIT License (MIT)
    @license   For non MIT usage, commercial licenses must be purchased. Contact us for more information.
**/

//in this .cpp only functions specific for D0, both Physical and Virtual: LedsDriverESP32D0, PhysicalDriverESP32D0 and VirtualDriverESP32D0

#ifdef CONFIG_IDF_TARGET_ESP32

#include "ESP32-LedsDriver.h"

#include "driver/gpio.h"
#include "soc/soc.h"
// #include "soc/io_mux_reg.h"
#include "hal/gpio_hal.h"
// #include "soc/gpio_periph.h"
#include "hal/gpio_ll.h"
#include "soc/gpio_struct.h"
#include "rom/gpio.h"

void LedsDriverESP32D0::setPinsD0() {
    ESP_LOGD(TAG, "nP:%d", numPins);
    for (int i = 0; i < numPins; i++) {
        PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pinConfig[i].gpio], PIN_FUNC_GPIO);
        gpio_set_direction((gpio_num_t)pinConfig[i].gpio, (gpio_mode_t)GPIO_MODE_DEF_OUTPUT);
        gpio_matrix_out(pinConfig[i].gpio, deviceBaseIndex[I2S_DEVICE] + i + 8, false, false);
    }
}

void LedsDriverESP32D0::i2sReset() {
    ESP_LOGD(TAG, "");
    const unsigned long lc_conf_reset_flags = I2S_IN_RST_M | I2S_OUT_RST_M | I2S_AHBM_RST_M | I2S_AHBM_FIFO_RST_M;
    (&I2S0)->lc_conf.val |= lc_conf_reset_flags;
    (&I2S0)->lc_conf.val &= ~lc_conf_reset_flags;
    const uint32_t conf_reset_flags = I2S_RX_RESET_M | I2S_RX_FIFO_RESET_M | I2S_TX_RESET_M | I2S_TX_FIFO_RESET_M;
    (&I2S0)->conf.val |= conf_reset_flags;
    (&I2S0)->conf.val &= ~conf_reset_flags;
}

void LedsDriverESP32D0::i2sReset_DMA() {
    ESP_LOGD(TAG, "");
    (&I2S0)->lc_conf.out_rst = 1;
    (&I2S0)->lc_conf.out_rst = 0;
}

void LedsDriverESP32D0::i2sReset_FIFO() {
    ESP_LOGD(TAG, "");
    (&I2S0)->conf.tx_fifo_reset = 1;
    (&I2S0)->conf.tx_fifo_reset = 0;
}

#include "esp_private/periph_ctrl.h" // for periph_module_enable (// #include "driver/periph_ctrl.h") deprecated
void LedsDriverESP32D0::i2sInitD0() {
    ESP_LOGD(TAG, "");
    if (I2S_DEVICE == 0) {
        i2s = &I2S0;
        periph_module_enable(PERIPH_I2S0_MODULE);
        interruptSource = ETS_I2S0_INTR_SOURCE;
        i2s_base_pin_index = I2S0O_DATA_OUT0_IDX;
    } else {
        i2s = &I2S1;
        periph_module_enable(PERIPH_I2S1_MODULE);
        interruptSource = ETS_I2S1_INTR_SOURCE;
        i2s_base_pin_index = I2S1O_DATA_OUT0_IDX;
    }

    i2sReset();
    i2sReset_DMA();
    i2sReset_FIFO();
    i2s->conf.tx_right_first = 0;

    // -- Set parallel mode
    i2s->conf2.val = 0;
    i2s->conf2.lcd_en = 1;
    i2s->conf2.lcd_tx_wrx2_en = 1; // 0 for 16 or 32 parallel output
    i2s->conf2.lcd_tx_sdx2_en = 0; // HN

    // -- Set up the clock rate and sampling
    i2s->sample_rate_conf.val = 0;
    i2s->sample_rate_conf.tx_bits_mod = 16; // Number of parallel bits/pins
    i2s->clkm_conf.val = 0;

    // specifics for Physical and Virtual dev, see i2sInitD0 of them

    // -- Create a semaphore to block execution until all the controllers are done

    if (LedDriver_sem == NULL)
        LedDriver_sem = xSemaphoreCreateBinary();
    if (LedDriver_semSync == NULL)
        LedDriver_semSync = xSemaphoreCreateBinary();
    if (LedDriver_semDisp == NULL)
        LedDriver_semDisp = xSemaphoreCreateBinary();
}

void LedsDriverESP32D0::i2sStop(LedsDriverESP32D0 *cont) {
    ESP_LOGD(TAG, "");

    esp_intr_disable(cont->_gI2SClocklessDriver_intr_handle);
    
    ets_delay_us(16);

    (&I2S0)->conf.tx_start = 0;
    while( (&I2S0)->conf.tx_start ==1) {}
    cont->i2sReset();
        
    cont->isDisplaying = false;

    if (cont->wasWaitingtofinish) {
        cont->wasWaitingtofinish = false;
        xSemaphoreGive( cont->LedDriver_waitDisp);
    }
}

void LedsDriverESP32D0::loadAndTranspose(LedsDriverESP32D0 *driver) {
    ESP_LOGD(TAG, "");
    //cont->leds, cont->num_strips, (uint16_t *)cont->DMABuffersTampon[cont->dmaBufferActive]->buffer, cont->ledToDisplay, cont->__green_map, cont->__red_map, cont->__blue_map, cont->__white_map, cont->nb_components, cont->p_g, cont->p_r, cont->p_b);
    Lines secondPixel[driver->channelsPerLed];
    uint16_t *buffer;
    if (driver->transpose)
        buffer=(uint16_t *)driver->DMABuffersTampon[driver->dmaBufferActive]->buffer;
    else
        buffer=(uint16_t *)driver->DMABuffersTransposed[driver->dmaBufferActive]->buffer;

    uint16_t led_tmp=driver->ledToDisplay;
    #ifdef __HARDWARE_MAP
        //led_tmp=driver->ledToDisplay*driver->numPins;
    #endif
    memset(secondPixel,0,sizeof(secondPixel));
    #ifdef _LEDMAPPING
        //#ifdef __SOFTWARE_MAP
            uint8_t *poli ;
        //#endif
    #else
        uint8_t *poli = driver->leds + driver->ledToDisplay * driver->channelsPerLed;
    #endif
    for (int i = 0; i < driver->numPins; i++) {

        if (driver->ledToDisplay < driver->pinConfig[i].nrOfLeds) {
            #ifdef _LEDMAPPING
                #ifdef __SOFTWARE_MAP
                    poli = driver->leds + driver->mapLed(led_tmp) * driver->channelsPerLed;
                #endif
                #ifdef __HARDWARE_MAP
                    poli = driver->leds + *(driver->_hmapoff);
                #endif
                #ifdef __HARDWARE_MAP_PROGMEM
                    poli = driver->leds + pgm_read_word_near(driver->_hmap + driver->_hmapoff);
                #endif
            #endif
            secondPixel[driver->offsetGreen].bytes[i] = driver->__green_map[*(poli + 1)];
            secondPixel[driver->offsetRed].bytes[i] = driver->__red_map[*(poli + 0)];
            secondPixel[driver->offsetBlue].bytes[i] =  driver->__blue_map[*(poli + 2)];
            if (driver->channelsPerLed > 3)
                secondPixel[driver->offsetWhite].bytes[i] = driver->__white_map[*(poli + 3)];
            #ifdef __HARDWARE_MAP
                driver->_hmapoff++;
            #endif
            #ifdef __HARDWARE_MAP_PROGMEM
                driver->_hmapoff++;
            #endif
        }
        #ifdef _LEDMAPPING
            #ifdef __SOFTWARE_MAP
                led_tmp+=driver->pinConfig[i].nrOfLeds;
            #endif
        #else
            poli += driver->pinConfig[i].nrOfLeds* driver->channelsPerLed;
        #endif
    }

    driver->transpose16x1_noinline2(secondPixel[0].bytes, (uint16_t *)buffer);
    driver->transpose16x1_noinline2(secondPixel[1].bytes, (uint16_t *)buffer + 3 * 8);
    driver->transpose16x1_noinline2(secondPixel[2].bytes, (uint16_t *)buffer + 2 * 3 * 8);
    if (driver->channelsPerLed > 3)
        driver->transpose16x1_noinline2(secondPixel[3].bytes, (uint16_t *)buffer + 3 * 3 * 8);
}

void LedsDriverESP32D0::LedDriverinterruptHandler(void *arg) {
    #ifdef DO_NOT_USE_INTERUPT
        REG_WRITE(I2S_INT_CLR_REG(0), (REG_READ(I2S_INT_RAW_REG(0)) & 0xffffffc0) | 0x3f);
        return;
    #else
        LedsDriverESP32D0 *cont = (LedsDriverESP32D0 *)arg;

        if (!cont->__enableDriver) {
            REG_WRITE(I2S_INT_CLR_REG(0), (REG_READ(I2S_INT_RAW_REG(0)) & 0xffffffc0) | 0x3f);
            // cont->i2sStop();
            i2sStop(cont);
            return;
        }

        if (GET_PERI_REG_BITS(I2S_INT_ST_REG(I2S_DEVICE), I2S_OUT_EOF_INT_ST_S, I2S_OUT_EOF_INT_ST_S)) {
            cont->framesync = !cont->framesync;

            if (cont->transpose) {
                // cont->ledToDisplay++; //warning: '++' expression of 'volatile'-qualified type is deprecated 
                cont->ledToDisplay += 1; //ewowi: okay, compiler happy ;-)
                if (cont->ledToDisplay < cont->maxNrOfLedsPerPin) {

                    loadAndTranspose(cont);
            
                    if (cont->ledToDisplay == cont->maxNrOfLedsPerPin - 3) { //here it's not -1 because it takes time top have the change into account and it reread the buufer
                        cont->DMABuffersTampon[cont->dmaBufferActive]->descriptor.qe.stqe_next = &(cont->DMABuffersTampon[3]->descriptor);
                    }
                    cont->dmaBufferActive = (cont->dmaBufferActive + 1) % 2;
                }
            }
            else {
                if (cont->framesync) {
                    portBASE_TYPE HPTaskAwoken = 0;
                    xSemaphoreGiveFromISR(cont->LedDriver_semSync, &HPTaskAwoken);
                    if (HPTaskAwoken == pdTRUE)
                        portYIELD_FROM_ISR();
                }
            }
        }

        if (GET_PERI_REG_BITS(I2S_INT_ST_REG(I2S_DEVICE), I2S_OUT_TOTAL_EOF_INT_ST_S, I2S_OUT_TOTAL_EOF_INT_ST_S)) {           
            // cont->i2sStop();
            i2sStop(cont);
            if (cont->isWaiting) {
                portBASE_TYPE HPTaskAwoken = 0;
                xSemaphoreGiveFromISR(cont->LedDriver_sem, &HPTaskAwoken);
                if (HPTaskAwoken == pdTRUE)
                    portYIELD_FROM_ISR();
            }
        }
        REG_WRITE(I2S_INT_CLR_REG(0), (REG_READ(I2S_INT_RAW_REG(0)) & 0xffffffc0) | 0x3f);
    #endif
}

void PhysicalDriverESP32D0::startDriver() {
    ESP_LOGD(TAG, "");
    // from void __initled(uint8_t *leds, int *Pinsq, int num_strips, int num_led_per_strip)

    _gammab = 1;
    _gammar = 1;
    _gammag = 1;
    _gammaw = 1;
    // startleds = 0; //ewowi: not used
    // this->leds = leds; //ewowi: done by initLeds
    // this->saveleds = leds; //ewowi: saveleds doesn't add anything, always same as leds
    // this->num_led_per_strip = num_led_per_strip; //ewowi: done by initLeds, using maxNrOfLedsPerPin

    //ewowi: comment for the moment, used in showPixels / #ifdef ENABLE_HARDWARE_SCROLL loadAndTranspose, implement that first

    // _offsetDisplay.offsetx = 0;
    // _offsetDisplay.offsety = 0;
    // _offsetDisplay.panel_width = num_led_per_strip;
    // _offsetDisplay.panel_height = 9999;
    // _defaultOffsetDisplay = _offsetDisplay;
    // linewidth = num_led_per_strip;
    // this->num_strips = num_strips;
    // this->dmaBufferCount = dmaBufferCount; // ewowi: not used for anything...

    // ESP_LOGD(TAG,"xdelay:%d",__delay); //ewowi: calculated live in waitDisplay (to do here)
    #if HARDWARESPRITES == 1
        //Serial.println(NUM_LEDS_PER_STRIP * NBIS2SERIALPINS * 8);
        target = (uint16_t *)malloc(maxNrOfLedsPerPin * numPins * 2 + 2);
    #endif

    #ifdef __HARDWARE_MAP
        #ifndef __NON_HEAP
        _hmap=(uint16_t *)malloc(  total_leds * 2);
        #endif
        if(!_hmap) {
            ESP_LOGE(TAG,"no memory for the hamp");
            return;
        }
        else {
            ESP_LOGE(TAG,"trying to map");
            /*
            for(int leddisp=0;leddisp<maxNrOfLedsPerPin;leddisp++)
            {
                for (int i = 0; i < numPins; i++)
                {
                    _hmap[i+leddisp*numPins]=mapLed(leddisp+i*maxNrOfLedsPerPin)*channelsPerLed;
                }
            }
            */
            //int offset=0;
            createhardwareMap();
        }
    #endif
    setBrightness(255);
    /*
    dmaBufferCount = 2;
    this->leds = leds;
    this->saveleds = leds;
    this->num_led_per_strip = num_led_per_strip;
    _offsetDisplay.offsetx = 0;
    _offsetDisplay.offsety = 0;
    _offsetDisplay.panel_width = num_led_per_strip;
    _offsetDisplay.panel_height = 9999;
    _defaultOffsetDisplay = _offsetDisplay;
    linewidth = num_led_per_strip;
    this->num_strips = num_strips;
    this->dmaBufferCount = dmaBufferCount;*/    
}

LedDriverDMABuffer *PhysicalDriverESP32D0::allocateDMABuffer(int bytes) {
    ESP_LOGD(TAG, "b:%d", bytes);
    LedDriverDMABuffer *b = (LedDriverDMABuffer *)heap_caps_malloc(sizeof(LedDriverDMABuffer), MALLOC_CAP_DMA);

    if (!b) {
        ESP_LOGE(TAG, "No more memory\n");
        return NULL;
    }

    b->buffer = (uint8_t *)heap_caps_malloc(bytes, MALLOC_CAP_DMA);

    if (!b->buffer) {
        ESP_LOGE(TAG, "No more memory\n");
        return NULL;
    }
    memset(b->buffer, 0, bytes);

    b->descriptor.length = bytes;
    b->descriptor.size = bytes;
    b->descriptor.owner = 1;
    b->descriptor.sosf = 1;
    b->descriptor.buf = b->buffer;
    b->descriptor.offset = 0;
    b->descriptor.empty = 0;
    b->descriptor.eof = 1;
    b->descriptor.qe.stqe_next = 0;

    return b;
}

void PhysicalDriverESP32D0::setPins() {
    ESP_LOGD(TAG, "");
    setPinsD0();
}

void PhysicalDriverESP32D0::i2sInit() {
    ESP_LOGD(TAG, "");
    i2sInitD0();

    //Physical specific. @Yves, check PhysicalDriverESP32D0::i2sInit as they look pretty similar. Combine or parametrize?

    i2s->clkm_conf.clka_en = 0;

    //add the capability of going a bit faster
    i2s->clkm_conf.clkm_div_a = 3;    // CLOCK_DIVIDER_A;
    i2s->clkm_conf.clkm_div_b = 1;    //CLOCK_DIVIDER_B;
    i2s->clkm_conf.clkm_div_num = 33; //CLOCK_DIVIDER_N;

    i2s->fifo_conf.val = 0;
    i2s->fifo_conf.tx_fifo_mod_force_en = 1;
    i2s->fifo_conf.tx_fifo_mod = 1;  // 16-bit single channel data
    i2s->fifo_conf.tx_data_num = 32; //32; // fifo length
    i2s->fifo_conf.dscr_en = 1;      // fifo will use dma
    i2s->sample_rate_conf.tx_bck_div_num = 1;
    i2s->conf1.val = 0;
    i2s->conf1.tx_stop_en = 0;
    i2s->conf1.tx_pcm_bypass = 1;

    i2s->conf_chan.val = 0;
    i2s->conf_chan.tx_chan_mod = 1; // Mono mode, with tx_msb_right = 1, everything goes to right-channel

    i2s->timing.val = 0;
    i2s->int_ena.val = 0;
    /*
    // -- Allocate i2s interrupt
    SET_PERI_REG_BITS(I2S_INT_ENA_REG(I2S_DEVICE), I2S_OUT_EOF_INT_ENA_V,1, I2S_OUT_EOF_INT_ENA_S);
    SET_PERI_REG_BITS(I2S_INT_ENA_REG(I2S_DEVICE), I2S_OUT_TOTAL_EOF_INT_ENA_V, 1, I2S_OUT_TOTAL_EOF_INT_ENA_S);
    SET_PERI_REG_BITS(I2S_INT_ENA_REG(I2S_DEVICE), I2S_OUT_TOTAL_EOF_INT_ENA_V, 1, I2S_OUT_TOTAL_EOF_INT_ENA_S);
    */
    esp_err_t e = esp_intr_alloc(interruptSource, ESP_INTR_FLAG_INTRDISABLED | ESP_INTR_FLAG_LEVEL3 | ESP_INTR_FLAG_IRAM, &LedDriverinterruptHandler, this, &_gI2SClocklessDriver_intr_handle);
}

void PhysicalDriverESP32D0::initDMABuffers() {
    ESP_LOGD(TAG, "cPL:%d", channelsPerLed);
    DMABuffersTampon[0] = allocateDMABuffer(channelsPerLed * 8 * 2 * 3); //the buffers for the
    DMABuffersTampon[1] = allocateDMABuffer(channelsPerLed * 8 * 2 * 3);
    DMABuffersTampon[2] = allocateDMABuffer(channelsPerLed * 8 * 2 * 3);
    DMABuffersTampon[3] = allocateDMABuffer(channelsPerLed * 8 * 2 * 3 * 4);

    putdefaultones((uint16_t *)DMABuffersTampon[0]->buffer);
    putdefaultones((uint16_t *)DMABuffersTampon[1]->buffer);
}

void PhysicalDriverESP32D0::putdefaultones(uint16_t *buffer) {
    ESP_LOGD(TAG, "");
    /*order to push the data to the pins
    0:D7
    1:1
    2:1
    3:0
    4:0
    5:D6
    6:D5
    7:1
    8:1
    9:0
    10:0
    11:D4
    12:D3
    13:1
    14:1
    15:0
    16:0
    17:D2
    18:D1
    19:1
    20:1
    21:0
    22:0
    23:D0
    */
    for (int i = 0; i < channelsPerLed * 8 / 2; i++) {
        buffer[i * 6 + 1] = 0xffff;
        buffer[i * 6 + 2] = 0xffff;
    }
}

void PhysicalDriverESP32D0::waitDisplay() {
    if (isDisplaying) {
        wasWaitingtofinish = true;
        ESP_LOGD(TAG, "already displaying... wait");
        if(LedDriver_waitDisp==NULL) {
            LedDriver_waitDisp = xSemaphoreCreateCounting(10,0);
        }

        uint32_t __delay = (((maxNrOfLedsPerPin * 125 * 8 * channelsPerLed) /100000) + 1);
        const TickType_t xDelay = __delay ;
        xSemaphoreTake(LedDriver_waitDisp, xDelay);   
    }
    isDisplaying=true;
}

void PhysicalDriverESP32D0::i2sStart(LedDriverDMABuffer *startBuffer) {
    ESP_LOGD(TAG, "");

    i2sReset();
    framesync = false;

    // counti = 0; //ewowi: looks like doing nothing

    (&I2S0)->lc_conf.val = I2S_OUT_DATA_BURST_EN | I2S_OUTDSCR_BURST_EN | I2S_OUT_DATA_BURST_EN;

    (&I2S0)->out_link.addr = (uint32_t) & (startBuffer->descriptor);

    (&I2S0)->out_link.start = 1;

    (&I2S0)->int_clr.val = (&I2S0)->int_raw.val;

    (&I2S0)->int_clr.val = (&I2S0)->int_raw.val;
    (&I2S0)->int_ena.val = 0;

    /*
        If we do not use the regular showpixels, then no need to activate the interupt at the end of each pixels
        */
    //if(transpose)
    (&I2S0)->int_ena.out_eof = 1;

    (&I2S0)->int_ena.out_total_eof = 1;
    esp_intr_enable(_gI2SClocklessDriver_intr_handle);

    //We start the I2S
    (&I2S0)->conf.tx_start = 1;

    //Set the mode to indicate that we've started
    isDisplaying = true;
}

void PhysicalDriverESP32D0::__showPixels() {

    if (!__enableDriver) {
        return;
    }
    #ifdef __HARDWARE_MAP
        _hmapoff=_hmap;
    #endif
    #ifdef __HARDWARE_MAP_HARDWARE
        _hmapoff=0;
    #endif

    if (leds == NULL) {
        ESP_LOGE(TAG, "no leds buffer defined");
        return;
    }

    ledToDisplay = 0;
    transpose = true;
    DMABuffersTampon[0]->descriptor.qe.stqe_next = &(DMABuffersTampon[1]->descriptor);
    DMABuffersTampon[1]->descriptor.qe.stqe_next = &(DMABuffersTampon[0]->descriptor);
    DMABuffersTampon[2]->descriptor.qe.stqe_next = &(DMABuffersTampon[0]->descriptor);
    DMABuffersTampon[3]->descriptor.qe.stqe_next = 0;
    dmaBufferActive = 0;

    loadAndTranspose(this);

    // __displayMode=dispmode;
    dmaBufferActive = 1;
    i2sStart(DMABuffersTampon[2]);
    isDisplaying=true;
    if (__displayMode == WAIT) {
        isWaiting = true;
        if (LedDriver_sem==NULL)
            LedDriver_sem=xSemaphoreCreateBinary();
        xSemaphoreTake(LedDriver_sem, portMAX_DELAY);
    } else {
        isWaiting = false;
        isDisplaying = true;
    }
}

void PhysicalDriverESP32D0::show() {
    if (!__enableDriver)
        return;
    waitDisplay();
    // leds=saveleds; //ewowi: looks not needed
    // _offsetDisplay=_defaultOffsetDisplay; //ewowi: not implemented yet
    __displayMode=WAIT; //ewowi: default for now

    // if NO_WAIT
    // [ 12383][D][ESP32-LedsDriverESP32D0.cpp:427] waitDisplay(): [🐸] already displaying... wait
    // [ 12395][D][ESP32-LedsDriverESP32D0.cpp:121] loadAndTranspose(): [🐸] 
    // [ 12402][D][ESP32-LedsDriverESP32D0.cpp:440] i2sStart(): [🐸] 
    // [ 12409][D][ESP32-LedsDriverESP32D0.cpp:37] i2sReset(): [🐸] 

    __showPixels();
}

void VirtualDriverESP32D0::setPins() {
    setPinsD0();

    if (latchPin == UINT8_MAX || clockPin == UINT8_MAX) {
        ESP_LOGE(TAG, "call setLatchAndClockPin() needed!");
        return;
    }

    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[latchPin], PIN_FUNC_GPIO);
    gpio_set_direction((gpio_num_t)latchPin, (gpio_mode_t)GPIO_MODE_DEF_OUTPUT);
    gpio_matrix_out(latchPin, deviceBaseIndex[I2S_DEVICE] + numPins + 8, false, false);
    gpio_set_direction((gpio_num_t)clockPin, (gpio_mode_t)GPIO_MODE_DEF_OUTPUT);
    gpio_matrix_out(clockPin, deviceClockIndex[I2S_DEVICE], false, false);
}

#include "soc/rtc.h" // rtc_clk_apll_enable
void VirtualDriverESP32D0::i2sInit() {
    i2sInitD0();

    // i2s->sample_rate_conf.tx_bck_div_num = 1;
    #ifdef __DL_CLK
        // Serial.println("norml clock");
        i2s->clkm_conf.clka_en = 0;
        // rtc_clk_apll_enable(true, 31, 133,7, 1); //19.2Mhz 7 pins +1 latchrtc_clk_apll_enable(true, 31, 133,7, 1); //19.2Mhz 7 pins +1 latch

        // -- Data clock is computed as Base/(div_num + (div_b/div_a))
        //    Base is 80Mhz, so 80/(3+ 7/6) = 19.2Mhz

        i2s->clkm_conf.clkm_div_a = 6;   // CLOCK_DIVIDER_A;
        i2s->clkm_conf.clkm_div_b = 7;   // CLOCK_DIVIDER_B;
        i2s->clkm_conf.clkm_div_num = 3; // CLOCK_DIVIDER_N;

    #else
        // Serial.println("precise clock");

    #ifndef _20_MHZ_CLK
    #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        rtc_clk_apll_enable(true);
        rtc_clk_apll_coeff_set(1, 31, 133, 7);
    #else
        rtc_clk_apll_enable(true, 31, 133, 7, 1); // 19.2Mhz 7 pins +1 latchrtc_clk_apll_enable(true, 31, 133,7, 1); //19.2Mhz 7 pins +1 latch
    #endif
    #else
    #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        rtc_clk_apll_enable(true);
        rtc_clk_apll_coeff_set(1, 0, 0, 8);
    #else
        rtc_clk_apll_enable(true, 0, 0, 8, 1); // 19.2Mhz 7 pins +1 latchrtc_clk_apll_enable(true, 31, 133,7, 1); //19.2Mhz 7 pins +1 latch
    #endif
        // rtc_clk_apll_enable(true, 0, 0, 8, 1);
    #endif
        i2s->clkm_conf.clka_en = 1;
        i2s->clkm_conf.clkm_div_a = 1;   // CLOCK_DIVIDER_A;
        i2s->clkm_conf.clkm_div_b = 0;   // CLOCK_DIVIDER_B;
        i2s->clkm_conf.clkm_div_num = 1; // CLOCK_DIVIDER_N;
    #endif

    i2s->fifo_conf.val = 0;
    i2s->fifo_conf.tx_fifo_mod_force_en = 1;
    i2s->fifo_conf.tx_fifo_mod = 1;  // 16-bit single channel data
    i2s->fifo_conf.tx_data_num = 32; // 32; // fifo length
    i2s->fifo_conf.dscr_en = 1;      // fifo will use dma

    i2s->sample_rate_conf.tx_bck_div_num = 1;
    i2s->conf1.val = 0;
    i2s->conf1.tx_stop_en = 0;
    i2s->conf1.tx_pcm_bypass = 1;

    i2s->conf_chan.val = 0;
    i2s->conf_chan.tx_chan_mod = 1; // Mono mode, with tx_msb_right = 1, everything goes to right-channel

    i2s->timing.val = 0;
}

void VirtualDriverESP32D0::initDMABuffers() {
    initDMABuffersVirtual(); // for all Virtual drivers, D0 and S3

    //nothing D0 specific (S3 has specific code)
}

LedDriverDMABuffer *VirtualDriverESP32D0::allocateDMABuffer(int bytes) {
    LedDriverDMABuffer * b = allocateDMABufferVirtual(bytes);

    //D0 specific
    b->descriptor.length = bytes;
    b->descriptor.size = bytes;
    b->descriptor.owner = 1;
    b->descriptor.sosf = 1;
    b->descriptor.buf = b->buffer;
    b->descriptor.offset = 0;
    b->descriptor.empty = 0;
    b->descriptor.eof = 1;
    b->descriptor.qe.stqe_next = 0;
    return b;
}

void VirtualDriverESP32D0::putdefaultlatch(uint16_t *buffer) {
    // printf("dd%d\n",NBIS2SERIALPINS);
    uint16_t mask1 = 1 << numPins;
    for (int i = 0; i < 24 * channelsPerLed; i++)
    {
        buffer[NUM_VIRT_PINS + i * (NUM_VIRT_PINS + 1) - 1 - 5 + DELTA_OFFSET_LATCH] = mask1; // 0x8000;
    }
}

void VirtualDriverESP32D0::putdefaultones(uint16_t *buffer) {
    putdefaultonesVirtual(buffer); // for all Virtual drivers, D0 and S3

    //virtual D0 specific: 
    uint16_t mas = 0xFFFF & (~(0xffff << (numPins)));

    for (int j = 0; j < 8 * channelsPerLed; j++) {
        buffer[1 + j * (3 * (NUM_VIRT_PINS + 1))] = 0xFFFF;
        buffer[0 + j * (3 * (NUM_VIRT_PINS + 1))] = mas;
    }
}

#endif //CONFIG_IDF_TARGET_ESP32
/**
    @title     ESP32-LedsDriver
    @file      ESP32-LedsDriverESP32dev.cpp
    @repo      https://github.com/ewowi/ESP32-LedsDriver, submit changes to this file as PRs
    @Authors   https://github.com/ewowi/ESP32-LedsDriver/commits/main
    @Doc       https://github.com/ewowi/ESP32-LedsDriver/
    @Copyright © 2025 Github ESP32-LedsDriver Commit Authors
    @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
    @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
**/

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

void LedsDriverESP32dev::setPinsDev() {
    for (int i = 0; i < numPins; i++) {
        PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pinConfig[i].gpio], PIN_FUNC_GPIO);
        gpio_set_direction((gpio_num_t)pinConfig[i].gpio, (gpio_mode_t)GPIO_MODE_DEF_OUTPUT);
        gpio_matrix_out(pinConfig[i].gpio, deviceBaseIndex[I2S_DEVICE] + i + 8, false, false);
    }
}

#include "esp_private/periph_ctrl.h" // for periph_module_enable (// #include "driver/periph_ctrl.h") deprecated
void LedsDriverESP32dev::i2sInitDev() {
    if (I2S_DEVICE == 0)
    {
        i2s = &I2S0;
        periph_module_enable(PERIPH_I2S0_MODULE);
        interruptSource = ETS_I2S0_INTR_SOURCE;
        i2s_base_pin_index = I2S0O_DATA_OUT0_IDX;
    }
    else
    {
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

    // specifics for Physical and Virtual dev, see i2sInitDev of them

    // -- Create a semaphore to block execution until all the controllers are done

    if (LedDriver_sem == NULL)
    {
        LedDriver_sem = xSemaphoreCreateBinary();
    }

    if (LedDriver_semSync == NULL)
    {
        LedDriver_semSync = xSemaphoreCreateBinary();
    }
    if (LedDriver_semDisp == NULL)
    {
        LedDriver_semDisp = xSemaphoreCreateBinary();
    }
}

LedDriverDMABuffer *PhysicalDriverESP32dev::allocateDMABuffer(int bytes) {
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

void PhysicalDriverESP32dev::setPins() {
    setPinsDev();
}

void PhysicalDriverESP32dev::i2sInit() {
    i2sInitDev();

    //Physical specific. @Yves, check PhysicalDriverESP32dev::i2sInit as they look pretty similar. Combine or parametrize?

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

void PhysicalDriverESP32dev::initDMABuffers() {
    DMABuffersTampon[0] = allocateDMABuffer(channelsPerLed * 8 * 2 * 3); //the buffers for the
    DMABuffersTampon[1] = allocateDMABuffer(channelsPerLed * 8 * 2 * 3);
    DMABuffersTampon[2] = allocateDMABuffer(channelsPerLed * 8 * 2 * 3);
    DMABuffersTampon[3] = allocateDMABuffer(channelsPerLed * 8 * 2 * 3 * 4);

    putdefaultones((uint16_t *)DMABuffersTampon[0]->buffer);
    putdefaultones((uint16_t *)DMABuffersTampon[1]->buffer);
}

void PhysicalDriverESP32dev::putdefaultones(uint16_t *buffer) {
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

void VirtualDriverESP32dev::setPins() {
    setPinsDev();

    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[latchPin], PIN_FUNC_GPIO);
    gpio_set_direction((gpio_num_t)latchPin, (gpio_mode_t)GPIO_MODE_DEF_OUTPUT);
    gpio_matrix_out(latchPin, deviceBaseIndex[I2S_DEVICE] + numPins + 8, false, false);
    gpio_set_direction((gpio_num_t)clockPin, (gpio_mode_t)GPIO_MODE_DEF_OUTPUT);
    gpio_matrix_out(clockPin, deviceClockIndex[I2S_DEVICE], false, false);
}

#include "soc/rtc.h" // rtc_clk_apll_enable
void VirtualDriverESP32dev::i2sInit() {
    i2sInitDev();

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

void VirtualDriverESP32dev::initDMABuffers() {
    initDMABuffersVirtual(); // for all Virtual drivers, S3 and non S3
}

LedDriverDMABuffer *VirtualDriverESP32dev::allocateDMABuffer(int bytes) {
    LedDriverDMABuffer * b = allocateDMABufferVirtual(bytes);

    //dev specific
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

void VirtualDriverESP32dev::putdefaultlatch(uint16_t *buffer) {
    // printf("dd%d\n",NBIS2SERIALPINS);
    uint16_t mask1 = 1 << numPins;
    for (int i = 0; i < 24 * channelsPerLed; i++)
    {
        buffer[NUM_VIRT_PINS + i * (NUM_VIRT_PINS + 1) - 1 - 5 + DELTA_OFFSET_LATCH] = mask1; // 0x8000;
    }
}

void VirtualDriverESP32dev::putdefaultones(uint16_t *buffer) {
    putdefaultonesVirtual(buffer); // for all Virtual drivers, S3 and non S3

    //virtual dev specific: 
    uint16_t mas = 0xFFFF & (~(0xffff << (numPins)));

    for (int j = 0; j < 8 * channelsPerLed; j++) {
        buffer[1 + j * (3 * (NUM_VIRT_PINS + 1))] = 0xFFFF;
        buffer[0 + j * (3 * (NUM_VIRT_PINS + 1))] = mas;
    }
}

#endif //CONFIG_IDF_TARGET_ESP32
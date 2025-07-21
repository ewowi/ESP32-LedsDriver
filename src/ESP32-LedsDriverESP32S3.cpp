/**
    @title     ESP32-LedsDriver
    @file      ESP32-LedsDriverESP32S3.cpp
    @repo      https://github.com/ewowi/ESP32-LedsDriver, submit changes to this file as PRs
    @Authors   https://github.com/ewowi/ESP32-LedsDriver/commits/main
    @Doc       https://github.com/ewowi/ESP32-LedsDriver/
    @Copyright © 2025 Yves BAZIN
    @license   The MIT License (MIT)
    @license   For non MIT usage, commercial licenses must be purchased. Contact us for more information.
**/

#ifdef CONFIG_IDF_TARGET_ESP32S3

#include "ESP32-LedsDriver.h"

#include "driver/gpio.h"
#include "soc/soc.h"
// #include "soc/io_mux_reg.h"
#include "hal/gpio_hal.h"
// #include "soc/gpio_periph.h"
#include "hal/gpio_ll.h"
#include "soc/gpio_struct.h"
#include "rom/gpio.h"

void PhysicalDriverESP32S3::setPins() {
    //nothing here for PhysicalDriverESP32S3: pins are set in initDMABuffers, ... need to investigate if it can be moved here
    //  also can pins be changed in runtime...
}

#define LCD_DRIVER_PSRAM_DATA_ALIGNMENT 64
#define __OFFSET 0 //  (24*3*2*2*2+2)
#define __OFFSET_END (24 * 3 * 2 * 2 * 2 + 2)

volatile xSemaphoreHandle I2SClocklessLedDriverS3_sem = NULL;

void PhysicalDriverESP32S3::i2sInit() {

    ESP_LOGD(TAG, "");

    uint16_t *led_output = NULL;
    uint16_t *led_output2 = NULL;

    //this comes from I2SClocklessLedDriveresp32S3::initLed() in original repo. put it here in i2sInit() for the moment

    currentframe = 0;
    _gammab = 1;
    _gammar = 1;
    _gammag = 1;
    _gammaw = 1;
    setBrightness(255);
    if (I2SClocklessLedDriverS3_sem == NULL) {
        I2SClocklessLedDriverS3_sem = xSemaphoreCreateBinary();
    }
    // esp_lcd_panel_io_handle_t init_lcd_driver(unsigned int
    // FASTLED_ESP32S3_I2S_CLOCK_HZ, size_t channelsPerLed) {
    led_output = (uint16_t *)heap_caps_aligned_alloc(
        LCD_DRIVER_PSRAM_DATA_ALIGNMENT,
        8 * channelsPerLed * maxNrOfLedsPerPin * 3 * 2 + __OFFSET +
            __OFFSET_END,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    memset(led_output, 0,
            8 * channelsPerLed * maxNrOfLedsPerPin * 3 * 2 + __OFFSET +
                __OFFSET_END);

    led_output2 = (uint16_t *)heap_caps_aligned_alloc(
        LCD_DRIVER_PSRAM_DATA_ALIGNMENT,
        8 * channelsPerLed * maxNrOfLedsPerPin * 3 * 2 + __OFFSET +
            __OFFSET_END,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    memset(led_output2, 0,
            8 * channelsPerLed * maxNrOfLedsPerPin * 3 * 2 + __OFFSET +
                __OFFSET_END);
    buffers[0] = led_output;
    buffers[1] = led_output2;
    // led_output[0] = 0xFFFF; //the +1 because it's like the first value
    // doesnt get pushed do not ask me why for now
    // led_output2[0] = 0xFFFF;
    led_output2 += __OFFSET / 2;
    led_output += __OFFSET / 2;

    for (int i = 0; i < maxNrOfLedsPerPin * channelsPerLed * 8; i++) {
        led_output[3 * i + 1] =
            0xFFFF; // the +1 because it's like the first value doesnt get
                    // pushed do not ask me why for now
        led_output2[3 * i + 1] = 0xFFFF;
    }
    // ledsbuff = leds;
    // _numstrips = numstrip;
    // num_leds_per_strip = maxNrOfLedsPerPin;
    // _initled(leds, pins, numstrip, NUM_LED_PER_STRIP); //we do this in initDMABuffers for the moment
}

// #include "esp_lcd_panel_io.h"

// Note: I2S REQUIRES that the FASTLED_OVERCLOCK factor is a a build-level-define and
// not an include level define. This is easy if you are already using PlatformIO or CMake.
// If you are using ArduinoIDE you'll have to download FastLED source code and hack the src
// to make this work.
#ifdef FASTLED_OVERCLOCK
#define FASTLED_ESP32S3_I2S_CLOCK_HZ_SCALE (FASTLED_OVERCLOCK)
#else
#define FASTLED_ESP32S3_I2S_CLOCK_HZ_SCALE (1)
#endif

#ifndef FASTLED_ESP32S3_I2S_CLOCK_HZ
#define FASTLED_ESP32S3_I2S_CLOCK_HZ uint32_t((24 * 100 * 1000)*FASTLED_ESP32S3_I2S_CLOCK_HZ_SCALE)
#endif

// #include "esp_lcd_panel_io.h" // for esp_lcd_panel_io_handle_t etc

// io_config.on_color_trans_done requires static function
// bool DRIVER_READY = true;
static bool IRAM_ATTR flush_ready(esp_lcd_panel_io_handle_t panel_io,
                                  esp_lcd_panel_io_event_data_t *edata,
                                  void *user_ctx) {

    PhysicalDriverESP32S3 *cont =
        (PhysicalDriverESP32S3 *)user_ctx;
    // cont->testcount++; //Yves, what is this doing ?: (looks like nothing)... is later used in ...
    // DRIVER_READY = true; //@yves: looks like doing nothing
    cont->isDisplaying = false;
    if (cont->isWaiting) {
        portBASE_TYPE HPTaskAwoken = 0;
        cont->isWaiting = false;
        xSemaphoreGiveFromISR(I2SClocklessLedDriverS3_sem, &HPTaskAwoken);
        if (HPTaskAwoken == pdTRUE)
            portYIELD_FROM_ISR(HPTaskAwoken);
    }
    return false;
}

#include "esp_err.h" // for ESP_ERROR_CHECK

void PhysicalDriverESP32S3::initDMABuffers() {

    ESP_LOGD(TAG, "");

    //this comes from I2SClocklessLedDriveresp32S3::__initLed() in original repo. put it here in initDMABuffers() for the moment

    // esp_lcd_panel_io_handle_t init_lcd_driver(unsigned int
    // FASTLED_ESP32S3_I2S_CLOCK_HZ, size_t channelsPerLed) {

    esp_lcd_i80_bus_handle_t i80_bus = NULL;

    esp_lcd_i80_bus_config_t bus_config;

    bus_config.clk_src = LCD_CLK_SRC_PLL160M;
    bus_config.dc_gpio_num = 0;
    bus_config.wr_gpio_num = 0;
    // bus_config.data_gpio_nums = (int*)malloc(16*sizeof(int));
    for (int i = 0; i < numPins; i++) {
        bus_config.data_gpio_nums[i] = pinConfig[i].gpio;
    }
    if (numPins < 16) {
        for (int i = numPins; i < 16; i++) {
            bus_config.data_gpio_nums[i] = 0;
        }
    }
    bus_config.bus_width = 16;
    bus_config.max_transfer_bytes =
    channelsPerLed * maxNrOfLedsPerPin * 8 * 3 * 2 + __OFFSET + __OFFSET_END;

    #if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 4, 0)
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    #endif

    // In IDF 5.3, psram_trans_align became deprecated. We kick the can down
    // the road a little bit and suppress the warning until idf 5.4 arrives.
    // In IDF < 5.4, psram_trans_align and sram_trans_align are required.
    //ewowi: we also need it in 5.4 deprecated or not ... this was needed to get lights burning !
    bus_config.psram_trans_align = LCD_DRIVER_PSRAM_DATA_ALIGNMENT; //@yves, DEPRECATED...
    bus_config.sram_trans_align = 4; //@yves: DEPRECATED

    #if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 4, 0)
        #pragma GCC diagnostic pop
    #endif

    ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus));

    esp_lcd_panel_io_i80_config_t io_config;

    io_config.cs_gpio_num = -1;
    io_config.pclk_hz = FASTLED_ESP32S3_I2S_CLOCK_HZ;
    io_config.trans_queue_depth = 1;
    io_config.dc_levels = {
        .dc_idle_level = 0,
        .dc_cmd_level = 0,
        .dc_dummy_level = 0,
        .dc_data_level = 1,
    };
    //.on_color_trans_done = flush_ready,
    // .user_ctx = nullptr,
    io_config.lcd_cmd_bits = 0;
    io_config.lcd_param_bits = 0;
    io_config.user_ctx = this;

    io_config.on_color_trans_done = flush_ready;
    ESP_ERROR_CHECK(
        esp_lcd_new_panel_io_i80(i80_bus, &io_config, &led_io_handle));
}

void PhysicalDriverESP32S3::transposeAll(uint16_t *ledoutput) {

    uint16_t ledToDisplay = 0;
    Lines secondPixel[channelsPerLed];
    uint16_t *buff =
        ledoutput + 2; //+1 pour le premier empty +1 pour le 1 systématique
    uint16_t jump = maxNrOfLedsPerPin * channelsPerLed;
    for (int j = 0; j < maxNrOfLedsPerPin; j++) {
        uint8_t *poli = leds + ledToDisplay * channelsPerLed; //@yves: leds was ledsbuff, why as it is same as leds *?
        for (int i = 0; i < numPins; i++) {

            secondPixel[offsetGreen].bytes[i] = __green_map[*(poli + 1)];
            secondPixel[offsetRed].bytes[i] = __red_map[*(poli + 0)];
            secondPixel[offsetBlue].bytes[i] = __blue_map[*(poli + 2)];
            if (channelsPerLed > 3)
                secondPixel[offsetWhite].bytes[i] = __white_map[*(poli + 3)];
            // #endif
            poli += jump;
        }
        ledToDisplay++;
        transpose16x1_noinline2(secondPixel[0].bytes, buff);
        buff += 24;
        transpose16x1_noinline2(secondPixel[1].bytes, buff);
        buff += 24;
        transpose16x1_noinline2(secondPixel[2].bytes, buff);
        buff += 24;
        if (channelsPerLed > 3) {
            transpose16x1_noinline2(secondPixel[3].bytes, buff);
            buff += 24;
        }
    }
}

// According to bug reports, this driver does not work well with the new WS2812-v5b. This is
// probably due to the extrrra long reset time requirements of this chipset. so we put in
// a hack that will always add 300 uS to the reset time.
#define FASTLED_EXPERIMENTAL_YVES_EXTRA_WAIT_MICROS 300

void PhysicalDriverESP32S3::show() {
    transposeAll(buffers[currentframe]);
    if (isDisplaying) {
        // Serial.println("on display dejà");
        isWaiting = true;
        if (I2SClocklessLedDriverS3_sem == NULL)
            I2SClocklessLedDriverS3_sem = xSemaphoreCreateBinary();
        xSemaphoreTake(I2SClocklessLedDriverS3_sem, portMAX_DELAY);
    }
    isDisplaying = true;

    if (FASTLED_EXPERIMENTAL_YVES_EXTRA_WAIT_MICROS) {
        delayMicroseconds(FASTLED_EXPERIMENTAL_YVES_EXTRA_WAIT_MICROS);
    }

    // @yves, I got an error on led_io_handle->tx_color saying pointer or reference to incomplete type "esp_lcd_panel_io_t" is not allowed 
    // AI gave me esp_lcd_panel_io_tx_color, save to use?
    esp_lcd_panel_io_tx_color(led_io_handle, 0x2C, buffers[currentframe],
                              channelsPerLed * maxNrOfLedsPerPin * 8 * 3 *
                                  2 +
                              __OFFSET + __OFFSET_END);

    currentframe = (currentframe + 1) % 2;
}

void VirtualDriverESP32S3::setPins() {

    if (latchPin == UINT8_MAX || clockPin == UINT8_MAX) {
        ESP_LOGE(TAG, "call setLatchAndClockPin() needed!");
        return;
    }

    for (int i = 0; i < numPins; i++) {
        esp_rom_gpio_connect_out_signal(pinConfig[i].gpio, signalsID[i], false, false);
        gpio_hal_iomux_func_sel(GPIO_PIN_MUX_REG[pinConfig[i].gpio], PIN_FUNC_GPIO);
        gpio_set_drive_capability((gpio_num_t)pinConfig[i].gpio, (gpio_drive_cap_t)3);
    }
    esp_rom_gpio_connect_out_signal(latchPin, signalsID[numPins], false, false);
    gpio_hal_iomux_func_sel(GPIO_PIN_MUX_REG[latchPin], PIN_FUNC_GPIO);
    gpio_set_drive_capability((gpio_num_t)latchPin, (gpio_drive_cap_t)3);

    esp_rom_gpio_connect_out_signal(clockPin, LCD_PCLK_IDX, false, false);
    gpio_hal_iomux_func_sel(GPIO_PIN_MUX_REG[clockPin], PIN_FUNC_GPIO);
    gpio_set_drive_capability((gpio_num_t)clockPin, (gpio_drive_cap_t)3);
}

#include "esp_private/periph_ctrl.h" // for periph_module_enable (// #include "driver/periph_ctrl.h") deprecated
#include <soc/lcd_cam_struct.h> // for LCD_CAM
void VirtualDriverESP32S3::i2sInit() {
    periph_module_enable(PERIPH_LCD_CAM_MODULE);
    periph_module_reset(PERIPH_LCD_CAM_MODULE);

    // Reset LCD bus
    LCD_CAM.lcd_user.lcd_reset = 1;
    esp_rom_delay_us(100);

    LCD_CAM.lcd_clock.clk_en = 1;                             // Enable peripheral clock
    LCD_CAM.lcd_clock.lcd_clk_sel = 2;                        // XTAL_CLK source
    LCD_CAM.lcd_clock.lcd_ck_out_edge = 0;                    // PCLK low in 1st half cycle
    LCD_CAM.lcd_clock.lcd_ck_idle_edge = 0;                   // PCLK low idle
    LCD_CAM.lcd_clock.lcd_clk_equ_sysclk = 0;                 // PCLK = CLK / (CLKCNT_N+1)
    LCD_CAM.lcd_clock.lcd_clkm_div_num = clockSpeed.div_num; // 1st stage 1:250 divide
    LCD_CAM.lcd_clock.lcd_clkm_div_a = clockSpeed.div_a;     // 0/1 fractional divide
    LCD_CAM.lcd_clock.lcd_clkm_div_b = clockSpeed.div_b;
    LCD_CAM.lcd_clock.lcd_clkcnt_n = 1; //

    LCD_CAM.lcd_ctrl.lcd_rgb_mode_en = 0;    // i8080 mode (not RGB)
    LCD_CAM.lcd_rgb_yuv.lcd_conv_bypass = 0; // Disable RGB/YUV converter
    LCD_CAM.lcd_misc.lcd_next_frame_en = 0;  // Do NOT auto-frame
    LCD_CAM.lcd_data_dout_mode.val = 0;      // No data delays
    LCD_CAM.lcd_user.lcd_always_out_en = 1;  // Enable 'always out' mode
    LCD_CAM.lcd_user.lcd_8bits_order = 0;    // Do not swap bytes
    LCD_CAM.lcd_user.lcd_bit_order = 0;      // Do not reverse bit order
    LCD_CAM.lcd_user.lcd_byte_order = 0;
    LCD_CAM.lcd_user.lcd_2byte_en = 1;       // 8-bit data mode
    LCD_CAM.lcd_user.lcd_dummy = 0;          // Dummy phase(s) @ LCD start
    LCD_CAM.lcd_user.lcd_dummy_cyclelen = 0; // 1 dummy phase
    LCD_CAM.lcd_user.lcd_cmd = 0;            // No command at LCD start
    LCD_CAM.lcd_misc.lcd_bk_en = 1;
    // -- Create a semaphore to block execution until all the controllers are done
    gdma_channel_alloc_config_t dma_chan_config = {
        .sibling_chan = NULL,
        .direction = GDMA_CHANNEL_DIRECTION_TX,
        .flags = {
            .reserve_sibling = 0}};
    gdma_new_ahb_channel(&dma_chan_config, &dma_chan); // @yves, gdma_new_channel deprecated so I replaced this (gdma_new_axi_channel() - for AXI DMA channels (only on chips that support it))
    gdma_connect(dma_chan, GDMA_MAKE_TRIGGER(GDMA_TRIG_PERIPH_LCD, 0));
    gdma_strategy_config_t strategy_config = {
        .owner_check = false,
        .auto_update_desc = false};
    gdma_apply_strategy(dma_chan, &strategy_config);
    /*
    gdma_transfer_ability_t ability = {
        .psram_trans_align = 64,
        //.sram_trans_align = 64,
    };
    gdma_set_transfer_ability(dma_chan, &ability);
*/
    // Enable DMA transfer callback
    gdma_tx_event_callbacks_t tx_cbs = {
        .on_trans_eof = LedDriverinterruptHandler
        };
    gdma_register_tx_event_callbacks(dma_chan, &tx_cbs, this);
    // esp_intr_disable((*dma_chan).intr);
    LCD_CAM.lcd_user.lcd_start = 0;
}

void VirtualDriverESP32S3::initDMABuffers() {
    initDMABuffersVirtual(); // for all Virtual drivers, used by non S3 and S3

    // S3 specific
    for (int buff_num = 0; buff_num < __NB_DMA_BUFFER - 1; buff_num++) {

        DMABuffersTampon[buff_num]->next = DMABuffersTampon[buff_num + 1];
    }

    DMABuffersTampon[__NB_DMA_BUFFER - 1]->next = DMABuffersTampon[0];
    DMABuffersTampon[__NB_DMA_BUFFER]->next = DMABuffersTampon[0];
    // memset(DMABuffersTampon[__NB_DMA_BUFFER]->buffer,0,WS2812_DMA_DESCRIPTOR_BUFFER_MAX_SIZE);
    // memset(DMABuffersTampon[__NB_DMA_BUFFER+1]->buffer,0,WS2812_DMA_DESCRIPTOR_BUFFER_MAX_SIZE);
    DMABuffersTampon[__NB_DMA_BUFFER + 1]->next = NULL;
    DMABuffersTampon[__NB_DMA_BUFFER]->dw0.suc_eof = 0;
}

LedDriverDMABuffer *VirtualDriverESP32S3::allocateDMABuffer(int bytes) {
    LedDriverDMABuffer *b = allocateDMABufferVirtual(bytes);

    //S3 specific
    b->dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_DMA;
    b->dw0.size = bytes;
    b->dw0.length = bytes;
    b->dw0.suc_eof = 1;
    return b;
}

void VirtualDriverESP32S3::putdefaultlatch(uint16_t *buffer) {
    // printf("dd%d\n",NBIS2SERIALPINS);
    uint16_t mask1 = 1 << numPins;
    for (int i = 0; i < 24 * channelsPerLed; i++)
    {
        buffer[i * (NUM_VIRT_PINS + 1)] = mask1;
        // buff[NUM_VIRT_PINS+i*(NUM_VIRT_PINS+1)]=0x02;
    }
}

void VirtualDriverESP32S3::putdefaultones(uint16_t *buffer) {
    putdefaultonesVirtual(buffer); // for all Virtual drivers, S3 and non S3

    //virtual S3 specific:
    uint16_t mas = 0xFFFF & (~(0xffff << (numPins)));
    // printf("mas%d\n",mas);
    for (int j = 0; j < 8 * channelsPerLed; j++) {
        buffer[0 + j * (3 * (NUM_VIRT_PINS + 1))] = 0xFFFF;
        buffer[1 + j * (3 * (NUM_VIRT_PINS + 1))] = mas;
    }
}

#endif //CONFIG_IDF_TARGET_ESP32S3
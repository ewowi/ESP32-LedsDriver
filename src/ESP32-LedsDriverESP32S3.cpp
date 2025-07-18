/**
    @title     ESP32-LedsDriver
    @file      ESP32-LedsDriverESP32S3.cpp
    @repo      https://github.com/ewowi/ESP32-LedsDriver, submit changes to this file as PRs
    @Authors   https://github.com/ewowi/ESP32-LedsDriver/commits/main
    @Doc       https://github.com/ewowi/ESP32-LedsDriver/
    @Copyright © 2025 Github ESP32-LedsDriver Commit Authors
    @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
    @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
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
    // using bus_config and io_config ...
    // for (int i = 0; i < numstrip; i++) {
    //     bus_config.data_gpio_nums[i] = pins[i];
    // }
    // if (numstrip < 16) {
    //     for (int i = numstrip; i < 16; i++) {
    //         bus_config.data_gpio_nums[i] = 0;
    //     }
    // }
}

void VirtualDriverESP32S3::setPins() {
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
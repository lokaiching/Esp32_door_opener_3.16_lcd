#include "esp_lcd_st7701.h"
#include "esp_lcd_panel_io_additions.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h" // Added to ensure esp_lcd_rgb_panel_event_callbacks_t is defined
#include "esp_lcd_types.h"
#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"
#include "../user_config.h"
#include "lcd_init.h"

#define EXAMPLE_LCD_BIT_PER_PIXEL 16

static bool example_on_bounce_frame_finish_event(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx) //刷新完成触发LVGL刷新新一帧
{
  BaseType_t high_task_awoken = pdFALSE;
  xSemaphoreGiveFromISR(flush_done_semaphore, &high_task_awoken);
  return high_task_awoken == pdTRUE;
}

static const st7701_lcd_init_cmd_t lcd_init_cmds[] = 
{
  //   cmd   data        data_size  delay_ms 1
  {0xFF,(uint8_t []){0x77,0x01,0x00,0x00,0x13},5,0},
  {0xEF,(uint8_t []){0x08},1,0},
  {0xFF,(uint8_t []){0x77,0x01,0x00,0x00,0x10},5,0},
  {0xC0,(uint8_t []){0xE5,0x02},2,0},
  {0xC1,(uint8_t []){0x15,0x0A},2,0},
  {0xC2,(uint8_t []){0x07,0x02},2,0},
  {0xCC,(uint8_t []){0x10},1,0},
  {0xB0,(uint8_t []){0x00,0x08,0x51,0x0D,0xCE,0x06,0x00,0x08,0x08,0x24,0x05,0xD0,0x0F,0x6F,0x36,0x1F},16,0},
  {0xB1,(uint8_t []){0x00,0x10,0x4F,0x0C,0x11,0x05,0x00,0x07,0x07,0x18,0x02,0xD3,0x11,0x6E,0x34,0x1F},16,0},
  {0xFF,(uint8_t []){0x77,0x01,0x00,0x00,0x11},5,0},
  {0xB0,(uint8_t []){0x4D},1,0},
  {0xB1,(uint8_t []){0x37},1,0},
  {0xB2,(uint8_t []){0x87},1,0},
  {0xB3,(uint8_t []){0x80},1,0},
  {0xB5,(uint8_t []){0x4A},1,0},
  {0xB7,(uint8_t []){0x85},1,0},
  {0xB8,(uint8_t []){0x21},1,0},
  {0xB9,(uint8_t []){0x00,0x13},2,0},
  {0xC0,(uint8_t []){0x09},1,0},
  {0xC1,(uint8_t []){0x78},1,0},
  {0xC2,(uint8_t []){0x78},1,0},
  {0xD0,(uint8_t []){0x88},1,0},
  {0xE0,(uint8_t []){0x80,0x00,0x02},3,100},
  {0xE1,(uint8_t []){0x0F,0xA0,0x00,0x00,0x10,0xA0,0x00,0x00,0x00,0x60,0x60},11,0},
  {0xE2,(uint8_t []){0x30,0x30,0x60,0x60,0x45,0xA0,0x00,0x00,0x46,0xA0,0x00,0x00,0x00},13,0},
  {0xE3,(uint8_t []){0x00,0x00,0x33,0x33},4,0},
  {0xE4,(uint8_t []){0x44,0x44},2,0},
  {0xE5,(uint8_t []){0x0F,0x4A,0xA0,0xA0,0x11,0x4A,0xA0,0xA0,0x13,0x4A,0xA0,0xA0,0x15,0x4A,0xA0,0xA0},16,0},
  {0xE6,(uint8_t []){0x00,0x00,0x33,0x33},4,0},
  {0xE7,(uint8_t []){0x44,0x44},2,0},
  {0xE8,(uint8_t []){0x10,0x4A,0xA0,0xA0,0x12,0x4A,0xA0,0xA0,0x14,0x4A,0xA0,0xA0,0x16,0x4A,0xA0,0xA0},16,0},
  {0xEB,(uint8_t []){0x02,0x00,0x4E,0x4E,0xEE,0x44,0x00},7,0},
  {0xED,(uint8_t []){0xFF,0xFF,0x04,0x56,0x72,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x27,0x65,0x40,0xFF,0xFF},16,0},
  {0xEF,(uint8_t []){0x08,0x08,0x08,0x40,0x3F,0x64},6,0},
  {0xFF,(uint8_t []){0x77,0x01,0x00,0x00,0x13},5,0},
  {0xE8,(uint8_t []){0x00,0x0E},2,0},
  {0xFF,(uint8_t []){0x77,0x01,0x00,0x00,0x00},5,0},
  {0x11,(uint8_t []){0x00},0,120},
  {0xFF,(uint8_t []){0x77,0x01,0x00,0x00,0x13},5,0},
  {0xE8,(uint8_t []){0x00,0x0C},2,10},
  {0xE8,(uint8_t []){0x00,0x00},2,0},
  {0xFF,(uint8_t []){0x77,0x01,0x00,0x00,0x00},5,0},
  {0x3A,(uint8_t []){0x55},1,0},
  {0x36,(uint8_t []){0x00},1,0},
  {0x35,(uint8_t []){0x00},1,0},
  {0x29,(uint8_t []){0x00},0,20},

};


void lcd_init(esp_lcd_panel_handle_t *panel_handle){


      spi_line_config_t line_config =
  {
    .cs_io_type = IO_TYPE_GPIO,             // Set to `IO_TYPE_GPIO` if using GPIO, same to below
    .cs_gpio_num = EXAMPLE_LCD_IO_SPI_CS,
    .scl_io_type = IO_TYPE_GPIO,
    .scl_gpio_num = EXAMPLE_LCD_IO_SPI_SCK,
    .sda_io_type = IO_TYPE_GPIO,
    .sda_gpio_num = EXAMPLE_LCD_IO_SPI_SDO,
    .io_expander = NULL,                        // Set to NULL if not using IO expander
  };
  esp_lcd_panel_io_3wire_spi_config_t io_config = ST7701_PANEL_IO_3WIRE_SPI_CONFIG(line_config, 0);
  esp_lcd_panel_io_handle_t io_handle = NULL;
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_3wire_spi(&io_config, &io_handle));

  esp_lcd_rgb_panel_config_t rgb_config = 
  {
    .clk_src = LCD_CLK_SRC_DEFAULT,
    .psram_trans_align = 64,
    .bounce_buffer_size_px = 10 * EXAMPLE_LCD_H_RES,
    .num_fbs = 2,
    .data_width = 16,
    .bits_per_pixel = 16,
    .de_gpio_num = EXAMPLE_LCD_IO_RGB_DE,
    .pclk_gpio_num = EXAMPLE_LCD_IO_RGB_PCLK,
    .vsync_gpio_num = EXAMPLE_LCD_IO_RGB_VSYNC,
    .hsync_gpio_num = EXAMPLE_LCD_IO_RGB_HSYNC,
    .flags.fb_in_psram = true,
    .disp_gpio_num = -1,
    .data_gpio_nums = 
    { //BGR
      EXAMPLE_LCD_IO_RGB_B0,
      EXAMPLE_LCD_IO_RGB_B1,
      EXAMPLE_LCD_IO_RGB_B2,
      EXAMPLE_LCD_IO_RGB_B3,
      EXAMPLE_LCD_IO_RGB_B4,
      EXAMPLE_LCD_IO_RGB_G0,
      EXAMPLE_LCD_IO_RGB_G1,
      EXAMPLE_LCD_IO_RGB_G2,
      EXAMPLE_LCD_IO_RGB_G3,
      EXAMPLE_LCD_IO_RGB_G4,
      EXAMPLE_LCD_IO_RGB_G5,
      EXAMPLE_LCD_IO_RGB_R0,
      EXAMPLE_LCD_IO_RGB_R1,
      EXAMPLE_LCD_IO_RGB_R2,
      EXAMPLE_LCD_IO_RGB_R3,
      EXAMPLE_LCD_IO_RGB_R4,
    },
    .timings = 
    {
      .pclk_hz = 18 * 1000 * 1000,
      .h_res = EXAMPLE_LCD_H_RES,
      .v_res = EXAMPLE_LCD_V_RES,
      .hsync_back_porch = 30,
      .hsync_front_porch = 30,  //30
      .hsync_pulse_width = 6,
      .vsync_back_porch = 20,   //10-100 40
      .vsync_front_porch = 20,  //10-100 70
      .vsync_pulse_width = 40,
    },
  };
  
  
  st7701_vendor_config_t vendor_config = 
  {
    .rgb_config = &rgb_config,
    .init_cmds = lcd_init_cmds,      // Uncomment these line if use custom initialization commands
    .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(st7701_lcd_init_cmd_t),
    .flags = 
    {
      .mirror_by_cmd = 1,       // Only work when `enable_io_multiplex` is set to 0
      .enable_io_multiplex = 0, /**
                                 * Set to 1 if panel IO is no longer needed after LCD initialization.
                                 * If the panel IO pins are sharing other pins of the RGB interface to save GPIOs,
                                 * Please set it to 1 to release the pins.
                                */
    },
  };
  
  const esp_lcd_panel_dev_config_t panel_config = 
  {
    .reset_gpio_num = EXAMPLE_LCD_IO_RGB_RESET,     // Set to -1 if not use
    .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,     // Implemented by LCD command `36h`
    .bits_per_pixel = EXAMPLE_LCD_BIT_PER_PIXEL,    // Implemented by LCD command `3Ah` (16/18/24)
    .vendor_config = &vendor_config,
  };         
  
  // Create the st7701 panel driver and pannel handler
  ESP_ERROR_CHECK(esp_lcd_new_panel_st7701(io_handle, &panel_config, panel_handle));   
  
  esp_lcd_rgb_panel_event_callbacks_t cbs = {
    .on_bounce_frame_finish = example_on_bounce_frame_finish_event,
  };
  
  ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(*panel_handle, &cbs, NULL));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(*panel_handle));                                   // Only reset RGB when `enable_io_multiplex` is set to 1, or reset st7701 meanwhile
  ESP_ERROR_CHECK(esp_lcd_panel_init(*panel_handle));                                    // Only initialize RGB when `enable_io_multiplex` is set to 1, or initialize st7701 meanwhile

}
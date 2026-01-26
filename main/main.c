#include <dirent.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_err.h"

#include "esp_timer.h"
#include "esp_event.h"

#include "lvgl.h"

#include "driver/gpio.h"
#include "user_config.h"

// my library
#include "serial_init/qr_reader.h"

#include "door_controller/door_controller.h"
#include "door_controller/door_controller_ui.h"


#include "esp_wifi_bsp.h"
#include "sdcard_bsp.h"

SemaphoreHandle_t *lvgl_mux = NULL;
SemaphoreHandle_t *flush_done_semaphore = NULL;

void app_main(void)
{

  esp_event_loop_create_default();
  qr_reader_init();
  espwifi_init();
  _sdcard_init();
  

  lvgl_mux = xSemaphoreCreateMutex();
  assert(lvgl_mux);
  flush_done_semaphore = xSemaphoreCreateBinary();
  assert(flush_done_semaphore);
  
  esp_lcd_panel_handle_t panel_handle = NULL;
  lcd_init(&panel_handle); //初始化LCD

  lvgl_init(panel_handle, lvgl_mux, flush_done_semaphore);

  if (example_lvgl_lock(-1))
  {
    //lv_demo_music();
    door_controller_ui_init();
    //lv_demo_widgets();      /* A widgets example */
    //lv_demo_music();        /* A modern, smartphone-like music player demo. */
    // lv_demo_stress();      /* A stress test for LVGL. */
    //lv_demo_benchmark();    /* A demo to measure the performance of LVGL or to compare different settings. */
    // Release the mutex
    example_lvgl_unlock();
  }

  door_controller_init();
}


#ifndef LVGL_INIT_H
#define LVGL_INIT_H

#include "lvgl.h"
#include "esp_lcd_panel_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

void lvgl_init(esp_lcd_panel_handle_t panel_handle, SemaphoreHandle_t *lvgl_mutex, SemaphoreHandle_t *flush_done_semaphore);

#ifdef __cplusplus
}
#endif

#endif // LVGL_INIT_H

#ifndef LVGL_INIT_H
#define LVGL_INIT_H

#include "lvgl.h"
#include "esp_lcd_panel_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"



#ifdef __cplusplus
extern "C" {
#endif

extern SemaphoreHandle_t lvgl_mux;


void lvgl_init();
bool example_lvgl_lock(int wait_ms);
void example_lvgl_unlock(void);

#ifdef __cplusplus
}
#endif

#endif // LVGL_INIT_H

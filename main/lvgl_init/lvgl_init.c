#include "lvgl_init.h"
#include "../user_config.h"
#include "../drv_init/lcd_init.h"


#include "esp_log.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include <assert.h>
#include <stdlib.h>

#define EXAMPLE_LCD_BIT_PER_PIXEL 16
#define BYTES_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565))
#define BUFF_SIZE (EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * BYTES_PER_PIXEL)

SemaphoreHandle_t *lvgl_mux = NULL;
SemaphoreHandle_t *flush_done_semaphore = NULL;
uint8_t *lvgl_dest = NULL;

static void example_increase_lvgl_tick(void *arg)
{
  /* Tell LVGL how many milliseconds has elapsed */
  lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

static void example_lvgl_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * color_p)
{
  esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);
#ifdef EXAMPLE_Rotate_90
  lv_display_rotation_t rotation = lv_display_get_rotation(disp);
  lv_area_t rotated_area;
  if(rotation != LV_DISPLAY_ROTATION_0)
  {
    lv_color_format_t cf = lv_display_get_color_format(disp);
    /*Calculate the position of the rotated area*/
    rotated_area = *area;
    lv_display_rotate_area(disp, &rotated_area);
    /*Calculate the source stride (bytes in a line) from the width of the area*/
    uint32_t src_stride = lv_draw_buf_width_to_stride(lv_area_get_width(area), cf);
    /*Calculate the stride of the destination (rotated) area too*/
    uint32_t dest_stride = lv_draw_buf_width_to_stride(lv_area_get_width(&rotated_area), cf);
    /*Have a buffer to store the rotated area and perform the rotation*/
    
    int32_t src_w = lv_area_get_width(area);
    int32_t src_h = lv_area_get_height(area);
    lv_draw_sw_rotate(color_p, lvgl_dest, src_w, src_h, src_stride, dest_stride, rotation, cf);
    /*Use the rotated area and rotated buffer from now on*/
    area = &rotated_area;
  }
  esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2+1, area->y2+1, lvgl_dest);
#else
  esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2+1, area->y2+1, color_p);
#endif
}

void lvgl_flush_wait_cb(lv_display_t * disp) //等待RGB发送数据完成,使用lvgl_flush_wait_cb 不需要再使用lv_disp_flush_ready(disp);
{
  xSemaphoreTake(flush_done_semaphore, portMAX_DELAY);
}

static bool example_lvgl_lock(int timeout_ms)
{
  assert(lvgl_mux && "bsp_display_start must be called first");

  const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
  return xSemaphoreTake(lvgl_mux, timeout_ticks) == pdTRUE;
}

static void example_lvgl_unlock(void)
{
  assert(lvgl_mux && "bsp_display_start must be called first");
  xSemaphoreGive(lvgl_mux);
}

static void example_lvgl_port_task(void *arg)
{
  uint32_t task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
  for(;;)
  {
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (example_lvgl_lock(-1))
    {
      task_delay_ms = lv_timer_handler();
      // Release the mutex
      example_lvgl_unlock();
    }
    if (task_delay_ms > EXAMPLE_LVGL_TASK_MAX_DELAY_MS)
    {
      task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
    }
    else if (task_delay_ms < EXAMPLE_LVGL_TASK_MIN_DELAY_MS)
    {
      task_delay_ms = EXAMPLE_LVGL_TASK_MIN_DELAY_MS;
    }
    vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
  }
}

void lvgl_port_init(esp_lcd_panel_handle_t *panel_handle, SemaphoreHandle_t *lvgl_mutex, SemaphoreHandle_t *lcd_flush_semaphore)
{
  // Create a mutex to protect LVGL calls
  lvgl_mux = lvgl_mutex;
  assert(lvgl_mux && "lvgl_mux create failed");
  flush_done_semaphore = lcd_flush_semaphore;
  assert(flush_done_semaphore && "flush_done_semaphore create failed");

    lv_init();
  lv_display_t * disp = lv_display_create(EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES); /* 以水平和垂直分辨率（像素）进行基本初始化 */
  lv_display_set_flush_cb(disp, example_lvgl_flush_cb);                          /* 设置刷新回调函数以绘制到显示屏 */
  lv_display_set_flush_wait_cb(disp,lvgl_flush_wait_cb);
  uint8_t *buf_1 = NULL;
  uint8_t *buf_2 = NULL;
  buf_1 = (uint8_t *)heap_caps_malloc(BUFF_SIZE, MALLOC_CAP_SPIRAM);
  buf_2 = (uint8_t *)heap_caps_malloc(BUFF_SIZE, MALLOC_CAP_SPIRAM);
  lv_display_set_buffers(disp, buf_1, buf_2, BUFF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_user_data(disp, panel_handle);
#ifdef EXAMPLE_Rotate_90
  lvgl_dest = (uint8_t *)heap_caps_malloc(BUFF_SIZE, MALLOC_CAP_SPIRAM); //旋转buf
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);
#endif
  const esp_timer_create_args_t lvgl_tick_timer_args = 
  {
    .callback = &example_increase_lvgl_tick,
    .name = "lvgl_tick"
  };
  esp_timer_handle_t lvgl_tick_timer = NULL;
  ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

  xTaskCreatePinnedToCore(example_lvgl_port_task, "LVGL", EXAMPLE_LVGL_TASK_STACK_SIZE, NULL, EXAMPLE_LVGL_TASK_PRIORITY, NULL,1); 

}

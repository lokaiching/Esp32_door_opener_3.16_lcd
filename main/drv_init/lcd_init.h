#ifndef LCD_INIT_H
#define LCD_INIT_H

#include "esp_lcd_types.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the LCD panel and related hardware.
 *
 * @param[out] panel_handle Pointer to the panel handle to be initialized.
 */
void lcd_init(esp_lcd_panel_handle_t *panel_handle, SemaphoreHandle_t *lcd_flush_semaphore);

#ifdef __cplusplus
}
#endif

#endif // LCD_INIT_H

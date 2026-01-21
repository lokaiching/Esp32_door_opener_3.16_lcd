idf_component_register(
    SRCS "main.c" 
    INCLUDE_DIRS "."
    REQUIRES 
        lcd_bl_pwm_bsp 
        esp_wifi_bsp 
        sdcard_bsp
    PRIV_REQUIRES esp_timer lvgl lv_demos
    )

idf_component_register(
    SRCS "main.c" 
         "door_controller/door_controller_ui.c" 
         "door_controller/door_controller.c" 
    INCLUDE_DIRS "." "door_controller"
    REQUIRES 
        lcd_bl_pwm_bsp 
        esp_wifi_bsp 
        sdcard_bsp
    PRIV_REQUIRES esp_timer lvgl 
    )

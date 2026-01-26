idf_component_register(
    SRCS "main.c" 
        "serial_init/qr_reader.c"
        "drv_init/lcd_init.c"
        "lvgl_init/lvgl_init.c"
        "fonts/door_opener_text_26.c"
        "door_controller/door_controller_ui.c" 
        "door_controller/door_controller.c"
        "door_controller/door_controller_http.c" 

    INCLUDE_DIRS 
        "." 
        "door_controller" 
        "fonts" 
        "lvgl_init" 
        "drv_init" 
        "serial_init"
    REQUIRES 
        lcd_bl_pwm_bsp 
        esp_wifi_bsp 
        sdcard_bsp
        esp_event
        esp_http_client
        json
        fatfs
    PRIV_REQUIRES esp_timer lvgl 
    )


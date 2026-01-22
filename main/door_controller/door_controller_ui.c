
#include "door_controller_ui.h"
#include "../fonts/door_opener_text_26.h"
#include "sdcard_bsp.h"
#include "esp_wifi_bsp.h"

#include "lvgl.h"
#include <string.h>

#define FONT_DEFAULT &door_opener_text_26

// Assume these are defined/created elsewhere and set by your main UI init
extern lv_obj_t* parent;
extern lv_obj_t* label;
extern lv_obj_t* status_bar;
extern lv_obj_t* img;
extern lv_obj_t* logo;

lv_obj_t* parent = NULL;
lv_obj_t* label = NULL;
lv_obj_t* status_bar = NULL;
lv_obj_t* img = NULL;
lv_obj_t* logo = NULL;

static volatile wifi_status_msg_t current_wifi_status = WIFI_STATUS_DISCONNECTED;
static volatile sdcard_status_msg_t current_sdcard_status = SDCARD_INIT_FAILED;
static lv_timer_t *status_check_timer = NULL;

static void status_check_timer_cb(lv_timer_t* timer) {
    // Check WiFi status
    if (wifi_status != current_wifi_status) {
        current_wifi_status = wifi_status;
        if (wifi_status == WIFI_STATUS_CONNECTED) {
            lv_label_set_text(status_bar, "");
        } else {
            lv_label_set_text(status_bar, "WiFi Disconnected");
        }
    }

    // Check SD card status
    if (sdcard_status != current_sdcard_status) {
        current_sdcard_status = sdcard_status;
        if (sdcard_status == SDCARD_INIT_SUCCESS) {
            lv_label_set_text(status_bar, "");
        } else {
            lv_label_set_text(status_bar, "SD Card Error");
        }
    }
}

#define MARQUEE_SPEED 3 // pixels per frame
static int marquee_x = 0;
static char marquee_text[128] = "掃 QR Code 開門~";
static lv_timer_t* marquee_timer = NULL;

static void marquee_timer_cb(lv_timer_t* timer) {
    int label_w = lv_obj_get_width(label);
    int parent_w = lv_obj_get_width(parent);
    marquee_x -= MARQUEE_SPEED;
    if (-marquee_x > label_w) {
        marquee_x = parent_w;
    }
    lv_obj_set_x(label, marquee_x);
}



void door_controller_ui_update_status(DoorStatus status) {
    char img_source[64];
    switch (status) {
        case IDLE:
            strcpy(marquee_text, "掃 QR Code 開門~");
            strcpy(img_source, "/sdcard/idle.jpg");
            bg_fading_color_change(label, lv_color_black());
            break;
        case LOADING:
            strcpy(marquee_text, "處理中 ^3^");
            strcpy(img_source, "/sdcard/loading.jpg");
            bg_fading_color_change(label, lv_color_hex(0x2962FF)); // Blue
            break;
        case SUCCESS:
            strcpy(marquee_text, "門已開啟 ~");
            strcpy(img_source, "/sdcard/success.jpg");
            bg_fading_color_change(label, lv_color_hex(0x00796B)); // Orange
            break;
        case EQUIP_ERR:
            strcpy(marquee_text, "設備錯誤");
            strcpy(img_source, "/sdcard/equip_err.jpg");
            bg_fading_color_change(label, lv_color_hex(0xFF0266));
            break;
        case INUSE:
            strcpy(marquee_text, "使用中");
            strcpy(img_source, "/sdcard/inuse.jpg");
            bg_fading_color_change(label, lv_color_hex(0xFF0266));
            break;
        case DEPOSIT_ERR:
            strcpy(marquee_text, "押金超過限制");
            strcpy(img_source, "/sdcard/deposit_err.jpg");
            bg_fading_color_change(label, lv_color_hex(0xFF0266)); 
            break;
        case CODE_EXPIRE:
            strcpy(marquee_text, "驗證碼已過期");
            strcpy(img_source, "/sdcard/code_expire.jpg");
            bg_fading_color_change(label, lv_color_hex(0xFF0266)); 
            break;
        case USING_ANOTHER_SHELF:
            strcpy(marquee_text, "上一個櫃門未關好");
            strcpy(img_source, "/sdcard/using_another_shelf.jpg");
            bg_fading_color_change(label, lv_color_hex(0xFF0266)); 
            break;
        case CHECKED_OUT:
            strcpy(marquee_text, "已結帳, 無法使用");
            strcpy(img_source, "/sdcard/checked_out.jpg");
            bg_fading_color_change(label, lv_color_hex(0xFF0266)); 
            break;
        default:
            strcpy(marquee_text, "搞唔掂, 請聯絡職員");
            strcpy(img_source, "/sdcard/idle.jpg");
            bg_fading_color_change(label, lv_color_hex(0xFF0266)); 
            break;
    }

    lv_label_set_text(label, marquee_text);
    marquee_x = lv_obj_get_width(parent); // reset to right edge
    lv_img_set_src(img, img_source);
}

/* label background color fading effect */
typedef struct {
        lv_color_t start;
        lv_color_t end;
} color_pair_t;

void bg_fading_color_change(lv_obj_t *obj,  lv_color_t end){
    
    lv_color_t start = lv_obj_get_style_bg_color(obj, 0);
    color_pair_t cp = {
        .start = start,
        .end = end
    };

    static lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, 0, 255);
    lv_anim_set_user_data(&a, &cp);
    lv_anim_set_time(&a, 1000);
    lv_anim_set_exec_cb(
        &a, 
        [](void *var, int32_t v) {
        lv_obj_t *obj = (lv_obj_t *)var;
        color_pair_t *p = (color_pair_t *)lv_anim_get_user_data(&a);  // ← important
        lv_color_t mixed = lv_color_mix(p->end, p->start, (uint8_t)v);
        lv_obj_set_style_text_color(obj, mixed, 0);
    });

    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}

void door_controller_ui_init(void) {
    //create parent container
    parent = lv_screen_active();
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    //Create Logo
    logo = lv_image_create(parent);
    lv_image_set_src(logo, "/sdcard/logo.jpg"); // Path to JPEG on SD card
    lv_obj_set_size(logo, 320, 80);
    lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 0); // Center the image

    // Create label
    label = lv_label_create(parent);
    lv_label_set_text(label, marquee_text);
    lv_obj_set_style_text_color(label, lv_color_white(), 0); // Set text color to white
    lv_obj_set_style_text_font(label, FONT_DEFAULT, 0); 
    lv_obj_set_width(label, lv_obj_get_width(parent));
    lv_obj_set_height(label, lv_font_get_line_height(FONT_DEFAULT) + 10);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL);
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 0); 
    marquee_x = lv_obj_get_width(parent);
    if (marquee_timer == NULL) {
        marquee_timer = lv_timer_create(marquee_timer_cb, 30, NULL); // 30 ms per frame
    }

    //Create Status Bar
    status_bar = lv_label_create(parent);
    lv_label_set_text(status_bar, ""); // Initially empty
    lv_obj_set_style_text_color(status_bar, lv_color_hex(0xFF0000), 0); // Red color for errors
    lv_obj_set_style_text_font(status_bar, FONT_DEFAULT, 0);
    lv_obj_align(status_bar, LV_ALIGN_TOP_MID, 0, 85); // Position below logo
    if (status_check_timer == NULL) {
        status_check_timer = lv_timer_create(status_check_timer_cb, 1000, NULL); // Check every second
    }

    // Create image
    img = lv_img_create(parent);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0); // Position it below center

    // Optionally: Initialize with IDLE status after a delay or event
    // door_controller_ui_update_status(IDLE);
}
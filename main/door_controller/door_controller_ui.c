#include <dirent.h>
#include "esp_log.h"
#include "door_controller_ui.h"
#include "../fonts/door_opener_text_26.h"
#include "sdcard_bsp.h"
#include "esp_wifi_bsp.h"

#include "lvgl.h"
#include <string.h>

#define FONT_DEFAULT &door_opener_text_26
#define USE_GIF_IMG 0

// Assume these are defined/created elsewhere and set by your main UI init
extern lv_obj_t* parent;
extern lv_obj_t* label;
extern lv_obj_t* status_bar;
extern lv_obj_t* gif_img;
extern lv_obj_t* fix_img;
extern lv_obj_t* logo;

lv_obj_t* parent = NULL;
lv_obj_t* label_bg = NULL; // background box for label
lv_obj_t* label = NULL;
lv_obj_t* status_bar = NULL;
lv_obj_t* gif_img = NULL;
lv_obj_t* fix_img = NULL;
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

/* scrolling text */
#define MARQUEE_SPEED 3 // pixels per frame
static int marquee_x = 0;
static char marquee_text[128] = "掃 QR Code 開門~";
static lv_timer_t* marquee_timer = NULL;

static void marquee_timer_cb(lv_timer_t* timer) {
    int label_w = lv_obj_get_width(label_bg);
    int parent_w = lv_obj_get_width(parent);
    marquee_x -= MARQUEE_SPEED;
    if (-marquee_x > label_w) {
        marquee_x = parent_w;
    }
    lv_obj_set_x(label, marquee_x);
}

/* label background color fading effect */
static lv_anim_t label_anim;
typedef struct {
        lv_color_t start;
        lv_color_t end;
} color_pair_t;

static void bg_fading_exec_cb(void *var, int32_t v) {
    lv_obj_t *obj = (lv_obj_t *)var;
    color_pair_t *p = (color_pair_t *)lv_anim_get_user_data(&label_anim);
    lv_color_t mixed = lv_color_mix( p->start , p->end , (uint8_t)v);
    lv_obj_set_style_bg_color(obj, mixed, 0);
}

void bg_fading_color_change(lv_obj_t *obj,  lv_color_t end){
    
    //lv_obj_set_style_bg_color(label_bg, end, 0);
    lv_color_t start = lv_obj_get_style_bg_color(obj, 0);
    color_pair_t cp = {
        .start = start,
        .end = end
    };


    lv_anim_init(&label_anim);
    lv_anim_set_var(&label_anim, obj);
    lv_anim_set_values(&label_anim, 0, 255);
    lv_anim_set_user_data(&label_anim, &cp);
    lv_anim_set_time(&label_anim, 1000);
    lv_anim_set_exec_cb(&label_anim, bg_fading_exec_cb);

    lv_anim_set_path_cb(&label_anim, lv_anim_path_ease_in_out);
    lv_anim_start(&label_anim);
}


void door_controller_ui_update_status(DoorStatus status) {
    char img_source[64];
    switch (status) {
        case IDLE:
            strcpy(marquee_text, "掃 QR Code 開門~");
            strcpy(img_source, "/sdcard/idle.jpg");
            bg_fading_color_change(label_bg, lv_color_black());
            break;
        case LOADING:
            strcpy(marquee_text, "處理中 ^3^. Processing ~");
            strcpy(img_source, "/sdcard/loading.jpg");
            bg_fading_color_change(label_bg, lv_color_hex(0x2962FF)); // Blue
            break;
        case SUCCESS:
            strcpy(marquee_text, "門已開啟 ~");
            strcpy(img_source, "/sdcard/success.jpg");
            bg_fading_color_change(label_bg, lv_color_hex(0x00796B)); // Orange
            break;
        case EQUIP_ERR:
            strcpy(marquee_text, "設備錯誤");
            strcpy(img_source, "/sdcard/equip_err.jpg");
            bg_fading_color_change(label_bg, lv_color_hex(0xFF0266));
            break;
        case INUSE:
            strcpy(marquee_text, "使用中");
            strcpy(img_source, "/sdcard/inuse.jpg");
            bg_fading_color_change(label_bg, lv_color_hex(0xFF0266));
            break;
        case DEPOSIT_ERR:
            strcpy(marquee_text, "押金超過限制");
            strcpy(img_source, "/sdcard/deposit_err.jpg");
            bg_fading_color_change(label_bg, lv_color_hex(0xFF0266)); 
            break;
        case CODE_EXPIRE:
            strcpy(marquee_text, "驗證碼已過期");
            strcpy(img_source, "/sdcard/code_expire.jpg");
            bg_fading_color_change(label_bg, lv_color_hex(0xFF0266)); 
            break;
        case USING_ANOTHER_SHELF:
            strcpy(marquee_text, "上一個櫃門未關好");
            strcpy(img_source, "/sdcard/using_another_shelf.jpg");
            bg_fading_color_change(label_bg, lv_color_hex(0xFF0266)); 
            break;
        case CHECKED_OUT:
            strcpy(marquee_text, "已結帳, 無法使用");
            strcpy(img_source, "/sdcard/checked_out.jpg");
            bg_fading_color_change(label_bg, lv_color_hex(0x000000)); 
            break;
        default:
            strcpy(marquee_text, "搞唔掂, 請聯絡職員");
            strcpy(img_source, "/sdcard/idle.jpg");
            bg_fading_color_change(label_bg, lv_color_hex(0xFF0266)); 
            break;
    }

    lv_label_set_text(label, marquee_text);
    marquee_x = lv_obj_get_width(parent); // reset to right edge
    ESP_LOGI("door_controller_ui", "Marquee text updated: %s", marquee_text);
    
    #ifndef USE_GIF_IMG
        lv_img_set_src(logo, img_source);
    #endif
    //lv_gif_set_src(gif_img, img_source);
}


void door_controller_ui_init(void) {
    //create parent container
    parent = lv_screen_active();
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    //Create Logo
    logo = lv_image_create(parent);
    lv_image_set_src(logo, "/sdcard/LOGO.JPG"); // Path to JPEG on SD card
    lv_obj_set_size(logo, 320, 80);
    lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 0); // Center the image

    const lv_image_dsc_t *img_dsc = lv_image_get_src(logo);
    if(img_dsc == NULL) {
        ESP_LOGE("door_controller_ui", "Failed to load logo image from SD card");
    } else {
        ESP_LOGI("door_controller_ui", "Logo image loaded: width=%d, height=%d", img_dsc->header.w, img_dsc->header.h);
    }

//     //Create Status Bar
//     status_bar = lv_label_create(parent);
//     lv_label_set_text(status_bar, ""); // Initially empty
//     lv_obj_set_style_text_color(status_bar, lv_color_hex(0xFF0000), 0); // Red color for errors
//     lv_obj_set_style_text_font(status_bar, FONT_DEFAULT, 0);
//     lv_obj_align(status_bar, LV_ALIGN_TOP_MID, 0, 85); // Position below logo
//     if (status_check_timer == NULL) {
//         status_check_timer = lv_timer_create(status_check_timer_cb, 1000, NULL); // Check every second
//     }

// #ifdef USE_GIF_IMG
//     // Create gif image
//     gif_img = lv_gif_create(parent);
//     lv_gif_set_src(gif_img, "/sdcard/loading.gif");
//     lv_obj_center(gif_img);
//     lv_obj_set_size(gif_img, 320, 700);
//     // Optional: control one of them later
//     // lv_gif_pause(gif_img);
//     // lv_gif_resume(gif_img);
//     // lv_gif_restart(gif_img);
// #else
//     // Create fixed image
    // fix_img = lv_image_create(parent);
    // lv_img_set_src(fix_img, "/sdcard/idle.jpg");
    // lv_obj_center(fix_img);
    // lv_obj_set_size(fix_img, 320, 700);
// #endif

    // Create label background box
    label_bg = lv_obj_create(parent);
    lv_obj_set_style_bg_color(label_bg, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(label_bg, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(label_bg, 0, 0);
    lv_obj_set_width(label_bg, lv_obj_get_width(parent) + 20);
    lv_obj_set_height(label_bg, 100);
    lv_obj_align(label_bg, LV_ALIGN_BOTTOM_MID, 0, 0);

    // Create label on top of background box
    label = lv_label_create(label_bg);
    lv_label_set_text(label, marquee_text);
    lv_obj_set_style_text_color(label, lv_color_white(), 0); // Set text color to white
    lv_obj_set_style_text_font(label, FONT_DEFAULT, 0);
    lv_obj_set_width(label, LV_SIZE_CONTENT); // Let label size to its text
    lv_obj_set_height(label, lv_font_get_line_height(FONT_DEFAULT));
    lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);
    marquee_x = lv_obj_get_width(label_bg);
    if (marquee_timer == NULL) {
        marquee_timer = lv_timer_create(marquee_timer_cb, 30, NULL); // 30 ms per frame
    }



    //Optionally: Initialize with IDLE status after a delay or event
    door_controller_ui_update_status(LOADING);
}
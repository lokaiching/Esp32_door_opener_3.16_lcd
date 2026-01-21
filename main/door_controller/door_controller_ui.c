
#include "door_controller_ui.h"
#include "lvgl.h"
#include <string.h>

// Assume these are defined/created elsewhere and set by your main UI init
extern lv_obj_t* parent;
extern lv_obj_t* label;
extern lv_obj_t* img;

lv_obj_t* parent = NULL;
lv_obj_t* label = NULL;
lv_obj_t* img = NULL;

#define MARQUEE_SPEED 2 // pixels per frame
static int marquee_x = 0;
static char marquee_text[128] = "刷QRCode开门";
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
            strcpy(marquee_text, "Scan QR to open door");
            strcpy(img_source, "S:/idle.jpg");
            break;
        case LOADING:
            strcpy(marquee_text, "Loading...");
            strcpy(img_source, "S:/loading.jpg");
            break;
        case SUCCESS:
            strcpy(marquee_text, "Door opened ~");
            strcpy(img_source, "S:/success.jpg");
            break;
        case EQUIP_ERR:
            strcpy(marquee_text, "Failed: Equipment Error");
            strcpy(img_source, "S:/equip_err.jpg");
            break;
        case INUSE:
            strcpy(marquee_text, "Failed: In Use");
            strcpy(img_source, "S:/inuse.jpg");
            break;
        case DEPOSIT_ERR:
            strcpy(marquee_text, "Failed: Deposit Exceeded");
            strcpy(img_source, "S:/deposit_err.jpg");
            break;
        case CODE_EXPIRE:
            strcpy(marquee_text, "Failed: Code Expired");
            strcpy(img_source, "S:/code_expire.jpg");
            break;
        case USING_ANOTHER_SHELF:
            strcpy(marquee_text, "Failed: Using another shelf");
            strcpy(img_source, "S:/using_another_shelf.jpg");
            break;
        case CHECKED_OUT:
            strcpy(marquee_text, "Failed: Checked out");
            strcpy(img_source, "S:/checked_out.jpg");
            break;
        default:
            strcpy(marquee_text, "Unknown Status");
            strcpy(img_source, "S:/idle.jpg");
            break;
    }
    lv_label_set_text(label, marquee_text);
    marquee_x = lv_obj_get_width(parent); // reset to right edge
    lv_img_set_src(img, img_source);
}


void door_controller_ui_init(void) {
    //create parent container
    parent = lv_screen_active();
        // Optional: set background color (helps debugging)
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    // Create label
    label = lv_label_create(parent);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 20); // Position it above center
    lv_label_set_text(label, marquee_text);
    lv_obj_set_style_text_color(label, lv_color_white(), 0); // Set text color to white
    lv_obj_set_style_text_font(label, &lv_font_montserrat_26, 0); // Use default font, or replace LV_FONT_DEFAULT with a valid font pointer
    lv_obj_set_width(label, lv_obj_get_width(parent));
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL); // Enable scrolling
    marquee_x = lv_obj_get_width(parent);
    if (marquee_timer == NULL) {
        marquee_timer = lv_timer_create(marquee_timer_cb, 30, NULL); // 30 ms per frame
    }

    // Create image
    img = lv_img_create(parent);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 40); // Position it below center

    // Optionally: Initialize with IDLE status after a delay or event
    // door_controller_ui_update_status(IDLE);
}
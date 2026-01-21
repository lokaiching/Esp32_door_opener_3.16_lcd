#include "door_controller_ui.h"
#include "lvgl.h"
#include <string.h>

// Assume these are defined/created elsewhere and set by your main UI init
extern lv_obj_t* label;
extern lv_obj_t* img;

void door_controller_ui_update_status(DoorStatus status) {
    char labelText[64];
    char img_source[64];

    switch (status) {
        case IDLE:
            strcpy(labelText, "Scan QR to open door");
            strcpy(img_source, "S:/idle.jpg");
            break;
        case LOADING:
            strcpy(labelText, "Loading...");
            strcpy(img_source, "S:/loading.jpg");
            break;
        case SUCCESS:
            strcpy(labelText, "Door opened ~");
            strcpy(img_source, "S:/success.jpg");
            break;
        case EQUIP_ERR:
            strcpy(labelText, "Failed: Equipment Error");
            strcpy(img_source, "S:/equip_err.jpg");
            break;
        case INUSE:
            strcpy(labelText, "Failed: In Use");
            strcpy(img_source, "S:/inuse.jpg");
            break;
        case DEPOSIT_ERR:
            strcpy(labelText, "Failed: Deposit Exceeded");
            strcpy(img_source, "S:/deposit_err.jpg");
            break;
        case CODE_EXPIRE:
            strcpy(labelText, "Failed: Code Expired");
            strcpy(img_source, "S:/code_expire.jpg");
            break;
        case USING_ANOTHER_SHELF:
            strcpy(labelText, "Failed: Using another shelf");
            strcpy(img_source, "S:/using_another_shelf.jpg");
            break;
        case CHECKED_OUT:
            strcpy(labelText, "Failed: Checked out");
            strcpy(img_source, "S:/checked_out.jpg");
            break;
        default:
            strcpy(labelText, "Unknown Status");
            strcpy(img_source, "S:/idle.jpg");
            break;
    }

    lv_label_set_text(label, labelText);
    lv_img_set_src(img, img_source);
}

void door_controller_ui_show_message(const char* msg) {
    // Example: show a message on the label (customize as needed)
    lv_label_set_text(label, msg);
}

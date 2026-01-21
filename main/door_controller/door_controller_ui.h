#ifndef DOOR_CONTROLLER_UI_H
#define DOOR_CONTROLLER_UI_H

#include "app_data.h"

#ifdef __cplusplus
extern "C" {
#endif

// Call this to update the UI when door status changes
void door_controller_ui_update_status(DoorStatus status);

// Optionally, show a message or notification
void door_controller_ui_show_message(const char* msg);

#ifdef __cplusplus
}
#endif

#endif // DOOR_CONTROLLER_UI_H

#ifndef DOOR_CONTROLLER_H
#define DOOR_CONTROLLER_H

#include "app_data.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
    DOOR_STATUS_CHANGED = 0,
}door_controller_event_t;

void qr_reader_init();
void door_controller_init();

#ifdef __cplusplus
}
#endif

#endif
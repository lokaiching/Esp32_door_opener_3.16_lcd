#ifndef DOOR_CONTROLLER_H
#define DOOR_CONTROLLER_H

#include "app_data.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
    DOOR_STATUS_CHANGED = 0,
    QR_UART_INPUT_RECEIVED = 1
}door_controller_event_t;

void qr_reader_init();
void door_controller_init();
void door_controller_do_work(app_data_t* app_data);

#ifdef __cplusplus
}
#endif

#endif
#ifndef DOOR_CONTROLLER_H
#define DOOR_CONTROLLER_H

#include "app_data.h"

#ifdef __cplusplus
extern "C" {
#endif

ESP_EVENT_DECLARE_BASE(DOOR_CONTROLLER_EVENTS);

enum{
    DOOR_STATUS_CHANGED = 0,
    QR_UART_INPUT_RECEIVED = 1
}

void qr_reader_init();
void door_controller_init();
void door_controller_do_work(app_data_t* app_data);

#ifdef __cplusplus
}
#endif

#endif
#ifndef QR_READER_H
#define QR_READER_H

#include "app_data.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

ESP_EVENT_DECLARE_BASE(QR_READER_EVENTS);
ESP_EVENT_DEFINE_BASE(QR_READER_EVENTS);

typedef enum{
    QR_UART_INPUT_RECEIVED = 0
}qr_reader_event_t;

void qr_reader_init();
void on_qr_uart_input_received(void *arg, esp_event_base_t base, int32_t id, void *data);


#ifdef __cplusplus
}
#endif

#endif
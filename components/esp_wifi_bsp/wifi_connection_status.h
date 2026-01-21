#ifndef WIFI_CONNECTION_STATUS_H
#define WIFI_CONNECTION_STATUS_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Message type for WiFi status
typedef enum {
    WIFI_STATUS_DISCONNECTED,
    WIFI_STATUS_CONNECTED
} wifi_status_msg_t;

// Queue handle for WiFi status updates
extern QueueHandle_t wifi_status_queue;

#endif // WIFI_CONNECTION_STATUS_H

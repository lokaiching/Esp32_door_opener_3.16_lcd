#ifndef ESP_WIFI_BSP_H
#define ESP_WIFI_BSP_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
extern EventGroupHandle_t wifi_even_;


typedef struct
{
  char _ip[25];
  int8_t rssi;
  int8_t apNum;
}esp_bsp_t;
extern esp_bsp_t user_esp_bsp;

typedef enum
{
  WIFI_STATUS_DISCONNECTED = 0,
  WIFI_STATUS_CONNECTED = 1,
}wifi_status_msg_t;
extern wifi_status_msg_t wifi_status;

#ifdef __cplusplus
extern "C" {
#endif

void espwifi_init(void);
void espwifi_deinit(void);


#ifdef __cplusplus
}
#endif

#endif
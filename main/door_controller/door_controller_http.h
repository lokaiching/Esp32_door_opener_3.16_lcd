#ifndef DOOR_CONTROLLER_HTTP_H
#define DOOR_CONTROLLER_HTTP_H

#include <stdbool.h>
#include <stddef.h>
#include "app_data.h"

#ifdef __cplusplus
extern "C" {
#endif

DoorStatus door_open_post_request(const char* customer_id, size_t* out_len);

#ifdef __cplusplus
}
#endif

#endif // DOOR_CONTROLLER_HTTP_H

#include "door_controller_http.h"
#include <string.h>
#include <stdlib.h>
#include <esp_log.h>
#include <esp_http_client.h>
#include <cJSON.h>
#include "app_data.h"

#define SERVER_URL CONFIG_VENDING_SERVER_URL
#define SHELF_ID CONFIG_SHELF_ID

static const char* TAG = "door_controller_http";

char* build_door_open_request_json(const char* customer_id) {
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "customerId", customer_id);
    cJSON_AddStringToObject(root, "shelfCode", SHELF_ID);
    char* jsonString = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return jsonString; // Must be freed by caller using cJSON_free()
}

DoorStatus door_open_post_request(const char* customer_id) {
    
    const char* door_open_url = SERVER_URL "/api/Cart/openShelfByQrCode";  
    char* jsonString = build_door_open_request_json(customer_id);

    // init and send the post request
    esp_http_client_config_t config = { .url = door_open_url};
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, jsonString, strlen(jsonString));
    esp_err_t err = esp_http_client_perform(client);

    // read response
    char* response = NULL;
    if (err == ESP_OK) {
        int content_length = esp_http_client_get_content_length(client);
        response = malloc(content_length + 1);
        if (response) {
            esp_http_client_read(client, response, content_length);
            response[content_length] = '\0';
        }
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d", esp_http_client_get_status_code(client), content_length);
    
    } else {
    
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    cJSON_free(jsonString);

    DoorStatus status = convert_response_to_door_status(response);
    free(response);
    return status;
}

DoorStatus convert_response_to_door_status(const char* response) {
    if (!response) 
        return EQUIP_ERR;
    
    int responseCode = atoi(response);
    
    switch (responseCode) {
        case 1:
            return SUCCESS;
        case 2:
            return EQUIP_ERR;
        case 3:
            return INUSE;
        case 4:
            return DEPOSIT_ERR;
        case 5:
            return CODE_EXPIRE;
        case 6:
            return USING_ANOTHER_SHELF;
        case 7:
            return CHECKED_OUT;
        default:
            return IDLE;
    }
}



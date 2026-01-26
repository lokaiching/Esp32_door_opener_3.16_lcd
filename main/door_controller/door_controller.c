#include "esp_log.h"
#include "esp_event.h"
#include "esp_timer.h"

#include "app_data.h"
#include "door_controller.h"
#include "door_controller_ui.h"
#include "door_controller_http.h"

#include "../serial_init/qr_reader.h"

// Free RTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#define STATUS_TIME_OUT_MS   10000 // 10 seconds

ESP_EVENT_DECLARE_BASE(DOOR_CONTROLLER_EVENTS);
ESP_EVENT_DEFINE_BASE(DOOR_CONTROLLER_EVENTS);
//esp_event_base_t DOOR_CONTROLLER_EVENTS = "door_controller_events";
static const char *TAG = "door_controller";

app_data_t app_data = {
	.wifi_connected = false,
	.door_status = IDLE,
	.status_changed = false,
	.last_status_update = 0,
};

/* Event Handlers */
static void on_status_changed(void *arg, esp_event_base_t base, int32_t id, void *data){
	if(base == DOOR_CONTROLLER_EVENTS && id == DOOR_STATUS_CHANGED){
		ESP_LOGI(TAG, "Door status changed event received");
		door_controller_ui_update_status(app_data.door_status);
		app_data.status_changed = false;
		app_data.last_status_update = esp_timer_get_time() / 1000; // in ms
		ESP_LOGI(TAG, "Door status changed to %d", app_data.door_status);
	}
}

static void on_qr_uart_input_received(void *arg, esp_event_base_t base, int32_t id, void *data){
	
	const char *qr_string = (const char *)data;
	
	// Change to loading status
	ESP_LOGI(TAG, "QR code scanned event received: %s", qr_string);
	app_data.door_status = LOADING;
	esp_event_post(DOOR_CONTROLLER_EVENTS, DOOR_STATUS_CHANGED, NULL, 0, portMAX_DELAY);

	// Make HTTP POST request to open door
	DoorStatus status = door_open_post_request(qr_string);
	app_data.door_status = status;
	app_data.status_changed = true;

	// Post event to notify status change
	esp_event_post(DOOR_CONTROLLER_EVENTS, DOOR_STATUS_CHANGED, NULL, 0, portMAX_DELAY);
	ESP_LOGI(TAG, "Door status updated to %d after QR scan", app_data.door_status);
}

static void status_reset_timer_callback(TimerHandle_t xTimer) {
    unsigned long current_time = esp_timer_get_time() / 1000; // in ms
    if (current_time - app_data.last_status_update >= STATUS_TIME_OUT_MS) {
        app_data.door_status = IDLE;
        app_data.status_changed = true;
        esp_event_post(DOOR_CONTROLLER_EVENTS, DOOR_STATUS_CHANGED, NULL, 0, portMAX_DELAY);
        ESP_LOGI(TAG, "Door status reset to IDLE after timeout");
    }
}

void start_status_reset_timer() {
	TimerHandle_t status_reset_timer = xTimerCreate(
		"StatusResetTimer",
		pdMS_TO_TICKS(1000),
		pdFALSE,
		(void*)0,
		status_reset_timer_callback
	);
}

void door_controller_init() {

	app_data.door_status = IDLE;
	app_data.wifi_connected = false;
	app_data.status_changed = false;
	app_data.last_status_update = esp_timer_get_time() / 1000; // in ms
	
	// 1. Register event handler for status changes
	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_register(DOOR_CONTROLLER_EVENTS, DOOR_STATUS_CHANGED, &on_status_changed, NULL, &instance_any_id);
	esp_event_handler_instance_t qr_uart_instance;
	esp_event_handler_instance_register(QR_READER_EVENTS, QR_UART_INPUT_RECEIVED, &on_qr_uart_input_received, NULL, &qr_uart_instance);

	// 3. Initialize status reset timer
	start_status_reset_timer();

	ESP_LOGI(TAG, "Door controller initialized");
}

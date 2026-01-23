#include "esp_log.h"
#include "esp_event.h"
#include "esp_timer.h"

#include "app_data.h"
#include "door_controller.h"
#include "door_controller_ui.h"
#include "door_controller_http.h"

// ESP-IDF UART includes
#include "driver/uart.h"
#include "driver/gpio.h"

// Free RTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#define QR_UART_PORT_NUM      UART_NUM_1
#define QR_UART_BAUD_RATE     CONFIG_QR_READER_BAUD_RATE
#define QR_UART_TX_PIN        (GPIO_NUM_17) // Change as needed
#define QR_UART_RX_PIN        (GPIO_NUM_16) // Change as needed
#define QR_UART_BUF_SIZE      1024

#define STATUS_TIME_OUT_MS   10000 // 10 seconds

ESP_EVENT_DECLARE_BASE(DOOR_CONTROLLER_EVENTS);
ESP_EVENT_DEFINE_BASE(DOOR_CONTROLLER_EVENTS);
//esp_event_base_t DOOR_CONTROLLER_EVENTS = "door_controller_events";
static const char *TAG = "door_controller";
QueueHandle_t qr_uart_queue = NULL;

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
	if(base == DOOR_CONTROLLER_EVENTS && id == QR_UART_INPUT_RECEIVED){
		
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
}

void qr_reader_init() {
	const uart_config_t uart_config = {
		.baud_rate = QR_UART_BAUD_RATE,
		.data_bits = UART_DATA_8_BITS,
		.parity    = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.source_clk = UART_SCLK_APB,
	};

	// Install UART driver with event queue
	uart_driver_install(QR_UART_PORT_NUM, QR_UART_BUF_SIZE, QR_UART_BUF_SIZE, 20, &qr_uart_queue, 0);
	uart_param_config(QR_UART_PORT_NUM, &uart_config);
	uart_set_pin(QR_UART_PORT_NUM, QR_UART_TX_PIN, QR_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	ESP_LOGI(TAG, "QR Reader UART initialized on port %d", QR_UART_PORT_NUM);
}

static void qr_uart_input_event_task(void *pvParameters) {
	uart_event_t event;
	static char scannedString[256] = {0};
	static int scanIndex = 0;
	static bool scanComplete = false;
	uint8_t data[64];

	while (1) {
		// Wait for UART event
		if (xQueueReceive(qr_uart_queue, (void *)&event, portMAX_DELAY)) {
			switch (event.type) {
				case UART_DATA: {
					int len = uart_read_bytes(QR_UART_PORT_NUM, data, event.size, 200 / portTICK_PERIOD_MS);
					for (int i = 0; i < len && !scanComplete; ++i) {
						char incomingByte = (char)data[i];
						if (incomingByte == '@') {
							scanComplete = true;
							scannedString[scanIndex] = '\0';
						} else {
							if (scanIndex < (int)sizeof(scannedString) - 1) {
								scannedString[scanIndex++] = incomingByte;
							}
						}
					}
					if (scanComplete) {
						ESP_LOGI(TAG, "QR Scan Complete: %s", scannedString);
						// Post event to notify QR code scanned
						esp_event_post(DOOR_CONTROLLER_EVENTS, QR_UART_INPUT_RECEIVED, scannedString, strlen(scannedString) + 1, portMAX_DELAY); // + 1 for the termination byte \0

						// Reset for next scan
						scanIndex = 0;
						scannedString[0] = '\0';
						scanComplete = false;
					}
					break;
				}
				case UART_FIFO_OVF:
					ESP_LOGW(TAG, "UART FIFO Overflow");
					uart_flush_input(QR_UART_PORT_NUM);
					xQueueReset(qr_uart_queue);
					break;
				case UART_BUFFER_FULL:
					ESP_LOGW(TAG, "UART Buffer Full");
					uart_flush_input(QR_UART_PORT_NUM);
					xQueueReset(qr_uart_queue);
					break;
				case UART_BREAK:
				case UART_PARITY_ERR:
				case UART_FRAME_ERR:
					ESP_LOGW(TAG, "UART Error Event: %d", event.type);
					break;
				default:
					break;
			}
		}
	}
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
	esp_event_handler_instance_register(DOOR_CONTROLLER_EVENTS, QR_UART_INPUT_RECEIVED, &on_qr_uart_input_received, NULL, &qr_uart_instance);

	// 2. Initialize QR code reader UART
	qr_reader_init();
	xTaskCreatePinnedToCore(qr_uart_input_event_task, "qr_uart_input_event_task", 2048, NULL, 12, NULL, 0);
	
	// 3. Initialize status reset timer
	start_status_reset_timer();

	ESP_LOGI(TAG, "Door controller initialized");
}

void door_controller_do_work(app_data_t* app_data) {
	// No need to poll for QR input anymore; handled by event task
	while(1){
		vTaskDelay(1000 / portTICK_PERIOD_MS); // Idle loop or add other periodic work
	}
}
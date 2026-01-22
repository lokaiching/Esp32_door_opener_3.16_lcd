#include "esp_log.h"
#include "esp_event.h"
#include "app_data.h"
#include "door_controller.h"
#include "door_controller_ui.h"

// ESP-IDF UART includes
#include "driver/uart.h"
#include "driver/gpio.h"

// Free RTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#define QR_UART_PORT_NUM      UART_NUM_1
#define QR_UART_BAUD_RATE     9600
#define QR_UART_TX_PIN        (GPIO_NUM_17) // Change as needed
#define QR_UART_RX_PIN        (GPIO_NUM_16) // Change as needed
#define QR_UART_BUF_SIZE      1024

#define STATUS_TIME_OUT_MS   10000 // 10 seconds

static const char *TAG = "door_controller";

app_data_t app_data;

/* Event Handlers */
static void on_status_change_cb(void *arg, esp_event_base_t base, int32_t id, void *data){
	//TODO
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

	// Configure UART parameters
	uart_driver_install(QR_UART_PORT_NUM, QR_UART_BUF_SIZE, 0, 0, NULL, 0);
	uart_param_config(QR_UART_PORT_NUM, &uart_config);
	uart_set_pin(QR_UART_PORT_NUM, QR_UART_TX_PIN, QR_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    LOGI("QR Reader UART initialized on port %d", QR_UART_PORT_NUM);
}

void handle_qr_input() {
	static char scannedString[256] = {0};
	static int scanIndex = 0;
	static bool scanComplete = false;

    //check if there is buffer
    size_t buffered_len = 0;
    uart_get_buffered_data_len(QR_UART_PORT_NUM, &buffered_len);
    if(buffered_len == 0) {
        return; // No data to read
    }

    LOGI("Buffered data length: %d", buffered_len);
	uint8_t data[64];
	int len = uart_read_bytes(QR_UART_PORT_NUM, data, sizeof(data), 100 / portTICK_PERIOD_MS);
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
        LOGI("QR Scan Complete: %s", scannedString);

		// TODO: Implement these functions as needed in your project
		// checkWiFiConnection();
		// lv_label_set_text(label, scannedString);
		// setStatus(DoorStatus_SUCCESS);

		// Print received string for debug
		printf("Received: %s\n", scannedString);
		// testJsonString(scannedString);
		// doorStatus = convertResponseToDoorStatus(sendPostRequest(scannedString));
		// setStatus(doorStatus);

		// Reset for next scan
		scanIndex = 0;
		scannedString[0] = '\0';
		scanComplete = false;
	}
}

void start_status_reset_timer() {
	TimerHandle_t status_reset_timer = xTimerCreate(
		"StatusResetTimer",
		pdMS_TO_TICKS(1000),
		pdFALSE,
		(void*)0,
		[](TimerHandle_t xTimer) {
			unsigned long current_time = esp_timer_get_time() / 1000; // in ms
			if (current_time - app_data.last_status_update >= STATUS_TIME_OUT_MS) { //
				app_data.door_status = IDLE;
				app_data.status_changed = false;
				LOGI("Door status reset to IDLE after timeout");
			}
		}
	);
}



void door_controller_init() {
	// Initialize QR code reader UART
	qr_reader_init();

	// Register event handler for status changes
	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_register("door_controller", ESP_EVENT_ANY_ID, &on_status_change_cb, NULL, &instance_any_id);

	LOGI("Door controller initialized");
}



void door_controller_do_work(app_data_t* app_data) {
    while(1){
        handle_qr_input();
        vTaskDelay(100 / portTICK_PERIOD_MS); // Adjust delay as needed
    }
}
#include "esp_log.h"
#include "esp_event.h"
#include "esp_timer.h"

// ESP-IDF UART includes
#include "driver/uart.h"
#include "driver/gpio.h"

#include "qr_reader.h"


#define QR_UART_PORT_NUM      UART_NUM_1
#define QR_UART_BAUD_RATE     CONFIG_QR_READER_BAUD_RATE
#define QR_UART_TX_PIN        (GPIO_NUM_43) // Change as needed
#define QR_UART_RX_PIN        (GPIO_NUM_44) // Change as needed
#define QR_UART_BUF_SIZE      1024

#define STATUS_TIME_OUT_MS   10000 // 10 seconds

ESP_EVENT_DECLARE_BASE(QR_READER_EVENTS);
ESP_EVENT_DEFINE_BASE(QR_READER_EVENTS);
//esp_event_base_t DOOR_CONTROLLER_EVENTS = "door_controller_events";
static const char *TAG = "qr_coder_reader";
QueueHandle_t qr_uart_queue = NULL;

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
	xTaskCreatePinnedToCore(qr_uart_input_event_task, "qr_uart_input_event_task", 4096, NULL, 12, NULL, 0);	
	
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
						esp_event_post(QR_READER_EVENTS, QR_UART_INPUT_RECEIVED, scannedString, strlen(scannedString) + 1, portMAX_DELAY); // + 1 for the termination byte \0

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
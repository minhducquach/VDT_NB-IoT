#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

static const int RX_BUF_SIZE = 1024;

#define PWR 2

#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)

int state = -1;
bool flag = 0;

void init(void) {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void initGPIO(void){
	gpio_config_t io_conf;
		io_conf.pin_bit_mask = 1 << PWR;
		io_conf.mode = GPIO_MODE_OUTPUT;
		io_conf.pull_up_en = 0;
		io_conf.pull_down_en = 0;
		io_conf.intr_type = GPIO_INTR_DISABLE;
		gpio_config(&io_conf);
	gpio_set_level(PWR, 0);
}

int sendData(const char* logName, const char* data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
    ESP_LOGI(logName, "Wrote %d bytes: %s", txBytes, data);
    return txBytes;
}

static void tx_task(void *arg)
{
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
    	switch (state){
    	case -1:
    		sendData(TX_TASK_TAG, "AT+IPR=115200\r");
    		break;
    	case 0:
    		sendData(TX_TASK_TAG, "ATE0\r");
    		vTaskDelay(1000 / portTICK_PERIOD_MS);
    		break;
    	case 1:
    		sendData(TX_TASK_TAG, "AT+CEREG?\r");
    		vTaskDelay(1000 / portTICK_PERIOD_MS);
    		break;
    	case 2:
    		sendData(TX_TASK_TAG, "AT+CENG?\r");
    		vTaskDelay(1000 / portTICK_PERIOD_MS);
    		break;
    	case 3:
    		sendData(TX_TASK_TAG, "AT+CNACT=0,0\r");
    		vTaskDelay(1000 / portTICK_PERIOD_MS);
    		break;
    	case 4:
    		sendData(TX_TASK_TAG, "AT+CNACT=0,1\r");
    		vTaskDelay(1000 / portTICK_PERIOD_MS);
    		break;
    	case 5:
    		sendData(TX_TASK_TAG, "AT+CNACT?\r");
    		vTaskDelay(1000 / portTICK_PERIOD_MS);
    		break;
    	case 6:
    		sendData(TX_TASK_TAG, "AT+SMCONF=\"URL\",\"mqtt.thingsboard.cloud\",1883\r");
    		vTaskDelay(1000 / portTICK_PERIOD_MS);
    		break;
    	case 7:
    		sendData(TX_TASK_TAG, "AT+SMCONF=\"KEEPTIME\",60\r");
    		vTaskDelay(1000 / portTICK_PERIOD_MS);
    		break;
    	case 8:
    		sendData(TX_TASK_TAG, "AT+SMCONF=\"USERNAME\",\"HXKXMzqojhpfaeirFPjA\"\r");
    		vTaskDelay(1000 / portTICK_PERIOD_MS);
    		break;
    	case 9:
    		sendData(TX_TASK_TAG, "AT+SMCONF=\"CLIENTID\",\"4085ee10-fd30-11ed-b64e-63b44ed1ddba\"\r");
    		vTaskDelay(1000 / portTICK_PERIOD_MS);
    		break;
    	case 10:
    		sendData(TX_TASK_TAG, "AT+SMCONF?\r");
    		vTaskDelay(1000 / portTICK_PERIOD_MS);
    		break;
    	case 11:
    		sendData(TX_TASK_TAG, "AT+SMCONN\r");
    		vTaskDelay(5000 / portTICK_PERIOD_MS);
    		break;
    	case 12:
    		sendData(TX_TASK_TAG, "AT+SMSTATE?\r");
    		vTaskDelay(1000 / portTICK_PERIOD_MS);
    		break;
    	case 13:
    		sendData(TX_TASK_TAG, "AT+SMPUB=\"v1/devices/me/telemetry\",8,0,1,{\"st\":1}\r");
    		vTaskDelay(1000 / portTICK_PERIOD_MS);
    		break;
    	default:
    		break;
    	}
    	vTaskDelay(4000 / portTICK_PERIOD_MS);
    }
}

static void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);
    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        if (rxBytes > 0) {
            data[rxBytes] = 0;
            if (state == -1 || (state >= 1 && state <= 3) ||state == 12){
            	if (state == -1 && strstr((const char*) data, "OK")){
            		state = 0;
            	    ESP_LOGI(RX_TASK_TAG, "State: '%d'", state);
            	    gpio_set_level(PWR, 1);
            	}
            	else if (state == 1 && strstr((const char*) data, "0,1")){
            		ESP_LOGI(RX_TASK_TAG, "State: '%d'", state);
            		state++;
            	}
            	else if (state == 2 && strstr((const char*) data, "NB-IOT")){
            		ESP_LOGI(RX_TASK_TAG, "State: '%d'", state);
            		state++;
            	}
            	else if (state == 3){
            		ESP_LOGI(RX_TASK_TAG, "State: '%d'", state);
            	    state++;
            	}
            	else if (state == 12 && (strstr((const char*) data, "1") || strstr((const char*) data, "2"))){
            		ESP_LOGI(RX_TASK_TAG, "State: '%d'", state);
            	    state++;
            	}
            	else if (state == 12 && (strstr((const char*) data, "0"))){
            	    ESP_LOGI(RX_TASK_TAG, "State: '%d'", state);
            	    state--;
            	}
            }
            else if (strstr((const char*) data, "OK")){
            	ESP_LOGI(RX_TASK_TAG, "State: '%d'", state);
            	state++;
            }
            else{
            	ESP_LOGI(RX_TASK_TAG, "State: '%d'", state);
            }

            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
//            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        }
    }
    free(data);
}

void app_main(void)
{
    init();
    initGPIO();
    xTaskCreate(rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
    xTaskCreate(tx_task, "uart_tx_task", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
}
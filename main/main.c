#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "soc/soc_caps.h"
#include "esp_sleep.h"
#include <sys/time.h>
#include <time.h>
#include "soc/rtc.h"
#include <inttypes.h>
#include "esp_timer.h"
#include <rom/ets_sys.h>
#include <stdio.h>
#include <stdlib.h>
#include "nvs_flash.h"
#include "nvs.h"


static const int RX_BUF_SIZE = 1024;

#define PWR 2
#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)

bool flag = 0;
char* data_pub;
char *data_raw = ""	;
char* data_send;
char *rsrp, *rsrq, *sinr, *pci, *cellid, *mcc, *mnc, *tac, *lon, *lat;

#define DST 5

RTC_DATA_ATTR int state = -1;
RTC_DATA_ATTR bool restart = 0;
RTC_DATA_ATTR bool sleepState = 0;
uint64_t timer = 0;


RTC_DATA_ATTR nvs_handle_t my_handle;

void goToDS(uint64_t timer){
	printf("Going to DS\n");
	esp_sleep_enable_timer_wakeup(timer);
	esp_deep_sleep_start();
}

void getDataCENG(char *chuoi);
void getDataGNSS(char *chuoi);
void init(void)
{

	const uart_config_t uart_config = { .baud_rate = 115200,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.source_clk = UART_SCLK_DEFAULT,
	};

	uart_driver_install(UART_NUM_1, RX_BUF_SIZE *2, 0, 0, NULL, 0);
	uart_param_config(UART_NUM_1, &uart_config);
	uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

	nvs_flash_init();
}

void initGPIO(void)
{
	gpio_config_t io_conf;
	io_conf.pin_bit_mask = 1 << PWR;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pull_up_en = 1;
	io_conf.pull_down_en = 0;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	gpio_config(&io_conf);
	if (restart == 0){
		gpio_set_level(PWR, 0);
	}
	else gpio_set_level(PWR, 1);
}

int sendData(const char *logName, const char *data)
{
	const int len = strlen(data);
	const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
	ESP_LOGI(logName, "Wrote %d bytes: %s", txBytes, data);
	return txBytes;
}

static void tx_task(void *arg)
{
	static
	const char *TX_TASK_TAG = "TX_TASK";
	esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
	while (1)
	{
		switch (state)
		{
			case -1:
				sendData(TX_TASK_TAG, "AT+IPR=115200\r");

				break;
			case 0:
				sendData(TX_TASK_TAG, "ATE0\r");
				vTaskDelay(500 / portTICK_PERIOD_MS);
				break;
			case 1:
				if (flag == 1)
				{
					sendData(TX_TASK_TAG, "AT+CPOWD=0\r");
					vTaskDelay(5000 / portTICK_PERIOD_MS);
					state = -1;
					flag = 0;
				}
				else
				{
					sendData(TX_TASK_TAG, "AT+CEREG?\r");
					vTaskDelay(500 / portTICK_PERIOD_MS);
				}
				break;
			case 2:
				sendData(TX_TASK_TAG, "AT+SMCONF=\"URL\",\"io.adafruit.com\",1883\r");
				vTaskDelay(500 / portTICK_PERIOD_MS);
				break;
			case 3:
				sendData(TX_TASK_TAG, "AT+SMCONF=\"USERNAME\",\"minhduco19\"\r");
				vTaskDelay(500 / portTICK_PERIOD_MS);
				break;
			case 4:
				sendData(TX_TASK_TAG, "AT+SMCONF=\"PASSWORD\",\"aio_ytot39or1nRzN5T2KMuSJrO8yCrR\"\r");
				vTaskDelay(500 / portTICK_PERIOD_MS);
				break;
			case 5:
				sendData(TX_TASK_TAG, "AT+SMCONF=\"CLIENTID\",\"4085ee10-fd30-11ed-b64e-63b44ed1ddba\"\r");
				vTaskDelay(500 / portTICK_PERIOD_MS);
				break;
			case 6:
				sendData(TX_TASK_TAG, "AT+CENG?\r");
				vTaskDelay(500 / portTICK_PERIOD_MS);
				break;
			case 7:
				sendData(TX_TASK_TAG, "AT+CGNSPWR=1\r");
				vTaskDelay(500 / portTICK_PERIOD_MS);
				break;
			case 8:
				sendData(TX_TASK_TAG, "AT+CGNSINF\r");
				vTaskDelay(2000 / portTICK_PERIOD_MS);
				break;
			case 9:
				sendData(TX_TASK_TAG, "AT+CGNSPWR=0\r");
				vTaskDelay(500 / portTICK_PERIOD_MS);
				break;
			case 10:
				sendData(TX_TASK_TAG, "AT+CNACT=0,0\r");
				vTaskDelay(500 / portTICK_PERIOD_MS);
				break;
			case 11:
				sendData(TX_TASK_TAG, "AT+CNACT=0,1\r");
				vTaskDelay(500 / portTICK_PERIOD_MS);
				break;
			case 12:
				sendData(TX_TASK_TAG, "AT+SMDISC\r");
				vTaskDelay(500 / portTICK_PERIOD_MS);
				break;
			case 13:
				sendData(TX_TASK_TAG, "AT+SMCONN\r");
				vTaskDelay(2000 / portTICK_PERIOD_MS);
				break;
			case 14:
				ESP_LOGI(TX_TASK_TAG, "Time: %d,Restart: %d,Sleep: %d",(int)esp_timer_get_time(), restart,sleepState);
				if (restart == 0){
					char *data_ATPub = (char*) malloc(100);
					sprintf(data_ATPub, "AT+SMPUB=\"minhduco19/feeds/nb-iot-tracking.nb-iot/json\",%d,0,1\r", strlen(data_pub));
					sendData(TX_TASK_TAG, data_ATPub);
					vTaskDelay(500 / portTICK_PERIOD_MS);
					free(data_ATPub);

				}
				else {

					if (sleepState == 0 && (esp_timer_get_time()-timer < 300000000)){
						ESP_LOGI(TX_TASK_TAG, "Start Deep Sleep !!!!!!");
						sleepState = 1;
						goToDS(300000000 - (esp_timer_get_time() -timer));
					}
					else if (sleepState == 1){
						char *data_ATPub = (char*) malloc(100);
						size_t str_size;
						nvs_open("storage", NVS_READWRITE, &my_handle);
						nvs_get_str(my_handle, "data_pub", NULL, &str_size);
						data_send = (char*) malloc(str_size);
						nvs_get_str(my_handle, "data_pub", data_send, &str_size);
						nvs_close(my_handle);
						sprintf(data_ATPub, "AT+SMPUB=\"minhduco19/feeds/nb-iot-tracking.nb-iot/json\",%d,0,1\r", strlen(data_send));
						sendData(TX_TASK_TAG, data_ATPub);
						sleepState=0;
						vTaskDelay(500 / portTICK_PERIOD_MS);
						free(data_ATPub);
					}
					else
					{
						char *data_ATPub = (char*) malloc(100);
						sprintf(data_ATPub, "AT+SMPUB=\"minhduco19/feeds/nb-iot-tracking.nb-iot/json\",%d,0,1\r", strlen(data_pub));
						sendData(TX_TASK_TAG, data_ATPub);
						free(data_ATPub);
						restart=0;
						vTaskDelay(500 / portTICK_PERIOD_MS);
					}
				}
				break;
			case 15:
				if (restart == 0){
					sendData(TX_TASK_TAG, data_pub);}
				else sendData(TX_TASK_TAG, data_send);
				vTaskDelay(1000 / portTICK_PERIOD_MS);
				break;
			default:
				sendData(TX_TASK_TAG, "AT+CPOWD=0\r");
				vTaskDelay(5000 / portTICK_PERIOD_MS);
				state = -1;
				break;
		}

		vTaskDelay(4000 / portTICK_PERIOD_MS);
	}
}

static void rx_task(void *arg)
{
	static
	const char *RX_TASK_TAG = "RX_TASK";
	esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
	uint8_t *data = (uint8_t*) malloc(RX_BUF_SIZE + 1);
	while (1)
	{
		const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
		if (rxBytes > 0)
		{
			data[rxBytes] = 0;

			if (state == -1 || state == 1 || state == 6 || state == 8 || state == 10 || state == 12 || state == 13)
			{

				if (state == -1 && strstr((const char *) data, "OK"))
				{
					state = 0;
					ESP_LOGI(RX_TASK_TAG, "State: '%d'", state);
					gpio_set_level(PWR, 1);
				}
				else if (state == 1 && strstr((const char *) data, "0,1"))
				{
					ESP_LOGI(RX_TASK_TAG, "State: '%d'", state);
					state++;
				}
				else if (state == 1 && strstr((const char *) data, "0,0"))
				{
					flag = 1;
				}
				else if (state == 6 && strstr((const char *) data, "NB-IOT"))
				{
					ESP_LOGI(RX_TASK_TAG, "State: '%d'", state);
					data_raw = (char*) data;
					data_pub = (char*) malloc(150);
					getDataCENG(data_raw);

					state++;
				}

				else if (state == 8 && !strstr((const char *) data, "+CGNSINF: 1,,") && !strstr((const char *) data, "+CGNSINF: 0"))
				{
					ESP_LOGI(RX_TASK_TAG, "State: '%d'", state);
					ESP_LOGI(RX_TASK_TAG, "Data: %s", (char*) data);
					data_raw = (char*) data;
					getDataGNSS(data_raw);
					state++;
				}
				else if (state == 10 || state == 12)
				{
					ESP_LOGI(RX_TASK_TAG, "State: '%d'", state);
					state++;
				}
				else if (state == 13 && strstr((const char *) data, "OK"))
				{
					ESP_LOGI(RX_TASK_TAG, "State: '%d'", state);
					state++;
					if (restart==1) {
					nvs_open("storage", NVS_READWRITE, &my_handle);
					nvs_set_str(my_handle, "data_pub", data_pub);
					nvs_commit(my_handle);
					nvs_close(my_handle);}
				}
				else if (state == 13 && strstr((const char *) data, "+APP PDP: 0,DEACTIVE"))
				{
					state=6;
				}
			}
			else if (strstr((const char *) data, "OK") && state == 15)
			{
				ESP_LOGI(RX_TASK_TAG, "State: '%d'", state);
				free(data_pub);
				if (restart==1) free(data_send);
				state = 6;
				if (restart == 0) timer = esp_timer_get_time();
				else {
					timer =0;
				}
				restart = 1;
			}
			else if (strstr((const char *) data, "OK"))
			{
				ESP_LOGI(RX_TASK_TAG, "State: '%d'", state);
				state++;
			}
			else if (strstr((const char *) data, ">"))
			{
				ESP_LOGI(RX_TASK_TAG, "State: '%d'", state);

				state++;
			}
			else if (strstr((const char *) data, "+SMSTATE: 0"))
			{
				ESP_LOGI(RX_TASK_TAG, "State: '%d' error mqtt connect", state);
				state=13;
			}
			else
			{
				ESP_LOGI(RX_TASK_TAG, "State: '%d'", state);
			}

			ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
		}
	}

	free(data);
}

void getDataCENG(char *chuoi)
{

	char *found = strchr(chuoi, '\"');

	char *found1 = strstr(found, "\n");

	size_t pos = found1 - found + 1;
	char sub_str[52];
	size_t i;
	for (i = 0; i < pos - 2; i++)
	{
		sub_str[i] = found[i + 1];
	}
	char *parameter;
	int cnt5 = 0;
	parameter = strtok(sub_str, ",");
	while (parameter != NULL && cnt5 < 10)
	{
		cnt5++;
		parameter = strtok(NULL, ",");
		switch (cnt5)
		{
			case 1:
				pci = parameter;
				break;
			case 2:
				rsrp = parameter;
				break;
			case 4:
				rsrq = parameter;
				break;
			case 5:
				sinr = parameter;
				break;
			case 6:
				tac = parameter;
				break;
			case 7:
				cellid = parameter;
				break;
			case 8:
				mcc = parameter;
				break;
			case 9:
				mnc = parameter;
				break;
			default:
				break;
		}
	}
	sprintf(data_pub, "{\"value\":\"pci:%s,rsrp:%s,rsrq:%s,sinr:%s,cellid:%s\",", pci, rsrp, rsrq, sinr, cellid);
}

void getDataGNSS(char *chuoi)
{
	char *st = strchr(chuoi, ':');
	char sub_str1[(int) strlen(st)];
	size_t i;
	for (i = 0; i < strlen(st); i++) sub_str1[i] = st[i];
	char *parameter1;
	int cnt1 = 0;
	parameter1 = strtok(sub_str1, ",");
	while (parameter1 != NULL && cnt1 < 5)
	{
		cnt1++;
		parameter1 = strtok(NULL, ",");
		if (cnt1 == 3) lat = parameter1;
		else if (cnt1 == 4) lon = parameter1;
	}
	sprintf(data_pub + strlen(data_pub), "\"lat\":%s,\"lon\":%s,\"ele\":0}", lat, lon);
}

void app_main(void)
{
	init();
	initGPIO();
	xTaskCreate(rx_task, "uart_rx_task", 1024 * 2, NULL, configMAX_PRIORITIES, NULL);
	xTaskCreate(tx_task, "uart_tx_task", 1024 * 2, NULL, configMAX_PRIORITIES - 1, NULL);
}

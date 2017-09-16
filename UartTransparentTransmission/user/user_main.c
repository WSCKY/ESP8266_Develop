/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "esp_common.h"
#include "user_config.h"
#include "lwip/lwip/sockets.h"
#include "freertos/semphr.h"
#include "uart.h"

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;
        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

LOCAL int32_t sock_fd = -1;
LOCAL uint8_t sock_to_initialized = 0;
LOCAL struct sockaddr_in sock_to;

#define UDP_DATA_LEN 256
void udp_process(void *p)
{
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(UDP_LOCAL_PORT);
	server_addr.sin_len = sizeof(server_addr);

	do {
		sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
		if(sock_fd == -1) {
			printf("ESP8266 UDP task > failed to create socket!\n");
			vTaskDelay(1000/portTICK_RATE_MS);
		}
	} while(sock_fd == -1);
	printf("ESP8266 UDP task > socket OK!\n");

	int ret = 0;
	do {
		ret = bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
		if(ret != 0) {
			printf("ESP8266 UDP task > captdns_task failed to bind socket!\n");
			vTaskDelay(1000/portTICK_RATE_MS);
		}
	} while(ret != 0);
	printf("ESP8266 UDP task > bind OK!\n");

	uint8_t *udp_msg = (uint8_t *)zalloc(UDP_DATA_LEN);
	struct sockaddr_in from;
	socklen_t fromlen = 0;
	char nNetTimeout = 0;
	while(1) {
        memset(udp_msg, 0, UDP_DATA_LEN);
        memset(&from, 0, sizeof(from));

        setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&nNetTimeout, sizeof(int));
        fromlen = sizeof(struct sockaddr_in);
        ret = recvfrom(sock_fd, (uint8_t *)udp_msg, UDP_DATA_LEN, 0, (struct sockaddr *)&from, (socklen_t *)&fromlen);
        if(ret > 0) {
        	uart_send_buffer(UART0, (uint8_t *)udp_msg, ret);
        	if(sock_to_initialized == 0) {
        		memcpy(&sock_to, &from, sizeof(struct sockaddr_in));
        		sock_to_initialized = 1;
        	}
//        	printf("ESP8266 UDP task > recv %d Bytes from %s, Port %d\n", ret, inet_ntoa(from.sin_addr), ntohs(from.sin_port));
        }
	}
	if(udp_msg) {
		free(udp_msg);
		udp_msg = NULL;
	}
	close(sock_fd);
}

uint8 fifo_len = 0;
LOCAL uint8 fifo_tmp[20] = {0};

xSemaphoreHandle xSemaphore;
void udp_senddata(void *p) {
	vSemaphoreCreateBinary( xSemaphore );
	if( xSemaphore != NULL ) {
		// The semaphore was created successfully.
		// The semaphore can now be used.
	}
	while(1) {
		if( xSemaphoreTake( xSemaphore, portMAX_DELAY ) == pdTRUE ) {
			if(sock_fd != -1 && sock_to_initialized == 1) {
				sendto(sock_fd, fifo_tmp, fifo_len, 0, (struct sockaddr *)&sock_to, sizeof(struct sockaddr_in));
			}
		}
	}
}

static signed portBASE_TYPE xHigherPriorityTaskWoken = pdTRUE;
LOCAL void
uart0_rx_intr_handler(void *para)
{
    /* uart0 and uart1 intr combine togther, when interrupt occur, see reg 0x3ff20020, bit2, bit0 represents
    * uart1 and uart0 respectively
    */
    uint8 RcvChar;
    uint8 uart_no = UART0;//UartDev.buff_uart_no;
    uint8 buf_idx = 0;

    uint32 uart_intr_status = READ_PERI_REG(UART_INT_ST(uart_no)) ;

    while (uart_intr_status != 0x0) {
        if (UART_FRM_ERR_INT_ST == (uart_intr_status & UART_FRM_ERR_INT_ST)) {
            WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_FRM_ERR_INT_CLR);
        } else if (UART_RXFIFO_FULL_INT_ST == (uart_intr_status & UART_RXFIFO_FULL_INT_ST)) {
            fifo_len = (READ_PERI_REG(UART_STATUS(uart_no)) >> UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT;
            for(buf_idx = 0; buf_idx < fifo_len; buf_idx ++) {
            	fifo_tmp[buf_idx] = READ_PERI_REG(UART_FIFO(uart_no)) & 0xFF;
            }
            if(sock_fd != -1 && sock_to_initialized == 1) {
            	xSemaphoreGiveFromISR( xSemaphore, &xHigherPriorityTaskWoken );
//            	printf("fifo_len = %d, str: %s \n", fifo_len, fifo_tmp);
//            	printf("send %d bytes to %s, %d\n", fifo_len, inet_ntoa(sock_to.sin_addr), ntohs(sock_to.sin_port));
            }

            WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_RXFIFO_FULL_INT_CLR);
        } else if (UART_RXFIFO_TOUT_INT_ST == (uart_intr_status & UART_RXFIFO_TOUT_INT_ST)) {
            fifo_len = (READ_PERI_REG(UART_STATUS(uart_no)) >> UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT;
            for(buf_idx = 0; buf_idx < fifo_len; buf_idx ++) {
            	fifo_tmp[buf_idx] = READ_PERI_REG(UART_FIFO(uart_no)) & 0xFF;
            }
            if(sock_fd != -1 && sock_to_initialized == 1) {
            	xSemaphoreGiveFromISR( xSemaphore, &xHigherPriorityTaskWoken );
//            	printf("fifo_len = %d, str: %s \n", fifo_len, fifo_tmp);
//            	printf("send %d bytes to %s, %d\n", fifo_len, inet_ntoa(sock_to.sin_addr), ntohs(sock_to.sin_port));
            }

            WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_RXFIFO_TOUT_INT_CLR);
        } else if (UART_TXFIFO_EMPTY_INT_ST == (uart_intr_status & UART_TXFIFO_EMPTY_INT_ST)) {
            WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_TXFIFO_EMPTY_INT_CLR);
            CLEAR_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_TXFIFO_EMPTY_INT_ENA);
        } else {
            //skip
        }

        uart_intr_status = READ_PERI_REG(UART_INT_ST(uart_no)) ;
    }
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
	/* station + softAP mode */
	wifi_set_opmode(STATIONAP_MODE);

    struct softap_config *apconfig = (struct softap_config *)zalloc(sizeof(struct softap_config)); //initialization.
    wifi_softap_get_config(apconfig);
    sprintf(apconfig->ssid, USR_AP_SSID);
    sprintf(apconfig->password, USR_AP_PASSWD);
    apconfig->authmode = AUTH_WPA_WPA2_PSK;
    apconfig->ssid_len = 0;
    apconfig->max_connection = 4;
    wifi_softap_set_config(apconfig);
    free(apconfig);

	struct station_config *stconfig = (struct station_config *)zalloc(sizeof(struct station_config));
	sprintf(stconfig->ssid, MY_AP_SSID);
	sprintf(stconfig->password, MY_AP_PASSWD);
	wifi_station_set_config(stconfig);
	free(stconfig);

	uart_init_new();
	UART_intr_handler_register(uart0_rx_intr_handler, NULL);
	ETS_UART_INTR_ENABLE();
    xTaskCreate(udp_process, "udp_process", 512, NULL, 2, NULL);
    xTaskCreate(udp_senddata, "udp_send", 512, NULL, 1, NULL);
}

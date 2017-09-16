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

#define UDP_DATA_LEN 64
void udp_process(void *p)
{
	LOCAL int32_t sock_fd;
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
        	printf("ESP8266 UDP task > recv %d Bytes from %s, Port %d\n", ret, inet_ntoa(from.sin_addr), ntohs(from.sin_port));
        	sendto(sock_fd, (uint8_t *)udp_msg, ret, 0, (struct sockaddr *)&from, fromlen);
        }
	}
	if(udp_msg) {
		free(udp_msg);
		udp_msg = NULL;
	}
	close(sock_fd);
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
    xTaskCreate(udp_process, "udp_process", 512, NULL, 2, NULL);
}

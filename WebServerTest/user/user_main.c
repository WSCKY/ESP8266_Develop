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
//#include "httpserver-netconn.h"

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

//#define UDP_DATA_LEN 64
//void tcp_process(void *p)
//{
//	int32 listenfd;
//	int32 ret;
//	struct sockaddr_in server_addr, remote_addr;
//	int stack_countr = 0;
//	memset(&server_addr, 0, sizeof(struct sockaddr_in));  /* Zero out structure */
//	server_addr.sin_family = AF_INET;                     /* Internet address family */
//	server_addr.sin_addr.s_addr = INADDR_ANY;             /* Any incoming interface */
//	server_addr.sin_port = htons(WEB_SERVER_PORT);
//	server_addr.sin_len = sizeof(server_addr);
//
//	/* Create socket for incoming connections */
//	do {
//		listenfd = socket(AF_INET, SOCK_STREAM, 0);
//		if(listenfd == -1) {
//			printf("ESP8266 TCP server task > socket error!\n");
//			vTaskDelay(1000/portTICK_RATE_MS);
//		}
//	} while(listenfd == -1);
//	printf("ESP8266 TCP server task > create socket: %d\n", listenfd);
//
//	/* Bind to the local port */
//	do {
//		ret = bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
//		if(ret != 0) {
//			printf("ESP8266 TCP server task > bind fail!\n");
//			vTaskDelay(1000/portTICK_RATE_MS);
//		}
//	} while(ret != 0);
//	printf("ESP8266 TCP server task > port: %d\n", ntohs(server_addr.sin_port));
//
//	/* Establish TCP server interception */
//	do {
//		/* Listen to the local connection */
//		ret = listen(listenfd, MAX_CONN);
//		if(ret != 0) {
//			printf("ESP8266 TCP server task > failed to set listen queue!\n");
//			vTaskDelay(1000/portTICK_RATE_MS);
//		}
//	} while(ret != 0);
//	printf("ESP8266 TCP server task > listener OK!\n");
//
//	/* Wait until the TCP client is connected to the server;
//	 then start receiving data packets when the TCP communication is established */
//	int32 client_sock;
//	int32 len = sizeof(struct sockaddr_in);
//	int32 recbytes = 0;
//
//	for(;;) {
//		printf("ESP8266 TCP server task > wait client\n");
//		/* block here waiting remote connect request */
//		if((client_sock = accept(listenfd, (struct sockaddr *)&remote_addr, (socklen_t *)&len)) < 0) {
//			printf("ESP8266 TCP server task > accept fail\n");
//			continue;
//		}
//		printf("ESP8266 TCP server task > Client from %s %d\n",
//				inet_ntoa(remote_addr.sin_addr), htons(remote_addr.sin_port));
//		char *recv_buf = (char *)zalloc(128);
//		while((recbytes = read(client_sock, recv_buf, 128)) > 0) {
//			recv_buf[recbytes] = 0;
//			printf("ESP8266 TCP server task > read data success %d!\n"
//				   "ESP8266 TCP server task > %s\n", recbytes, recv_buf);
//		}
//		free(recv_buf);
//		if(recbytes <= 0) {
//			printf("ESP8266 TCP server task > read data fail\n");
//		}
//	}
//}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
    /* softAP mode */
    wifi_set_opmode(SOFTAP_MODE);

    struct softap_config *config = (struct softap_config *)zalloc(sizeof(struct softap_config)); //initialization.
    wifi_softap_get_config(config);
    sprintf(config->ssid, MY_AP_SSID);
    sprintf(config->password, MY_AP_PASSWD);
    config->authmode = AUTH_WPA_WPA2_PSK;
    config->ssid_len = 0;
    config->max_connection = MAX_CONN;
    wifi_softap_set_config(config);
    free(config);

    http_server_netconn_init();
//    xTaskCreate(tcp_process, "tcp_process", 512, NULL, 2, NULL);
}

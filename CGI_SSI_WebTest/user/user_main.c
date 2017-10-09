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
//#include "httpserver-netconn.h"
#include "uart.h"
#include "espressif/upgrade.h"

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

uint8 fifo_len = 0;
LOCAL uint8 fifo_tmp[20] = {0};

LOCAL void uart0_rx_intr_handler(void *para)
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
            if(strncmp((const char *)fifo_tmp, "start", 5) == 0) {
            	printf("recv 'start' cmd.\n");
            	system_upgrade_flag_set(UPGRADE_FLAG_START);
            } else if(strncmp((const char *)fifo_tmp, "reboot", 6) == 0) {
            	printf("recv 'reboot' cmd.\n");
            	system_upgrade_reboot();
            } else if(strncmp((const char *)fifo_tmp, "finish", 6) == 0) {
            	printf("recv 'finish' cmd.\n");
            	system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
            } else {
            	printf("cmd error!\n");
            }

            WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_RXFIFO_FULL_INT_CLR);
        } else if (UART_RXFIFO_TOUT_INT_ST == (uart_intr_status & UART_RXFIFO_TOUT_INT_ST)) {
        	fifo_len = (READ_PERI_REG(UART_STATUS(uart_no)) >> UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT;
			for(buf_idx = 0; buf_idx < fifo_len; buf_idx ++) {
				fifo_tmp[buf_idx] = READ_PERI_REG(UART_FIFO(uart_no)) & 0xFF;
			}
			if(strncmp((const char *)fifo_tmp, "start", 5) == 0) {
				printf("recv 'start' cmd.\n");
				system_upgrade_flag_set(UPGRADE_FLAG_START);
			} else if(strncmp((const char *)fifo_tmp, "reboot", 6) == 0) {
			    printf("recv 'reboot' cmd.\n");
			    system_upgrade_reboot();
			} else if(strncmp((const char *)fifo_tmp, "finish", 6) == 0) {
				printf("recv 'finish' cmd.\n");
				system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
			} else {
				printf("cmd error!\n");
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

void TaskStart(void *p)
{
	struct softap_config *apconfig = (struct softap_config *)zalloc(sizeof(struct softap_config)); //initialization.
	wifi_softap_get_config(apconfig);
	sprintf(apconfig->ssid, USR_AP_SSID);
	sprintf(apconfig->password, USR_AP_PASSWD);
	apconfig->authmode = AUTH_WPA_WPA2_PSK;
	apconfig->ssid_len = 0;
	apconfig->max_connection = MAX_CONN;
	wifi_softap_set_config(apconfig);
	free(apconfig);

	struct station_config *stconfig = (struct station_config *)zalloc(sizeof(struct station_config));
	sprintf(stconfig->ssid, MY_AP_SSID);
	sprintf(stconfig->password, MY_AP_PASSWD);
	wifi_station_set_config(stconfig);
	free(stconfig);
	wifi_station_set_hostname("kyChu_ESP8266");

	/* Tftpd Init */
	tftpd_init();
	/* Httpd Init */
	httpd_init();
//	http_server_netconn_init();

	vTaskDelete(NULL);
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

	uart_init_new();
	UART_intr_handler_register(uart0_rx_intr_handler, NULL);
	ETS_UART_INTR_ENABLE();

	xTaskCreate(TaskStart, "startTask", 512, NULL, 5, NULL);

    printf("Version Identifier: V0.0.6\n");
}

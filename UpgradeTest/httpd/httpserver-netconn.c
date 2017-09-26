/**
  ******************************************************************************
  * @file    LwIP/LwIP_HTTP_Server_Netconn_RTOS/Src/httpser-netconn.c 
  * @author  MCD Application Team
  * @version V1.2.0
  * @date    30-December-2016
  * @brief   Basic http server implementation using LwIP netconn API  
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics International N.V. 
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "string.h"
#include <stdio.h>
#include "httpserver-netconn.h"
#include "freertos/task.h"
#include "user_config.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define WEBSERVER_THREAD_PRIO    2
#define DATA_BUFF_LEN            512

#define _HTML_INDEX_LEN          1872
#define _HTML_INDEX_ADDR         0x81000
#define _HTML_404_LEN            770
#define _HTML_404_ADDR           0x82000
#define _HTML_LOGO_LEN           4030
#define _HTML_LOGO_ADDR          0x83000
#define _HTML_PNG_LEN            54867
#define _HTML_PNG_ADDR           0x84000

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t *data_buffer;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief serve tcp connection  
  * @param conn: pointer on connection structure 
  * @retval None
  */
static void http_server_serve(struct netconn *conn) 
{
  struct netbuf *inbuf;
  err_t recv_err;
  char* buf;
  u16_t buflen;
  uint32_t file_len;
  uint32_t total_wr;
  
  /* Read the data from the port, blocking if nothing yet there. 
   We assume the request (the part we care about) is in one netbuf */
  recv_err = netconn_recv(conn, &inbuf);
  
  if (recv_err == ERR_OK)
  {
    if (netconn_err(conn) == ERR_OK) 
    {
      netbuf_data(inbuf, (void**)&buf, &buflen);
    
      /* Is this an HTTP GET command? (only check the first 5 chars, since
      there are other formats for GET, and we're keeping it very simple )*/
      if ((buflen >=5) && (strncmp(buf, "GET /", 5) == 0))
      {
        /* Check if request to get header.jpg */
        if (strncmp((char const *)buf,"GET /img/header.png", 19) == 0)
        {
        	/* Check if request to get header.png */
        	data_buffer = (uint8_t *)zalloc(DATA_BUFF_LEN);
			file_len = DATA_BUFF_LEN;
			total_wr = 0;
			while(total_wr < _HTML_PNG_LEN) {
				if(_HTML_PNG_LEN - total_wr < DATA_BUFF_LEN) file_len = _HTML_PNG_LEN - total_wr;
				spi_flash_read(_HTML_PNG_ADDR + total_wr, (uint32_t)data_buffer, file_len);
				netconn_write(conn, (const unsigned char*)data_buffer, (size_t)file_len, NETCONN_NOCOPY);
				total_wr += file_len;
			}
			free(data_buffer);
        }
        else if((strncmp(buf, "GET /index.html", 15) == 0)||(strncmp(buf, "GET / ", 6) == 0))
        {
        	/* Load index page */
        	data_buffer = (uint8_t *)zalloc(DATA_BUFF_LEN);
        	file_len = DATA_BUFF_LEN;
        	total_wr = 0;
        	while(total_wr < _HTML_INDEX_LEN) {
        		if(_HTML_INDEX_LEN - total_wr < DATA_BUFF_LEN) file_len = _HTML_INDEX_LEN - total_wr;
        		spi_flash_read(_HTML_INDEX_ADDR + total_wr, (uint32_t)data_buffer, file_len);
        		netconn_write(conn, (const unsigned char*)data_buffer, (size_t)file_len, NETCONN_NOCOPY);
        		total_wr += file_len;
        	}
        	free(data_buffer);
        }
        else if(strncmp((char const *)buf,"GET /img/logo.ico", 17) == 0)
        {
        	/* Check if request to get logo.ico */
			data_buffer = (uint8_t *)zalloc(DATA_BUFF_LEN);
			file_len = DATA_BUFF_LEN;
			total_wr = 0;
			while(total_wr < _HTML_LOGO_LEN) {
				if(_HTML_LOGO_LEN - total_wr < DATA_BUFF_LEN) file_len = _HTML_LOGO_LEN - total_wr;
				spi_flash_read(_HTML_LOGO_ADDR + total_wr, (uint32_t)data_buffer, file_len);
				netconn_write(conn, (const unsigned char*)data_buffer, (size_t)file_len, NETCONN_NOCOPY);
				total_wr += file_len;
			}
			free(data_buffer);
        }
        else
        {
        	/* Load Error page */
        	data_buffer = (uint8_t *)zalloc(DATA_BUFF_LEN);
			file_len = DATA_BUFF_LEN;
			total_wr = 0;
			while(total_wr < _HTML_404_LEN) {
				if(_HTML_404_LEN - total_wr < DATA_BUFF_LEN) file_len = _HTML_404_LEN - total_wr;
				spi_flash_read(_HTML_404_ADDR + total_wr, (uint32_t)data_buffer, file_len);
				netconn_write(conn, (const unsigned char*)data_buffer, (size_t)file_len, NETCONN_NOCOPY);
				total_wr += file_len;
			}
			free(data_buffer);
        }
      }
    }
  }
  /* Close the connection (server closes in HTTP) */
  netconn_close(conn);

  /* Delete the buffer (netconn_recv gives us ownership,
   so we have to make sure to deallocate the buffer) */
  netbuf_delete(inbuf);
}


/**
  * @brief  http server thread 
  * @param arg: pointer on argument(not used here) 
  * @retval None
  */
static void http_server_netconn_thread(void *arg)
{ 
  struct netconn *conn, *newconn;
  err_t err, accept_err;
  
  /* Create a new TCP connection handle */
  conn = netconn_new(NETCONN_TCP);
  
  if (conn!= NULL)
  {
    /* Bind to port 80 (HTTP) with default IP address */
    err = netconn_bind(conn, NULL, WEB_SERVER_PORT);
    
    if (err == ERR_OK)
    {
      /* Put the connection into LISTEN state */
      netconn_listen(conn);
  
      while(1) 
      {
        /* accept any icoming connection */
        accept_err = netconn_accept(conn, &newconn);
        if(accept_err == ERR_OK)
        {
          /* serve connection */
          http_server_serve(newconn);

          /* delete connection */
          netconn_delete(newconn);
        }
      }
    }
  }
}

/**
  * @brief  Initialize the HTTP server (start its thread) 
  * @param  none
  * @retval None
  */
void http_server_netconn_init()
{
  sys_thread_new("HTTP", http_server_netconn_thread, NULL, 512, WEBSERVER_THREAD_PRIO);
}

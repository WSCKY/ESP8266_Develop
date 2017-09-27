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
#define _HTML_404_LEN            785
#define _HTML_404_ADDR           0x82000
#define _HTML_LOGO_LEN           4030
#define _HTML_LOGO_ADDR          0x83000
#define _HTML_UPLOAD_LEN         1542
#define _HTML_UPLOAD_ADDR        0x84000
#define _HTML_PNG_LEN            54867
#define _HTML_PNG_ADDR           0x85000

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t *data_buffer;
/* Private function prototypes -----------------------------------------------*/
static void write_http_data(struct netconn *conn, uint32_t DataAddr, uint32_t DataLength);
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
  
  if (recv_err == ERR_OK) {
    if (netconn_err(conn) == ERR_OK) {
      netbuf_data(inbuf, (void**)&buf, &buflen);
    
      /* Is this an HTTP GET command? (only check the first 5 chars, since
      there are other formats for GET, and we're keeping it very simple )*/
      /* process HTTP GET requests */
      if ((buflen >=5) && (strncmp(buf, "GET /", 5) == 0)) {
        /* Check if request to get header.jpg */
        if (strncmp((char const *)buf,"GET /img/header.png", 19) == 0) {
        	/* Check if request to get header.png */
        	write_http_data(conn, _HTML_PNG_ADDR, _HTML_PNG_LEN);
        } else if((strncmp(buf, "GET /index.html", 15) == 0)||(strncmp(buf, "GET / ", 6) == 0)) {
        	/* Load index page */
        	write_http_data(conn, _HTML_INDEX_ADDR, _HTML_INDEX_LEN);
        } else if(strncmp((char const *)buf,"GET /img/logo.ico", 17) == 0) {
        	/* Check if request to get logo.ico */
        	write_http_data(conn, _HTML_LOGO_ADDR, _HTML_LOGO_LEN);
        } else {
        	/* Load 404 page */
        	write_http_data(conn, _HTML_404_ADDR, _HTML_404_LEN);
        }
      }
      /* process POST requests */
      else if(strncmp(buf, "POST /", 6) == 0) {
    	  if(strncmp((char const *)buf, "POST /kyChu/login.cgi", 21) == 0) {
    		  printf("got login post.\n");
    		  /* Load upgrade page */
    		  write_http_data(conn, _HTML_UPLOAD_ADDR, _HTML_UPLOAD_LEN);
    	  } else if(strncmp((char const *)buf, "POST /kyChu/print.cgi", 21) == 0) {
    		  for(file_len = buflen - 10; file_len > 0; file_len --) {
    			  if(strncmp((char *)(buf + file_len), "comment=", 8) == 0) {
    				  printf("Print: %s.\n", (char *)(buf + file_len + 8));
    				  break;
    			  }
    		  }
    		  /* Load index page */
    		  write_http_data(conn, _HTML_INDEX_ADDR, _HTML_INDEX_LEN);
    	  } else if(strncmp((char const *)buf, "POST /upgrade/wifi.cgi", 22) == 0) {
    		  printf("wifi: %s", buf);
    		  /* Load index page */
    		  write_http_data(conn, _HTML_INDEX_ADDR, _HTML_INDEX_LEN);
    	  } else if(strncmp((char const *)buf, "POST /upgrade/fc.cgi", 20) == 0) {
    		  printf("fc: %s", buf);
			  /* Load index page */
			  write_http_data(conn, _HTML_INDEX_ADDR, _HTML_INDEX_LEN);
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
  * @brief  send server data to http client.
  * @param  conn: pointer on connection structure.
  * @param  DataAddr: address in flash where store the web data.
  * @param  DataLength: the length of web data.
  * @retval None
  */
static void write_http_data(struct netconn *conn, uint32_t DataAddr, uint32_t DataLength)
{
	uint32_t file_len;
	uint32_t total_wr;

	/* Load index page */
	data_buffer = (uint8_t *)zalloc(DATA_BUFF_LEN);
	file_len = DATA_BUFF_LEN;
	total_wr = 0;
	while(total_wr < DataLength) {
	  if(DataLength - total_wr < DATA_BUFF_LEN) file_len = DataLength - total_wr;
		spi_flash_read(DataAddr + total_wr, (uint32_t)data_buffer, file_len);
		netconn_write(conn, (const unsigned char*)data_buffer, (size_t)file_len, NETCONN_NOCOPY);
		total_wr += file_len;
	}
	free(data_buffer);
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

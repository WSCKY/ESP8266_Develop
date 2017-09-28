/**
  ******************************************************************************
  * @file    httpd/httpser-netconn.c
  * @author  kyChu
  * @version V1.0.0
  * @date    27-September-2017
  * @brief   Basic http server implementation using LwIP netconn API
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/lwip/mem.h"
#include "string.h"
#include <stdio.h>
#include "httpserver-netconn.h"
#include "freertos/task.h"
#include "user_config.h"
#include "lwip/lwip/tcp.h"

/* Private typedef -----------------------------------------------------------*/
struct http_state {
  char *file;
  u32_t left;
};
/* Private define ------------------------------------------------------------*/
#define WEBSERVER_THREAD_PRIO    2

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
static struct tcp_pcb *pcb;

static uint8_t RecPostDataFlag = 0;
static uint8_t BrowserFlag = 0;
static uint32_t ContentLengthOffset = 0;
static uint32_t ContentSize = 0;
static uint32_t TotalReceived = 0;
static uint32_t TotalData = 0;

static const char http_crnl_2[4] =
/* "\r\n--" */
{0xd, 0xa,0x2d,0x2d};
static const char octet_stream[14] =
/* "octet-stream" */
{0x6f, 0x63, 0x74, 0x65, 0x74, 0x2d, 0x73, 0x74, 0x72, 0x65, 0x61, 0x6d,0x0d, };
static const char Content_Length[17] =
/* Content Length */
{0x43, 0x6f, 0x6e, 0x74, 0x65, 0x6e, 0x74, 0x2d, 0x4c, 0x65, 0x6e, 0x67,0x74, 0x68, 0x3a, 0x20, };
/* Private function prototypes -----------------------------------------------*/
static uint32_t Parse_Content_Length(char *data, uint32_t len);
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  callback function for handling connection errors
  * @param  arg: pointer to an argument to be passed to callback function
  * @param  err: LwIP error code
  * @retval None
  */
static void conn_err(void *arg, err_t err)
{
  struct http_state *hs;

  hs = arg;
  mem_free(hs);
}

/**
  * @brief  closes tcp connection
  * @param  pcb: pointer to a tcp_pcb struct
  * @param  hs: pointer to a http_state struct
  * @retval
  */
static void close_conn(struct tcp_pcb *pcb, struct http_state *hs)
{
  tcp_arg(pcb, NULL);
  tcp_sent(pcb, NULL);
  tcp_recv(pcb, NULL);
  mem_free(hs);
  tcp_close(pcb);
}

/**
  * @brief sends data found in  member "file" of a http_state struct
  * @param pcb: pointer to a tcp_pcb struct
  * @param hs: pointer to a http_state struct
  * @retval None
  */
static void send_data(struct tcp_pcb *pcb, struct http_state *hs)
{
  err_t err;
  u16_t len;
  uint8_t *data_buffer;

  /* We cannot send more data than space available in the send
     buffer */
  if (tcp_sndbuf(pcb) < hs->left) {
    len = tcp_sndbuf(pcb);
  } else {
    len = hs->left;
  }
  data_buffer = (uint8_t *)zalloc(len);
  spi_flash_read(hs->file, (uint32_t)data_buffer, len);
  err = tcp_write(pcb, (const uint8_t *)data_buffer, len, 0);
  if (err == ERR_OK) {
    hs->file += len;
    hs->left -= len;
  }
  free(data_buffer);
}

/**
  * @brief tcp poll callback function
  * @param arg: pointer to an argument to be passed to callback function
  * @param pcb: pointer on tcp_pcb structure
  * @retval err_t
  */
static err_t http_poll(void *arg, struct tcp_pcb *pcb)
{
  if (arg == NULL) {
    tcp_close(pcb);
  } else {
    send_data(pcb, (struct http_state *)arg);
  }
  return ERR_OK;
}

/**
  * @brief callback function called after a successfull TCP data packet transmission
  * @param arg: pointer to an argument to be passed to callback function
  * @param pcb: pointer on tcp_pcb structure
  * @param len
  * @retval err : LwIP error code
  */
static err_t http_sent(void *arg, struct tcp_pcb *pcb, u16_t len)
{
  struct http_state *hs;

  hs = arg;

  if (hs->left > 0) {
    send_data(pcb, hs);
  } else {
    close_conn(pcb, hs);
  }
  return ERR_OK;
}

/**
  * @brief serve tcp connection
  * @param conn: pointer on connection structure
  * @retval None
  */
//static err_t http_server_serve(struct netconn *conn)
static err_t http_recv(void *arg, struct tcp_pcb *pcb,  struct pbuf *p, err_t err)
{
  char* data;
  int32_t len = 0;
  uint32_t i = 0;
  char filename[35];
  char *ptr;
  uint32_t DataOffset = 0, FilenameOffset = 0;

  struct http_state *hs;

  hs = arg;

  if (err == ERR_OK && p != NULL) {
	/* Inform TCP that we have taken the data */
	tcp_recved(pcb, p->tot_len);
	if(hs->file == NULL) {

	  data = p->payload;
	  len = p->tot_len;

      /* process HTTP GET requests */
      if (strncmp(data, "GET /", 5) == 0) {
        /* Check if request to get header.jpg */
        if (strncmp((char const *)data,"GET /img/header.png", 19) == 0) {
        	/* Load header.png */
        	hs->file = (char *)_HTML_PNG_ADDR;
        	hs->left = _HTML_PNG_LEN;
        	pbuf_free(p);
        	/* Tell TCP that we wish be to informed of data that has been
        	   successfully sent by a call to the http_sent() function. */
        	tcp_sent(pcb, http_sent);
        } else if((strncmp(data, "GET /index.html", 15) == 0)||(strncmp(data, "GET / ", 6) == 0)) {
        	/* Load index page */
        	hs->file = (char *)_HTML_INDEX_ADDR;
			hs->left = _HTML_INDEX_LEN;
			pbuf_free(p);
			/* Tell TCP that we wish be to informed of data that has been
			   successfully sent by a call to the http_sent() function. */
			tcp_sent(pcb, http_sent);
        } else if(strncmp((char const *)data,"GET /img/logo.ico", 17) == 0) {
        	/* Load logo.ico */
        	hs->file = (char *)_HTML_LOGO_ADDR;
			hs->left = _HTML_LOGO_LEN;
			pbuf_free(p);
			/* Tell TCP that we wish be to informed of data that has been
			   successfully sent by a call to the http_sent() function. */
			tcp_sent(pcb, http_sent);
        } else {
        	/* Load 404 page */
        	hs->file = (char *)_HTML_404_ADDR;
			hs->left = _HTML_404_LEN;
			pbuf_free(p);
			/* Tell TCP that we wish be to informed of data that has been
			   successfully sent by a call to the http_sent() function. */
			tcp_sent(pcb, http_sent);
        }
      }
      /* process POST requests */
      else if(strncmp((char const *)data, "POST /kyChu/login.cgi", 21) == 0) {
    	/* Load upgrade page */
		hs->file = (char *)_HTML_UPLOAD_ADDR;
		hs->left = _HTML_UPLOAD_LEN;
		pbuf_free(p);
		/* Tell TCP that we wish be to informed of data that has been
		   successfully sent by a call to the http_sent() function. */
		tcp_sent(pcb, http_sent);
      } else if(strncmp((char const *)data, "POST /kyChu/print.cgi", 21) == 0) {
    	for(i = len - 10; i > 0; i --) {
    	  if(strncmp((char *)(data + i), "comment=", 8) == 0) {
    		printf("Print: %s.\n", (char *)(data + i + 8));
    		break;
    	  }
    	}
		/* Load index page */
		hs->file = (char *)_HTML_INDEX_ADDR;
		hs->left = _HTML_INDEX_LEN;
		pbuf_free(p);
		/* Tell TCP that we wish be to informed of data that has been
		   successfully sent by a call to the http_sent() function. */
		tcp_sent(pcb, http_sent);
      } else if((strncmp((char const *)data, "POST /upgrade/wifi.cgi", 22) == 0) || RecPostDataFlag >= 1) {
    	DataOffset = 0;
    	if(RecPostDataFlag == 0) {
    	  BrowserFlag = 0;
    	  TotalReceived = 0;
    	  /* parse packet for Content-length field */
    	  ContentSize = Parse_Content_Length(data, len);
    	  printf("content size: %d.\n", ContentSize);

    	  /* parse packet for the octet-stream field */
    	  for (i = 0; i < len; i ++) {
			if (strncmp((char *)(data + i), octet_stream, 13) == 0) {
			  DataOffset = i + 16;
			  break;
			}
    	  }
		  /* case of MSIE8 : we do not receive data in the POST packet*/
		  if (DataOffset == 0) {
			  RecPostDataFlag ++;
			  BrowserFlag = 1;
			  printf("dbg1: %s", (char *)data);
			  pbuf_free(p);
			  return ERR_OK;
		  } else {/* case of Mozilla Firefox : we receive data in the POST packet*/
			  TotalReceived = len - (ContentLengthOffset + 4);
		  }
		}
    	if (((RecPostDataFlag == 1) && (BrowserFlag == 1)) || ((RecPostDataFlag == 0) && (BrowserFlag == 0))) {
    		if ((RecPostDataFlag == 0) && (BrowserFlag == 0)) {
    			RecPostDataFlag ++;
    			printf("<!-- 1 -->\n");
    		} else if((RecPostDataFlag == 1) && (BrowserFlag == 1)) {
    			/* parse packet for the octet-stream field */
    			for (i = 0; i < len; i ++) {
    				if(strncmp((char *)(data + i), octet_stream, 13) == 0) {
    					DataOffset = i + 16;
    					break;
    				}
    			}
    			TotalReceived += len;
    			RecPostDataFlag ++;
//    			printf("octet: %s.\n", octet_stream);
    			printf("dataoff: %d.\n", DataOffset);
//    			printf("dbg2: %s", (char *)data);
    		}
    		/* parse packet for the filename field */
    		FilenameOffset = 0;
    		for(i = 0; i < len; i ++) {
    			if(strncmp((char *)(data + i), "filename=", 9) == 0) {
    				FilenameOffset = i + 10;
    				break;
    			}
    		}
    		i = 0;
    		if(FilenameOffset) {
    			while((*(data + FilenameOffset + i) != 0x22) && (i < 35)) {
    				filename[i] = *(data + FilenameOffset + i);
    				i ++;
    			}
    			filename[i] = 0x0;
    			printf("filename: %s \n", filename);
    		}
    		if(i == 0) {
				/* Load index page */
				hs->file = (char *)_HTML_INDEX_ADDR;
				hs->left = _HTML_INDEX_LEN;
				pbuf_free(p);
				/* Tell TCP that we wish be to informed of data that has been
				   successfully sent by a call to the http_sent() function. */
				tcp_sent(pcb, http_sent);
				RecPostDataFlag = 0;
    		    return ERR_OK;
    		}
    		TotalData = 0;
    		/* -------- */
    		printf("Start Receive data ...\n");
    	} else {
    		/* DataFlag >1 => the packet is data only */
    		TotalReceived += len;
    	}

    	ptr = (char *)(data + DataOffset);
    	len -= DataOffset;
    	/* update Total data received counter */
    	TotalData +=len;

    	if(TotalReceived == ContentSize) {
    		/* if last packet need to remove the http boundary tag */
    		/* parse packet for "\r\n--" starting from end of data */
    		i = 4;
    		while(strncmp((char *)(data + p->tot_len - i), http_crnl_2, 4)) {
    			i ++;
    		}
    		len -= i;
    		TotalData -= i;

    		/* write data in Flash */
//    		if (len) IAP_HTTP_writedata(ptr, len);
    		printf("end: %s...", (char *)ptr);
    		RecPostDataFlag = 0;
    		printf("%d bytes received.\n", TotalData);
    		/* Load index page */
    		hs->file = (char *)_HTML_INDEX_ADDR;
    		hs->left = _HTML_INDEX_LEN;
    		pbuf_free(p);
    		/* Tell TCP that we wish be to informed of data that has been
    		   successfully sent by a call to the http_sent() function. */
    		tcp_sent(pcb, http_sent);
    	} else {
    		/* not last data packet */
    		/* write data in flash */
//    		if(len) IAP_HTTP_writedata(ptr, len);
    		printf("fd: %s", (char *)ptr);
    	}
    	pbuf_free(p);
      } else if(strncmp((char const *)data, "POST /upgrade/fc.cgi", 20) == 0) {
    	  printf("fc: %s", data);
    	  /* Load index page */
    	  hs->file = (char *)_HTML_INDEX_ADDR;
    	  hs->left = _HTML_INDEX_LEN;
    	  pbuf_free(p);
    	  /* Tell TCP that we wish be to informed of data that has been
    	     successfully sent by a call to the http_sent() function. */
    	  tcp_sent(pcb, http_sent);
      } else {
    	  /* Bad HTTP request\n */
    	  close_conn(pcb, hs);
      }
	} else {
		pbuf_free(p);
		close_conn(pcb, hs);
	}
  }
  if(err == ERR_OK && p == NULL) {
	  /* received empty frame */
      close_conn(pcb, hs);
  }
  return ERR_OK;
}

/**
  * @brief  Extract the Content_Length data from HTML data
  * @param  data : pointer on receive packet buffer
  * @param  len  : buffer length
  * @retval size : Content_length in numeric format
  */
static uint32_t Parse_Content_Length(char *data, uint32_t len)
{
  uint32_t i = 0, size = 0, S = 1;
  int32_t j = 0;
  char sizestring[6], *ptr;

  ContentLengthOffset = 0;

  /* find Content-Length data in packet buffer */
  for (i = len - 17; i > 0; i --) {
    if (strncmp((char *)(data + i), Content_Length, 16) == 0) {
      ContentLengthOffset = i + 16;
      break;
    }
  }
  /* read Content-Length value */
  if (ContentLengthOffset) {
    i = 0;
    ptr = (char *)(data + ContentLengthOffset);
    while(*(ptr + i) != 0x0d) {
      sizestring[i] = *(ptr + i);
      i ++;
      ContentLengthOffset ++;
    }
    if (i > 0) {
      /* transform string data into numeric format */
      for(j = i - 1; j >= 0; j --) {
        size += (sizestring[j] - '0') * S;
        S = S * 10;
      }
    }
  }
  return size;
}

/**
  * @brief  callback function on TCP connection setup ( on port 80)
  * @param  arg: pointer to an argument structure to be passed to callback function
  * @param  pcb: pointer to a tcp_pcb structure
  * &param  err: Lwip stack error code
  * @retval err
  */
static err_t http_accept(void *arg, struct tcp_pcb *pcb, err_t err)
{
  struct http_state *hs;

  /* Allocate memory for the structure that holds the state of the connection */
  hs = (struct http_state *)mem_malloc(sizeof(struct http_state));

  if (hs == NULL)
  {
    return ERR_MEM;
  }

  /* Initialize the structure. */
  hs->file = NULL;
  hs->left = 0;

  /* Tell TCP that this is the structure we wish to be passed for our
     callbacks. */
  tcp_arg(pcb, hs);

  /* Tell TCP that we wish to be informed of incoming data by a call
     to the http_recv() function. */
  tcp_recv(pcb, http_recv);

  tcp_err(pcb, conn_err);

  tcp_poll(pcb, http_poll, 10);
  return ERR_OK;
}

/**
  * @brief  intialize HTTP webserver
  * @param arg: pointer on argument(not used here)
  * @retval None
  */
static void http_server_netconn_thread(void *arg)
{
	/*create new pcb*/
	pcb = (struct tcp_pcb *)tcp_new();
	/* bind HTTP traffic to pcb */
	tcp_bind(pcb, IP_ADDR_ANY, WEB_SERVER_PORT);
	/* start listening on port 80 */
	pcb = (struct tcp_pcb *)tcp_listen(pcb);
	/* define callback function for TCP connection setup */
	tcp_accept(pcb, http_accept);
	vTaskDelete(NULL);
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

#include "httpd.h"
#include "lwip/tcp.h"

const char* TAGS[] = {"echo", "here"};
static char* display_1 = " echo: hi, world! - 0<br />";
static char* display_2 = " here: hi, world! - 0<br />";

static uint8_t req_cnt_1 = 0;
static uint8_t req_cnt_2 = 0;

u16_t StringHandler(int iIndex, char *pcInsert, int iInsertLen)
{
	uint8_t i = 0;
//	printf("handler process.\n");
	if(iIndex == 0) {
		for(i = 0; i < 28; i ++)
			pcInsert[i] = display_1[i];

		pcInsert[20] += req_cnt_1;
		req_cnt_1 ++;
//		printf("insert %s.\n", pcInsert);
		return 28;
	} else if(iIndex == 1) {
		for(i = 0; i < 28; i ++)
			pcInsert[i] = display_2[i];

		pcInsert[20] += req_cnt_2;
		req_cnt_2 ++;
//		printf("insert %s.\n", pcInsert);
		return 28;
	}
	return 0;
}

void Httpd_cgi_ssi_init(void)
{
	http_set_ssi_handler(StringHandler, (char const **)TAGS, 2);
}

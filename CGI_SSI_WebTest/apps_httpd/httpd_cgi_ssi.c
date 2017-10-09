#include "httpd.h"
#include "lwip/tcp.h"

const char* TAGCHAR="echo";
const char** TAGS = &TAGCHAR;
static char* display = "<br />hi, i'm kychu! - 0<br />";

u16_t StringHandler(int iIndex, char *pcInsert, int iInsertLen)
{
	uint8_t i = 0;
//	printf("handler process.\n");
	if(iIndex == 0) {
		for(i = 0; i < 31; i ++)
			pcInsert[i] = display[i];

		display[23] ++;
		return 31;
	}
}

void Httpd_cgi_ssi_init(void)
{
	http_set_ssi_handler(StringHandler, (char const **)TAGS, 1);
}

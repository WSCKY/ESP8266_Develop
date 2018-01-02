#ifndef __FUTABA_H
#define __FUTABA_H

#include "esp_common.h"

#define RC_MIDDLE_VALUE             1024

typedef struct
{
	uint8_t startByte;
	union
	{
		uint16_t data8[22];
		struct
		{
			uint16_t data1 :11;
			uint16_t data2 :11;
			uint16_t data3 :11;
			uint16_t data4 :11;
			uint16_t data5 :11;
			uint16_t data6 :11;
			uint16_t data7 :11;
			uint16_t data8 :11;
			uint16_t data9 :11;
			uint16_t data10 :11;
			uint16_t data11 :11;
			uint16_t data12 :11;
			uint16_t data13 :11;
			uint16_t data14 :11;
			uint16_t data15 :11;
			uint16_t data16 :11;
		} data11;
	} rf_data;
	uint8_t flags;
	uint8_t endByte;
} RC_DATA_t;

typedef struct {
	uint16_t Channel[16];
} RC_CHANNLE_t;

uint8_t GetRCUpdateFlag(void);
RC_CHANNLE_t *GetRC_ChannelData(void);
void RC_ProcHandler(uint8_t *p, uint8_t len);
void RC_ParseData(void);

#endif /* __FUTABA_H */

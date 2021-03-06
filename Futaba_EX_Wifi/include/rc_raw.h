#ifndef __RC_RAW_H
#define __RC_RAW_H

#include "esp_common.h"

#define RC_CHANNEL_NUMBER      12

#define PACKET_TOTAL_LENGTH    ((RC_CHANNEL_NUMBER << 1) + 5)
#define PACKET_DATA_LENGTH     (PACKET_TOTAL_LENGTH - 3)
#define PACKET_CRC8_LENGTH     (PACKET_TOTAL_LENGTH - 3)

typedef struct {
	uint8_t stx1;
	uint8_t stx2;
	uint8_t len;
	uint8_t type;
	uint16_t Channel[RC_CHANNEL_NUMBER];
	uint8_t crc;
} PackageStructDef;

typedef union {
	PackageStructDef PackData;
	uint8_t RawData[PACKET_TOTAL_LENGTH];
} RawWifiRcDataDef;

#define CRC8_INIT            (0x66)

#endif /* __RC_RAW_H */

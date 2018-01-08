#ifndef __CRC8_H
#define __CRC8_H

#include "esp_common.h"

uint8_t Get_CRC8_Check_Sum(uint8_t *pchMessage, uint32_t dwLength, uint8_t ucCRC8);

#endif /* __CRC8_H */

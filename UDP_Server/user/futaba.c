#include "futaba.h"

static RC_DATA_t RC_Data = {0};
static uint8_t RCDataUpdate = 0;
static RC_CHANNLE_t RC_ChannelData = {0};

static uint8_t _data_step = 0;
static uint8_t _rx_data_cnt = 0;
static RC_DATA_t _rc_data = {0};

static void _rc_data_decode(uint8_t data)
{
	switch(_data_step) {
		case 0: {
			if(data == 0x0F) {
				_rc_data.startByte = data;
				_rx_data_cnt = 0;
				_data_step = 1;
			}
		} break;
		case 1: {
			_rc_data.rf_data.data8[_rx_data_cnt] = data;
			_rx_data_cnt ++;
			if(_rx_data_cnt >= 22)
				_data_step = 2;
		} break;
		case 2: {
			_rc_data.flags = data;
			_data_step = 3;
		} break;
		case 3: {
			_data_step = 0;
			_rc_data.endByte = data;
			if(data == 0x04 || data == 0x14 || data == 0x24 || data == 0x34) {
				if((_rc_data.flags & 0x0C) != 0x04 && (_rc_data.flags & 0x0C) != 0x0C) { //useless data.
					RC_Data = _rc_data;
					RCDataUpdate = 1;
				}
			}
		} break;
		default: _data_step = 0; break;
	}
}

uint8_t GetRCUpdateFlag(void)
{
	if(RCDataUpdate) {
		RCDataUpdate = 0;
		return 1;
	}
	return 0;
}

RC_CHANNLE_t *GetRC_ChannelData(void)
{
	return &RC_ChannelData;
}

void RC_ProcHandler(uint8_t *p, uint8_t len)
{
	uint8_t i = 0;
	for(; i < len; i ++) {
		_rc_data_decode(*(p + i));
	}
}

void RC_ParseData(void)
{
	RC_ChannelData.Channel[0]  = ((RC_Data.rf_data.data8[0] | RC_Data.rf_data.data8[1] << 8) & 0x07FF);
	RC_ChannelData.Channel[1]  = ((RC_Data.rf_data.data8[1] >> 3 | RC_Data.rf_data.data8[2] << 5) & 0x07FF);
	RC_ChannelData.Channel[2]  = ((RC_Data.rf_data.data8[2] >> 6 | RC_Data.rf_data.data8[3] << 2 | RC_Data.rf_data.data8[4] << 10) & 0x07FF);
	RC_ChannelData.Channel[3]  = ((RC_Data.rf_data.data8[4] >> 1 | RC_Data.rf_data.data8[5] << 7) & 0x07FF);
	RC_ChannelData.Channel[4]  = ((RC_Data.rf_data.data8[5] >> 4 | RC_Data.rf_data.data8[6] << 4) & 0x07FF);
	RC_ChannelData.Channel[5]  = ((RC_Data.rf_data.data8[6] >> 7 | RC_Data.rf_data.data8[7] << 1 | RC_Data.rf_data.data8[8] << 9) & 0x07FF);
	RC_ChannelData.Channel[6]  = ((RC_Data.rf_data.data8[8] >> 2 | RC_Data.rf_data.data8[9] << 6) & 0x07FF);
	RC_ChannelData.Channel[7]  = ((RC_Data.rf_data.data8[9] >> 5 | RC_Data.rf_data.data8[10]<<3) & 0x07FF); // & the other 8 + 2 channels if you need them
	RC_ChannelData.Channel[8]  = ((RC_Data.rf_data.data8[11] | RC_Data.rf_data.data8[12] << 8) & 0x07FF);
	RC_ChannelData.Channel[9]  = ((RC_Data.rf_data.data8[12] >> 3 | RC_Data.rf_data.data8[13] << 5) & 0x07FF);
	RC_ChannelData.Channel[10] = ((RC_Data.rf_data.data8[13] >> 6 | RC_Data.rf_data.data8[14] << 2 | RC_Data.rf_data.data8[15] << 10) & 0x07FF);
	RC_ChannelData.Channel[11] = ((RC_Data.rf_data.data8[15] >> 1 | RC_Data.rf_data.data8[16] << 7) & 0x07FF);
	RC_ChannelData.Channel[12] = ((RC_Data.rf_data.data8[16] >> 4 | RC_Data.rf_data.data8[17] << 4) & 0x07FF);
	RC_ChannelData.Channel[13] = ((RC_Data.rf_data.data8[17] >> 7 | RC_Data.rf_data.data8[18] << 1 | RC_Data.rf_data.data8[19] << 9) & 0x07FF);
	RC_ChannelData.Channel[14] = ((RC_Data.rf_data.data8[19] >> 2 | RC_Data.rf_data.data8[20] << 6) & 0x07FF);
	RC_ChannelData.Channel[15] = ((RC_Data.rf_data.data8[20] >> 5 | RC_Data.rf_data.data8[21] << 3) & 0x07FF);
}

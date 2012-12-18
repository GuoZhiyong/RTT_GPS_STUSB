#ifndef _H_JT808_H_
#define _H_JT808_H_

#include <stm32f4xx.h>

/*字节顺序的定义网络顺序*/
typedef struct
{
	uint32_t latitude; 		/*纬度 1/10000分 */
	uint32_t longitude; 	/*经度 1/10000分 */
	uint16_t altitude;		/*高程 m*/
	uint16_t speed;			/*速度 1/10KMH*/
	uint8_t direction;		/*方向 0-178 刻度为2度*/
	uint8_t datetime[6];	/*YY-MM-DD hh-mm-ss BCD编码*/
}T_GPSINFO;


void gps_rx(uint8_t *pinfo,uint16_t length);
void gprs_rx(uint8_t *pinfo,uint16_t length);


extern uint32_t jt808_alarm;
extern uint32_t jt808_status;

#endif



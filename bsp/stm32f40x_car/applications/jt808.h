#ifndef _H_JT808_H_
#define _H_JT808_H_

#include <stm32f4xx.h>

/*�ֽ�˳��Ķ�������˳��*/
typedef struct
{
	uint32_t latitude; 		/*γ�� 1/10000�� */
	uint32_t longitude; 	/*���� 1/10000�� */
	uint16_t altitude;		/*�߳� m*/
	uint16_t speed;			/*�ٶ� 1/10KMH*/
	uint8_t direction;		/*���� 0-178 �̶�Ϊ2��*/
	uint8_t datetime[6];	/*YY-MM-DD hh-mm-ss BCD����*/
}T_GPSINFO;


void gps_rx(uint8_t *pinfo,uint16_t length);
void gprs_rx(uint8_t *pinfo,uint16_t length);


extern uint32_t jt808_alarm;
extern uint32_t jt808_status;

#endif



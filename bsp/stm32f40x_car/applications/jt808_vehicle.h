/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		// �ļ���
 * Author:			// ����
 * Date:			// ����
 * Description:		// ģ������
 * Version:			// �汾��Ϣ
 * Function List:	// ��Ҫ�������书��
 *     1. -------
 * History:			// ��ʷ�޸ļ�¼
 *     <author>  <time>   <version >   <desc>
 *     David    96/10/12     1.0     build this moudle
 ***********************************************************/
#ifndef _H_JT808_VEHICLE_
#define _H_JT808_VEHICLE_

#include "stm32f4xx.h"
#include "jt808_util.h"

/*�ⲿIO��*/
typedef struct _auxio_out
{
	GPIO_TypeDef* port;
	uint16_t	pin;
	uint8_t		value;                      /*��ǰ�˿�ֵ*/
	void ( *onchange )(uint8_t,uint8_t);    /*�˿ڱ仯������*/
}AUX_OUT;

typedef struct _auxio_in
{
	GPIO_TypeDef* port;
	uint16_t	pin;
	uint8_t		value;                      /*��ǰ�˿�ֵ*/
	uint32_t	dithering_threshold;        /*���ޣ�����ȥ������50ms���һ��*/
	uint8_t		dithering_count;            /*��������*/
	uint32_t	duration;                   /*��ǰ״̬�ĳ���ʱ��*/
	void ( *onchange )(uint8_t,uint8_t);          /*�˿ڱ仯������*/
}AUX_IN;



#define SPEED_STATUS_ACC	0x01            /*acc״̬ 0:�� 1:��*/
#define SPEED_STATUS_BRAKE	0x02            /*ɲ��״̬ 0:�� 1:��*/

#define SPEED_JUDGE_ACC		0x04            /*�Ƿ��ж�ACC*/
#define SPEED_JUDGE_BRAKE	0x08            /*�Ƿ��ж�BRAKE ɲ���ź�*/

#define SPEED_USE_PULSE 0x10                /*ʹ�������ź� 0:��ʹ�� 1:ʹ��*/
#define SPEED_USE_GPS	0x20                /*ʹ��gps�ź� 0:��ʹ�� 1:ʹ��*/




extern uint32_t	Frequency;

extern uint32_t AD_Volte;;
extern uint32_t AD_Volte_Min,AD_Volte_Max;


extern AUX_IN	PIN_IN[10];

void jt808_vehicle_init( void );


#endif

/************************************** The End Of File **************************************/

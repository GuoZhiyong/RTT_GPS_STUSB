/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		// 文件名
 * Author:			// 作者
 * Date:			// 日期
 * Description:		// 模块描述
 * Version:			// 版本信息
 * Function List:	// 主要函数及其功能
 *     1. -------
 * History:			// 历史修改记录
 *     <author>  <time>   <version >   <desc>
 *     David    96/10/12     1.0     build this moudle
 ***********************************************************/
#ifndef _H_JT808_VEHICLE_
#define _H_JT808_VEHICLE_

#include "stm32f4xx.h"
#include "jt808_util.h"

/*外部IO口*/
typedef struct _auxio_out
{
	GPIO_TypeDef* port;
	uint16_t	pin;
	uint8_t		value;                      /*当前端口值*/
	void ( *onchange )(uint8_t,uint8_t);    /*端口变化处理函数*/
}AUX_OUT;

typedef struct _auxio_in
{
	GPIO_TypeDef* port;
	uint16_t	pin;
	uint8_t		value;                      /*当前端口值*/
	uint32_t	dithering_threshold;        /*门限，用作去抖动，50ms检查一次*/
	uint8_t		dithering_count;            /*抖动计数*/
	uint32_t	duration;                   /*当前状态的持续时间*/
	void ( *onchange )(uint8_t,uint8_t);          /*端口变化处理函数*/
}AUX_IN;



#define SPEED_STATUS_ACC	0x01            /*acc状态 0:关 1:开*/
#define SPEED_STATUS_BRAKE	0x02            /*刹车状态 0:关 1:开*/

#define SPEED_JUDGE_ACC		0x04            /*是否判断ACC*/
#define SPEED_JUDGE_BRAKE	0x08            /*是否判断BRAKE 刹车信号*/

#define SPEED_USE_PULSE 0x10                /*使用脉冲信号 0:不使用 1:使用*/
#define SPEED_USE_GPS	0x20                /*使用gps信号 0:不使用 1:使用*/




extern uint32_t	Frequency;

extern uint32_t AD_Volte;;
extern uint32_t AD_Volte_Min,AD_Volte_Max;


extern AUX_IN	PIN_IN[10];

void jt808_vehicle_init( void );


#endif

/************************************** The End Of File **************************************/

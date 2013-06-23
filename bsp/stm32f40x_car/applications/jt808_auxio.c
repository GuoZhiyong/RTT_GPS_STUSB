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

#include "jt808_gps.h"
#include "stm32f4xx.h"
#include <rtthread.h>

/*外部IO口*/
typedef struct _auxio_out
{
	GPIO_TypeDef* port;
	uint16_t	pin;
	uint8_t		value;                      /*当前端口值*/
	void ( *onchange )( uint8_t value );    /*端口变化处理函数*/
}AUX_OUT;

/*紧急情况的处理*/
void onemg( uint8_t value )
{
	if( value )
	{
		jt808_alarm |= BIT_ALARM_EMG;
	} else
	{
		jt808_alarm &= ~BIT_ALARM_EMG;
	}
}

/*ACC状态变化*/
void onacc( uint8_t value )
{
	if( value )
	{
		jt808_status |= BIT_STATUS_ACC;
	}else
	{
		jt808_status &= ~BIT_STATUS_ACC;
	}
}

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void ondefault( uint8_t value )
{


}

typedef struct _auxio_in
{
	GPIO_TypeDef* port;
	uint16_t	pin;
	uint8_t		value;                              /*当前端口值*/
	uint32_t	dithering_threshold;                /*门限，用作去抖动，50ms检查一次*/
	uint8_t		dithering_count;                    /*抖动计数*/
	uint32_t	duration;                           /*当前状态的持续时间*/
	void ( *onchange )( uint8_t );                  /*端口变化处理函数*/
}AUX_IN;

AUX_IN	PIN_IN[10] = {
	{ GPIOE, GPIO_Pin_8,  0, 40, 0, 0, onemg	 }, /*紧急按钮*/
	{ GPIOE, GPIO_Pin_9,  0, 40, 0, 0, onacc	 }, /*ACC*/
	{ GPIOE, GPIO_Pin_7,  0, 0,	 0, 0, ondefault }, /*输入*/
	{ GPIOC, GPIO_Pin_0,  0, 0,	 0, 0, ondefault }, /*4.远光*/
	{ GPIOC, GPIO_Pin_1,  0, 0,	 0, 0, ondefault }, /*5.车门*/
	{ GPIOA, GPIO_Pin_1,  0, 0,	 0, 0, ondefault }, /*6.喇叭 定义为AD输入*/
	{ GPIOC, GPIO_Pin_3,  0, 0,	 0, 0, ondefault }, /*7.左转 定义为AD输入*/
	{ GPIOC, GPIO_Pin_2,  0, 0,	 0, 0, ondefault }, /*8.右转*/
	{ GPIOE, GPIO_Pin_11, 0, 0,	 0, 0, ondefault }, /*9.刹车*/
	{ GPIOE, GPIO_Pin_10, 0, 0,	 0, 0, ondefault }, /*10.雨刷*/
};

AUX_OUT PIN_OUT[] = {
	{ GPIOB, GPIO_Pin_1, 0, 0 },                    /*继电器*/
	{ GPIOB, GPIO_Pin_6, 0, 0 },                    /*蜂鸣器*/
};


/*
   读取输入口状态
   增加去抖

   要不要传递进来tick值?

 */
void auxio_input_check( void)
{
	int		i;
	uint8_t st;
	for( i = 0; i < sizeof( PIN_IN ) / sizeof( AUX_IN ); i++ )
	{
		st = GPIO_ReadInputDataBit( PIN_IN[i].port, PIN_IN[i].pin );
		if( st ^ PIN_IN[i].value )                      /*值不同,有变化*/
		{
			if( PIN_IN[i].dithering_threshold == 0 )    /*不判门限*/
			{
				PIN_IN[i].value = st;
				PIN_IN[i].onchange( st );               /*调用处理函数*/
			}else
			{
				PIN_IN[i].dithering_count++;
				if( PIN_IN[i].dithering_count == PIN_IN[i].dithering_threshold )
				{
					PIN_IN[i].value = st;
					PIN_IN[i].onchange( st );           /*调用处理函数*/
					PIN_IN[i].dithering_count = 0;
				}
			}
		}else
		{
			PIN_IN[i].duration++;
		}
	}
}

/*
   配置外部的输入输出口
   todo:是否导入到存储中，便于灵活配置

 */
void auxio_init( void )
{
	GPIO_InitTypeDef	GPIO_InitStructure;
	int					i;



	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA, ENABLE );
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOB, ENABLE );
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOC, ENABLE );
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOD, ENABLE );
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOE, ENABLE );

	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_UP;

	for( i = 0; i < sizeof( PIN_IN ) / sizeof( AUX_IN ); i++ )
	{
		GPIO_InitStructure.GPIO_Pin = PIN_IN[i].pin;
		GPIO_Init( PIN_IN[i].port, &GPIO_InitStructure );
	}
}

/************************************** The End Of File **************************************/

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
#include "jt808_vehicle.h"
#include "jt808_param.h"


uint8_t car_stop_run=0;		/*车辆启动停止状态*/



/*声明一个定时器，用来定时检查AUX*/
struct rt_timer tmr_50ms;



/*紧急情况的处理*/
void onemg( uint8_t value )
{
	if( value )
	{
		jt808_alarm &= ~BIT_ALARM_EMG;
	} else
	{
		jt808_alarm |= BIT_ALARM_EMG;
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

AUX_IN	PIN_IN[10] = {
	{ GPIOE, GPIO_Pin_8,  0, 20, 0, 0, onemg	 }, /*紧急按钮*/
	{ GPIOE, GPIO_Pin_9,  0, 20, 0, 0, onacc	 }, /*ACC*/
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

/*外接车速信号*/
__IO uint16_t	IC2Value	= 0;
__IO uint16_t	DutyCycle	= 0;
__IO uint32_t	Frequency	= 0;







/*
   读取输入口状态
   增加去抖

   要不要传递进来tick值?

 */
static void cb_tmr_50ms( void* parameter )
{
	uint8_t	i;
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


/*TIM5_CH1,脉冲判断速度*/
void TIM5_IRQHandler( void )
{
	RCC_ClocksTypeDef RCC_Clocks;
	RCC_GetClocksFreq( &RCC_Clocks );

	TIM_ClearITPendingBit( TIM5, TIM_IT_CC2 );

	/* Get the Input Capture value */
	IC2Value = TIM_GetCapture2( TIM5 );

	if( IC2Value != 0 )
	{
		/* Duty cycle computation */
		//DutyCycle = ( TIM_GetCapture1( TIM5 ) * 100 ) / IC2Value;
		/* Frequency computation   TIM4 counter clock = (RCC_Clocks.HCLK_Frequency)/2 */
		//Frequency = (RCC_Clocks.HCLK_Frequency)/2 / IC2Value;
/*是不是反向电路?*/
		DutyCycle	= ( IC2Value * 100 ) / TIM_GetCapture1( TIM5 );
		Frequency	= ( RCC_Clocks.HCLK_Frequency ) / 2 / TIM_GetCapture1( TIM5 );
	}else
	{
		DutyCycle	= 0;
		Frequency	= 0;
	}
}

/*采用PA.0 作为外部脉冲计数*/
void pulse_init( void )
{
	GPIO_InitTypeDef	GPIO_InitStructure;
	NVIC_InitTypeDef	NVIC_InitStructure;
	TIM_ICInitTypeDef	TIM_ICInitStructure;

	/* TIM5 clock enable */
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_TIM5, ENABLE );

	/* GPIOA clock enable */
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA, ENABLE );

	/* TIM5 chennel1 configuration : PA.0 */
	GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_UP;
	GPIO_Init( GPIOA, &GPIO_InitStructure );

	/* Connect TIM pin to AF0 */
	GPIO_PinAFConfig( GPIOA, GPIO_PinSource0, GPIO_AF_TIM5 );

	/* Enable the TIM5 global Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel						= TIM5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority	= 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority			= 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd					= ENABLE;
	NVIC_Init( &NVIC_InitStructure );

	TIM_ICInitStructure.TIM_Channel		= TIM_Channel_1;
	TIM_ICInitStructure.TIM_ICPolarity	= TIM_ICPolarity_Rising;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter	= 0x0;

	TIM_PWMIConfig( TIM5, &TIM_ICInitStructure );

	/* Select the TIM5 Input Trigger: TI1FP1 */
	TIM_SelectInputTrigger( TIM5, TIM_TS_TI1FP1 );

	/* Select the slave Mode: Reset Mode */
	TIM_SelectSlaveMode( TIM5, TIM_SlaveMode_Reset );
	TIM_SelectMasterSlaveMode( TIM5, TIM_MasterSlaveMode_Enable );

	/* TIM enable counter */
	TIM_Cmd( TIM5, ENABLE );

	/* Enable the CC2 Interrupt Request */
	TIM_ITConfig( TIM5, TIM_IT_CC2, ENABLE );
}



/*
   配置外部的输入输出口
   todo:是否导入到存储中，便于灵活配置

 */
void jt808_vehicle_init( void )
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

	rt_timer_init( &tmr_50ms, "tmr_50ms",       /* 定时器名字是 tmr_gps */
	               cb_tmr_50ms,                 /* 超时时回调的处理函数 */
	               RT_NULL,                     /* 超时函数的入口参数 */
	               RT_TICK_PER_SECOND / 20,     /* 定时长度，以OS Tick为单位 */
	               RT_TIMER_FLAG_PERIODIC );    /* 周期性定时器 */

	rt_timer_start( &tmr_50ms );
	pulse_init( );                              /*接脉冲计数*/
}

/************************************** The End Of File **************************************/

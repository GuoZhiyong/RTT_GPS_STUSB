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
#include "jt808.h"
#include "jt808_vehicle.h"
#include "jt808_param.h"

uint8_t car_stop_run = 0;  /*车辆启动停止状态*/

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
	//{ GPIOA, GPIO_Pin_1,  0, 0,	 0, 0, ondefault }, /*6.喇叭 定义为AD输入*/
	//{ GPIOC, GPIO_Pin_3,  0, 0,	 0, 0, ondefault }, /*7.左转 定义为AD输入*/
	{ GPIOC, GPIO_Pin_2,  0, 0,	 0, 0, ondefault }, /*8.右转*/
	{ GPIOE, GPIO_Pin_11, 0, 0,	 0, 0, ondefault }, /*9.刹车*/
	{ GPIOE, GPIO_Pin_10, 0, 0,	 0, 0, ondefault }, /*10.雨刷*/
};

AUX_OUT PIN_OUT[] = {
	{ GPIOB, GPIO_Pin_1, 0, 0 },                    /*继电器*/
//	{ GPIOB, GPIO_Pin_6, 0, 0 },                    /*蜂鸣器*/
};

/*外接车速信号*/
__IO uint16_t	IC2Value	= 0;
__IO uint16_t	DutyCycle	= 0;
__IO uint32_t	Frequency	= 0;

#define ADC1_DR_Address			( (uint32_t)0X4001204C )
#define    BD_IO_Pin6_7_A1C3                //  北斗应用用 PA1    6   灰线 PC3   7  绿线

uint16_t		ADC_ConValue[3];            //   3  个通道ID    0 : 电池 1: 灰线   2:  绿线

static uint32_t ADC_ConvertedValue	= 0;    //电池电压AD数值
uint32_t AD_Volte			= 0;
static uint32_t AD_2through[2];             //  另外2 路AD 的数值

static uint8_t	power_lost_counter	= 0;
static uint8_t	power_low_counter	= 0;


/*
   读取输入口状态
   增加去抖

   要不要传递进来tick值?

 */
static void cb_tmr_50ms( void* parameter )
{
	uint8_t i;
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

	ADC_ConvertedValue	= ADC_ConValue[0]; //ADC_GetConversionValue(ADC1);
	AD_Volte			= ( ( ADC_ConvertedValue * 543 ) >> 12 );
	//rt_kprintf ("\r\n  获取到的电池AD数值为:	%d	 AD电压为: %d V  电源电压: %d V\r\n",ADC_ConvertedValue,a,a+11);
	//  ---电源欠压报警----
	AD_Volte = AD_Volte + 11 + 10;

	//  -----	另外2 路  AD 的采集电压值转换
	// 1 .through	1  Voltage Value
	AD_2through[0]	= ( ( ( ADC_ConValue[1] - 70 ) * 543 ) >> 12 );
	AD_2through[0]	= AD_2through[0] + 11 + 10;
	AD_2through[0]	= AD_2through[0] * 100; // mV
	// 2 .through	2  Voltage Value
	AD_2through[1]	= ( ( ( ADC_ConValue[2] - 70 ) * 543 ) >> 12 );
	AD_2through[1]	= AD_2through[1] + 11 + 10;
	AD_2through[1]	= AD_2through[1] * 100;

	if( ADC_ConvertedValue < 500 )          //  小于500 认为是外部断电
	{
		power_lost_counter++;
		if( power_lost_counter == 40 )      /*20次 持续2s*/
		{
			if( ( jt808_alarm & BIT_ALARM_LOST_PWR ) == 0 )
			{
				rt_kprintf( "\nAUX>主电源掉电!" );
				jt808_alarm |= BIT_ALARM_LOST_PWR;
			}
		}
	}else /*电源正常情况下*/
	{
		power_lost_counter = 0;
		if( jt808_alarm & BIT_ALARM_LOST_PWR ) /*已经掉电，又恢复了*/
		{
			rt_kprintf( "\n主电源正常!" );
			jt808_alarm &= ~BIT_ALARM_LOST_PWR;
		}
	}
	//------------判断欠压和正常-----
	if( AD_Volte < 100 ) // 16V		<->  160
	{
		power_low_counter++;
		if( power_low_counter == 20 )
		{
			if( ( jt808_alarm & BIT_ALARM_LOW_PWR ) == 0 )
			{
				jt808_alarm |= BIT_ALARM_LOW_PWR;
				rt_kprintf( "\n 欠压报警!" );
			}
		}
	}else
	{
		power_low_counter = 0;
		if( jt808_alarm & BIT_ALARM_LOW_PWR )
		{
			jt808_alarm &=~BIT_ALARM_LOW_PWR;
			rt_kprintf( "\n从欠压中还原正常!" );
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

/**/
void ad_init( void )
{
	ADC_InitTypeDef			ADC_InitStructure;
	GPIO_InitTypeDef		gpio_init;
	ADC_CommonInitTypeDef	ADC_CommonInitStructure;
	DMA_InitTypeDef			DMA_InitStructure;

	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_DMA2 | RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOC, ENABLE );
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_ADC1, ENABLE );

	gpio_init.GPIO_Mode = GPIO_Mode_AIN;
	gpio_init.GPIO_Pin	= GPIO_Pin_5;
	GPIO_Init( GPIOC, &gpio_init );

	gpio_init.GPIO_Pin = GPIO_Pin_1;
	GPIO_Init( GPIOA, &gpio_init );

	gpio_init.GPIO_Pin = GPIO_Pin_3;
	GPIO_Init( GPIOC, &gpio_init );

//  3. ADC Common Init
	/* ADC Common configuration *************************************************/
	ADC_CommonInitStructure.ADC_Mode				= ADC_Mode_Independent; /*在独立模式下 每个ADC接口独立工作*/
	ADC_CommonInitStructure.ADC_Prescaler			= ADC_Prescaler_Div4;
	ADC_CommonInitStructure.ADC_DMAAccessMode		= ADC_DMAAccessMode_1;  // ADC_DMAAccessMode_Disabled;
	ADC_CommonInitStructure.ADC_TwoSamplingDelay	= ADC_TwoSamplingDelay_5Cycles;
	ADC_CommonInit( &ADC_CommonInitStructure );

	ADC_InitStructure.ADC_Resolution			= ADC_Resolution_12b;
	ADC_InitStructure.ADC_ScanConvMode			= ENABLE;                   // if used  multi channels set enable
	ADC_InitStructure.ADC_ContinuousConvMode	= ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConvEdge	= ADC_ExternalTrigConvEdge_None;
	ADC_InitStructure.ADC_ExternalTrigConv		= ADC_ExternalTrigConv_T1_CC1;
	ADC_InitStructure.ADC_DataAlign				= ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfConversion		= 3;                        // number of   channel
	ADC_Init( ADC1, &ADC_InitStructure );

//  4. DMA  Config
	/* DMA2 Stream0 channel0 configuration */
	DMA_InitStructure.DMA_Channel				= DMA_Channel_0;
	DMA_InitStructure.DMA_PeripheralBaseAddr	= (uint32_t)ADC1_DR_Address;
	DMA_InitStructure.DMA_Memory0BaseAddr		= (uint32_t)ADC_ConValue;
	DMA_InitStructure.DMA_DIR					= DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_BufferSize			= 3;
	DMA_InitStructure.DMA_PeripheralInc			= DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc				= DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize	= DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize		= DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode					= DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority				= DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode				= DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold			= DMA_FIFOThreshold_HalfFull;
	DMA_InitStructure.DMA_MemoryBurst			= DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst		= DMA_PeripheralBurst_Single;
	DMA_Init( DMA2_Stream0, &DMA_InitStructure );

	/* DMA2_Stream0 enable */
	DMA_Cmd( DMA2_Stream0, ENABLE );

	ADC_RegularChannelConfig( ADC1, ADC_Channel_15, 1, ADC_SampleTime_56Cycles );   // 通道1  电池电量
	ADC_RegularChannelConfig( ADC1, ADC_Channel_1, 2, ADC_SampleTime_56Cycles );    //  通道2   灰线
	ADC_RegularChannelConfig( ADC1, ADC_Channel_13, 3, ADC_SampleTime_56Cycles );   // 通道3   绿线

	/* Enable DMA request after last transfer (Single-ADC mode) */
	ADC_DMARequestAfterLastTransferCmd( ADC1, ENABLE );

	/* Enable ADC1 DMA */
	ADC_DMACmd( ADC1, ENABLE );

	/* Enable ADC3 */
	ADC_Cmd( ADC1, ENABLE );

	ADC_SoftwareStartConv( ADC1 );
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
	ad_init();
}

/************************************** The End Of File **************************************/

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
#include "scr.h"



AUX_IO				PIN_IN[10] = {
	{ GPIOE, GPIO_Pin_8,  78,  19 },    /*紧急按钮*/
	{ GPIOE, GPIO_Pin_9,  64,  26 },    /*ACC*/
	{ GPIOE, GPIO_Pin_7,  71,  19 },    /*输入*/
	{ GPIOC, GPIO_Pin_0,  95,  26 },    /*4.远光*/
	{ GPIOC, GPIO_Pin_1,  88,  26 },    /*5.车门*/
	{ GPIOA, GPIO_Pin_1,  116, 19 },    /*6.喇叭 定义为AD输入*/
	{ GPIOC, GPIO_Pin_3,  109, 19 },    /*7.左转 定义为AD输入*/
	{ GPIOC, GPIO_Pin_2,  102, 19 },    /*8.右转*/
	{ GPIOE, GPIO_Pin_11, 95,  19 },    /*9.刹车*/
	{ GPIOE, GPIO_Pin_10, 88,  19 },    /*10.雨刷*/
};

AUX_IO				PIN_OUT[] = {
	{ GPIOB, GPIO_Pin_1, 0, 0 },        /*继电器*/
	{ GPIOB, GPIO_Pin_6, 0, 0 },        /*蜂鸣器*/
};

const unsigned char res_cross[] = {
	/* 84218421*/
	0x90,                               /*[*  *    ]*/
	0x60,                               /*[ **     ]*/
	0x60,                               /*[ **     ]*/
	0x90,                               /*[*  *    ]*/
};

DECL_BMP( 4, 4, res_cross );

/*AD检测*/

#define ADC1_DR_Address ( (uint32_t)0X4001204C )

uint16_t ADC_ConValue[3];   //   3  个通道ID    0 : 电池 1: 灰线   2:  绿线

/*首次定位的时刻*/
static uint32_t fixed_sec = 0;


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

	/* ADC1 regular channel15 configuration *************************************/
	ADC_RegularChannelConfig( ADC1, ADC_Channel_15, 1, ADC_SampleTime_56Cycles );   // 通道1  电池电量
	/* ADC1 regular channel1 configuration *************************************/
	ADC_RegularChannelConfig( ADC1, ADC_Channel_1, 2, ADC_SampleTime_56Cycles );    //  通道2   灰线
	/* ADC1 regular channel13 configuration *************************************/
	ADC_RegularChannelConfig( ADC1, ADC_Channel_13, 3, ADC_SampleTime_56Cycles );   // 通道3   绿线

	/* Enable DMA request after last transfer (Single-ADC mode) */
	ADC_DMARequestAfterLastTransferCmd( ADC1, ENABLE );

	/* Enable ADC1 DMA */
	ADC_DMACmd( ADC1, ENABLE );

	/* Enable ADC3 */
	ADC_Cmd( ADC1, ENABLE );

	ADC_SoftwareStartConv( ADC1 );
}

static uint8_t year,month,day,hour,minute,sec;


__IO uint16_t	IC2Value	= 0;
__IO uint16_t	DutyCycle	= 0;
__IO uint32_t	Frequency	= 0;


uint32_t mems_alarm_tick=0;



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

/*TIM5_CH1*/
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

/*显示状态信息*/
static void showinfo(void)
{
	char buf[32];
	if( fixed_sec )
	{
		sprintf( buf, "%02d:%02d", fixed_sec / 60, fixed_sec % 60 );
		lcd_asc0608( 0, 8, buf, LCD_MODE_SET );
	}
	lcd_update( 0, 31 );
}



/*
   只是显示信息，并没有子菜单
   显示3页的信息 经纬度 定位 时间
 */

static void show( void *parent )
{
	GPIO_InitTypeDef	GPIO_InitStructure;
	uint8_t				i;
	char				buf[32];

	scr_1_idle.parent = (PSCR)parent;

	pulse_init( );
	//ad_init( );

	for( i = 0; i < sizeof( PIN_OUT ) / sizeof( AUX_IO ); i++ )
	{
		GPIO_InitStructure.GPIO_Pin		= PIN_OUT[i].pin;
		GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
		GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
		GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;
		GPIO_Init( PIN_OUT[i].port, &GPIO_InitStructure );
	}

	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;

	for( i = 0; i < sizeof( PIN_IN ) / sizeof( AUX_IO ); i++ )
	{
		GPIO_InitStructure.GPIO_Pin = PIN_IN[i].pin;
		GPIO_Init( PIN_IN[i].port, &GPIO_InitStructure );
	}
/*PA0 速度信号*/


	showinfo();

}

/*按键处理*/
static void keypress( unsigned int key )
{
	uint8_t buf[32];
	uint8_t i,h,m;

	if(key==0) return;
	switch( key )
	{
		case KEY_MENU_PRESS:
			break;
		case KEY_OK_PRESS:      /*返回上级菜单*/
			break;
		case KEY_UP_PRESS:		/*拍照*/
			for(i=0;i<3;i++) step(100,1000);
			break;
		case KEY_DOWN_PRESS:    /*打印测试*/
			GPIO_ResetBits(GPIOB,GPIO_Pin_6);
			/*
			printer( "车牌号码:\r\n车牌分类:\r\n车辆VIN:\r\n驾驶员姓名:\r\n驾驶证代码:\r\n" );
			
			sprintf( buf, "打印时间:20%02d/%02d/%02d %02d:%02d:%02d\r\n" ,year,month,day,hour,minute,sec );
			printer( buf );
			printer( "停车前15分钟车速:\r\n" );
			h=hour;
			m=minute;
			for(i=0;i<15;i++)
			{	
				if(m==0)
				{
					m=60;
					if(h==0) h=24;
					h--;
				}	
				m--;
				sprintf(buf,"%02d:%02d 000km/h\r\n",h,m);
				printer(buf);
			}
			printer( "最近一次疲劳驾驶记录:\r\n无疲劳驾驶记录\r\n" );
			*/
			printer( "最近一次超速驾驶记录:\r\n无超速驾驶记录\r\n" );	
			step(25,1000);
			
			break;
	}
}

/*系统时间50ms*/
static void timetick( unsigned int systick )
{
	static uint8_t	offset = 0;
	uint32_t		sec;
	uint8_t			buf[32];
	uint8_t			i,j;



	offset++;
	if( offset >= 20 )
	{
		offset	= 0;
		sec		= systick / 100;
		sprintf( buf, "%02d:%02d  %04d  %02d%%", sec / 60, sec % 60, Frequency, DutyCycle );
		lcd_asc0608( 0, 0, buf, LCD_MODE_SET );
		if( sec & 0x01 ) /*输出控制*/
		{
			GPIO_SetBits( GPIOB, GPIO_Pin_1 );
		}else
		{
			GPIO_ResetBits( GPIOB, GPIO_Pin_1 );
		}
	}

/*IO输入控制*/

	lcd_update( 0, 31 );
}

/*处理自检状态的消息*/
static void msg( void *pmsg )
{
	LCD_MSG		* plcd_msg = (LCD_MSG* )pmsg;
	char		buf[64];
	uint32_t	i;

	if( plcd_msg->id == LCD_MSG_ID_GPS )
	{
		if( ( fixed_sec == 0 ) && ( plcd_msg->info.gps_rmc.gps_av == 'A' ) )
		{
			fixed_sec = rt_tick_get( ) * 10 / 1000;
			sprintf( buf, "%02d:%02d", fixed_sec / 60, fixed_sec % 60 );
			lcd_asc0608( 0, 8, buf, LCD_MODE_SET );
		}
		year=plcd_msg->info.gps_rmc.year;
		month=plcd_msg->info.gps_rmc.month;
		day= plcd_msg->info.gps_rmc.day;
		hour= plcd_msg->info.gps_rmc.hour;
		minute= plcd_msg->info.gps_rmc.minitue;
		sec= plcd_msg->info.gps_rmc.sec;
			
		
		sprintf( buf, "%c%02d-%02d %02d:%02d:%02d",
		         plcd_msg->info.gps_rmc.gps_av,month,day,hour,minute,sec );
		lcd_asc0608( 122 - 6 * 15, 8, buf, LCD_MODE_SET );
	}
	if( plcd_msg->id == LCD_MSG_ID_GSM )
	{
		if( plcd_msg->info.payload[0] == 1 ) /*通话*/
		{
			rt_kprintf( "\r\nIncoming Call" );
			pscr = &scr_2_call;
			pscr->show( &scr_1_idle );
		}
	}
	if(plcd_msg->id == LCD_MSG_ID_GPRS)
	{
		i=rt_tick_get()/100;
		sprintf( buf, "%02d:%02d  GPRS",i/60,i%60);
		lcd_asc0608( 0,16, buf, LCD_MODE_SET );

	}

	showinfo();
	
}

SCR scr_1_idle =
{
	&show,
	&keypress,
	&timetick,
	&msg,
};

/************************************** The End Of File **************************************/

/*
 * File      : application.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      the first version
 */


/**
 * @addtogroup STM32
 */
/*@{*/

#include <stdio.h>

#include "stm32f4xx.h"
#include <board.h>
#include <rtthread.h>

#include "rtc.h"
#include "jt808_gps.h"
#include "jt808_util.h"



/*
   通过gps语句触发的1秒定时
   未定位时也有疲劳驾驶
 */
static void adjust_mytime_now( void )
{
	uint8_t year, month, day, hour, minute, sec;

		sec		= SEC( mytime_now );
		minute	= MINUTE( mytime_now );
		hour	= HOUR( mytime_now );
		day		= DAY( mytime_now );
		month	= MONTH( mytime_now );
		year	= YEAR( mytime_now );
		sec++;

		if( sec == 60 )
		{
			sec = 0;
			minute++;
		}
		if( minute == 60 )
		{
			minute = 0;
			hour++;
		}
		if( hour == 24 )
		{
			hour = 0;
			day++;
		}
		if( ( month == 4 ) || ( month == 6 ) || ( month == 9 ) || ( month == 11 ) )
		{
			if( day == 31 )
			{
				day = 1;
				month++;
			}
		}else if( month == 2 )
		{
			if( year % 4 == 0 ) /*闰年29天*/
			{
				if( day == 30 )
				{
					day = 1;
					month++;
				}
			}else
			{
				if( day == 29 )
				{
					day = 1;
					month++;
				}
			}
		}else if( day == 32 )
		{
			day = 1;
			month++;
		}
		if( month == 13 )
		{
			month = 1;
			year++;
		}
		mytime_now = MYDATETIME( year, month, day, hour, minute, sec );
}


/**/
void synctime( void )
{
	RTC_TimeTypeDef ts;
	RTC_DateTypeDef ds;
	unsigned int	year, month, day;
	unsigned int	hour, minute, sec;
	uint8_t			buf[4];
	uint32_t		utc_bkp, utc;

	RTC_GetTime( RTC_Format_BIN, &ts );
	RTC_GetDate( RTC_Format_BIN, &ds );
	year	= ds.RTC_Year + 2000;
	month	= ds.RTC_Month;
	day		= ds.RTC_Date;
	hour	= ts.RTC_Hours;
	minute	= ts.RTC_Minutes;
	sec		= ts.RTC_Seconds;

	if( 0 >= (int)( month -= 2 ) )    /**//* 1..12 -> 11,12,1..10 */
	{
		month		+= 12;              /**//* Puts Feb last since it has leap day */
		year	-= 1;
	}

	utc= ( ( ( (unsigned long)( year / 4 - year / 100 + year / 400 + 367 * month / 12 + day ) +
	             year * 365 - 719499
	             ) * 24 + hour      /**//* now have hours */
	           ) * 60 + minute         /**//* now have minutes */
	         ) * 60 + sec;          /**//* finally seconds */

	utc_bkp=*(__IO uint32_t*)( BKPSRAM_BASE + 4 );
	rt_kprintf ( "\nutc=%d utc_bkp=%d",utc,utc_bkp);
	if( utc >= utc_bkp )
	{
		if( utc - utc_bkp < 60 * 60 * 6 )		/*6小时以内，认为准确*/
		{
			utc_now		= utc;
			mytime_now	= MYDATETIME( year-2000, month, day, hour, minute, sec );
		}
	}
}



static __IO uint32_t uwLsiFreq = 0;

__IO uint32_t uwTimingDelay = 0;
__IO uint32_t uwCaptureNumber = 0;
__IO uint32_t uwPeriodValue = 0;


/**
  * @brief  Configures TIM5 to measure the LSI oscillator frequency.
  * @param  None
  * @retval LSI Frequency
  */
static uint32_t GetLSIFrequency(void)
{
  NVIC_InitTypeDef   NVIC_InitStructure;
  TIM_ICInitTypeDef  TIM_ICInitStructure;
  RCC_ClocksTypeDef  RCC_ClockFreq;

  /* Enable the LSI oscillator ************************************************/
  RCC_LSICmd(ENABLE);

  /* Wait till LSI is ready */
  while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
  {
  }

  /* TIM5 configuration *******************************************************/
  /* Enable TIM5 clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

  /* Connect internally the TIM5_CH4 Input Capture to the LSI clock output */
  TIM_RemapConfig(TIM5, TIM5_LSI);

  /* Configure TIM5 presclaer */
  TIM_PrescalerConfig(TIM5, 0, TIM_PSCReloadMode_Immediate);

  /* TIM5 configuration: Input Capture mode ---------------------
     The LSI oscillator is connected to TIM5 CH4
     The Rising edge is used as active edge,
     The TIM5 CCR4 is used to compute the frequency value
  ------------------------------------------------------------ */
  TIM_ICInitStructure.TIM_Channel = TIM_Channel_4;
  TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV8;
  TIM_ICInitStructure.TIM_ICFilter = 0;
  TIM_ICInit(TIM5, &TIM_ICInitStructure);

  /* Enable TIM5 Interrupt channel */
  NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Enable TIM5 counter */
  TIM_Cmd(TIM5, ENABLE);

  /* Reset the flags */
  TIM5->SR = 0;

  /* Enable the CC4 Interrupt Request */
  TIM_ITConfig(TIM5, TIM_IT_CC4, ENABLE);


  /* Wait until the TIM5 get 2 LSI edges (refer to TIM5_IRQHandler() in
    stm32f4xx_it.c file) ******************************************************/
  while(uwCaptureNumber != 2)
  {
  }
  /* Deinitialize the TIM5 peripheral registers to their default reset values */
  TIM_DeInit(TIM5);


  /* Compute the LSI frequency, depending on TIM5 input clock frequency (PCLK1)*/
  /* Get SYSCLK, HCLK and PCLKx frequency */
  RCC_GetClocksFreq(&RCC_ClockFreq);

  /* Get PCLK1 prescaler */
  if ((RCC->CFGR & RCC_CFGR_PPRE1) == 0)
  {
    /* PCLK1 prescaler equal to 1 => TIMCLK = PCLK1 */
    return ((RCC_ClockFreq.PCLK1_Frequency / uwPeriodValue) * 8);
  }
  else
  { /* PCLK1 prescaler different from 1 => TIMCLK = 2 * PCLK1 */
    return (((2 * RCC_ClockFreq.PCLK1_Frequency) / uwPeriodValue) * 8) ;
  }
}


void iwdg_init(void)
{
	/* Get the LSI frequency:  TIM5 is used to measure the LSI frequency */
	uwLsiFreq = GetLSIFrequency();
	rt_kprintf("\nLSI Freq = % d ",uwLsiFreq);

	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
	
	  /* IWDG counter clock: LSI/32 */
	  IWDG_SetPrescaler(IWDG_Prescaler_64);
	
	  /* Set counter reload value to obtain 250ms IWDG TimeOut.
		 IWDG counter clock Frequency = LsiFreq/32
		 Counter Reload Value = 250ms/IWDG counter clock period
							  = 0.25s / (32/LsiFreq)
							  = LsiFreq/(32 * 4)
							  = LsiFreq/128

		如果改为4秒   LsiFreq*4/64=LsiFreq/16;
	   */
	
	  IWDG_SetReload(uwLsiFreq/16);  /*[0..4095]*/
	
	  /* Reload IWDG counter */
	  IWDG_ReloadCounter();
	
	  /* Enable IWDG (the LSI oscillator will be enabled by hardware) */
	  IWDG_Enable();
}




ALIGN( RT_ALIGN_SIZE )
static char thread_app_stack[256];
struct rt_thread thread_app;


/*
   应该在此处初始化必要的设备和事件集
 */
void rt_thread_entry_app( void* parameter )
{
	uint8_t need_rtc_init	= 1;
	uint8_t bkpsram_ok		= 0;

	bkpsram_init( );
	//bkpsram_rd(0,16);
	while( 1 )
	{
		if( need_rtc_init ) /*需要初始化*/
		{
			if( rtc_init( ) == 0 ) /*初始化正确*/
			{
				need_rtc_init = 0;
				synctime( );
			}
		}
		adjust_mytime_now();
		//IWDG_ReloadCounter();
		rt_thread_delay( RT_TICK_PER_SECOND );
	}
}

/***********************************************************
* Function:
* Description:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
int rt_application_init( void )
{
	rt_thread_init( &thread_app,
	                "init",
	                rt_thread_entry_app,
	                RT_NULL,
	                &thread_app_stack[0],
	                sizeof( thread_app_stack ), RT_THREAD_PRIORITY_MAX - 3, 5 );
	rt_thread_startup( &thread_app );
	return 0;
}

/*@}*/

/************************************** The End Of File **************************************/

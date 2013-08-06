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
#include <rtthread.h>
#include <stm32f4xx.h>
#include <finsh.h>
#include "rtc.h"


__IO uint32_t			AsynchPrediv = 0, SynchPrediv = 0;
RTC_TimeTypeDef			RTC_TimeStructure;
RTC_InitTypeDef			RTC_InitStructure;
RTC_DateTypeDef			RTC_DateStructure;


#define RTC_CONFIGED_FLAG 0x32f3

#define RTC_CLOCK_SOURCE_LSE /* LSE used as RTC source clock */


/* #define RTC_CLOCK_SOURCE_LSI */    /* LSI used as RTC source clock. The RTC Clock
                                         may varies due to LSI frequency dispersion. */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
RTC_TimeTypeDef		RTC_TimeStructure;
RTC_InitTypeDef		RTC_InitStructure;
RTC_AlarmTypeDef	RTC_AlarmStructure;

__IO uint32_t		uwAsynchPrediv	= 0;
__IO uint32_t		uwSynchPrediv	= 0;

/* Private function prototypes -----------------------------------------------*/
static ErrorStatus RTC_Config( void )
{
	RTC_DateTypeDef RTC_DateStructure;
	__IO uint32_t	timeout = 0xC0000;

	RCC_APB1PeriphClockCmd( RCC_APB1Periph_PWR, ENABLE );
	PWR_BackupAccessCmd( ENABLE );

	/* Enable the LSE OSC */
	RCC_LSEConfig( RCC_LSE_ON );

	/* Wait till LSE is ready */

	while( RCC_GetFlagStatus( RCC_FLAG_LSERDY ) == RESET )
	{
		if( timeout )
		{
			timeout--;
		}else
		{
			rt_kprintf( "\n%s(%d) error\n", __func__, __LINE__ );
			return ERROR;
		}
	}

	/* Select the RTC Clock Source */
	RCC_RTCCLKConfig( RCC_RTCCLKSource_LSE );
	/* ck_spre(1Hz) = RTCCLK(LSE) /(uwAsynchPrediv + 1)*(uwSynchPrediv + 1)*/
	uwSynchPrediv	= 0xFF;
	uwAsynchPrediv	= 0x7F;

	/* Enable the RTC Clock */
	RCC_RTCCLKCmd( ENABLE );

	/* Wait for RTC APB registers synchronisation */
	if( RTC_WaitForSynchro( ) == SUCCESS )
	{
		RTC_InitStructure.RTC_AsynchPrediv	= uwAsynchPrediv;
		RTC_InitStructure.RTC_SynchPrediv	= uwSynchPrediv;
		RTC_InitStructure.RTC_HourFormat	= RTC_HourFormat_24;
		RTC_Init( &RTC_InitStructure );

		/* Set the date: Friday January 11th 2013 */
		RTC_DateStructure.RTC_Year		= 0x13;
		RTC_DateStructure.RTC_Month		= RTC_Month_January;
		RTC_DateStructure.RTC_Date		= 0x11;
		RTC_DateStructure.RTC_WeekDay	= RTC_Weekday_Saturday;
		RTC_SetDate( RTC_Format_BCD, &RTC_DateStructure );

		/* Set the time to 05h 20mn 00s AM */
		RTC_TimeStructure.RTC_H12		= RTC_H12_AM;
		RTC_TimeStructure.RTC_Hours		= 0x05;
		RTC_TimeStructure.RTC_Minutes	= 0x20;
		RTC_TimeStructure.RTC_Seconds	= 0x00;

		RTC_SetTime( RTC_Format_BCD, &RTC_TimeStructure );

		/* Indicator for the RTC configuration */
		RTC_WriteBackupRegister( RTC_BKP_DR0, RTC_CONFIGED_FLAG );
		return SUCCESS;
	}else
	{
		rt_kprintf( "\n%s(%d) error", __func__, __LINE__ );
		return ERROR;
	}
}




/**
 * @brief  Display the current time.
 * @param  None
 * @retval None
 */
void datetime( void )
{
	/* Get the current Time */
	RTC_GetTime( RTC_Format_BIN, &RTC_TimeStructure );
	RTC_GetDate(RTC_Format_BIN,&RTC_DateStructure);
	/* Display time Format : hh:mm:ss */
	rt_kprintf( "\nRTC=%02d-%02d-%02d %02d:%02d:%02d",\
		RTC_DateStructure.RTC_Year,\
		RTC_DateStructure.RTC_Month,\
		RTC_DateStructure.RTC_Date,\
		RTC_TimeStructure.RTC_Hours,\
		RTC_TimeStructure.RTC_Minutes,\
		RTC_TimeStructure.RTC_Seconds );
}

FINSH_FUNCTION_EXPORT(datetime,show time);

/*设置时间*/
void time_set(uint8_t hour,uint8_t min,uint8_t sec)
{
	RTC_TimeStructure.RTC_H12	  = RTC_H12_AM;
	RTC_TimeStructure.RTC_Hours   = hour;
	RTC_TimeStructure.RTC_Minutes = min;
	RTC_TimeStructure.RTC_Seconds = sec; 
	
	RTC_SetTime(RTC_Format_BIN, &RTC_TimeStructure);  
}
FINSH_FUNCTION_EXPORT(time_set,set time);


/*设置日期*/
void date_set(uint8_t year,uint8_t month,uint8_t day)
{
	RTC_DateStructure.RTC_Year   = year;
	RTC_DateStructure.RTC_Month = month;
	RTC_DateStructure.RTC_Date = day; 
	
	RTC_SetDate(RTC_Format_BIN, &RTC_DateStructure);  
}
FINSH_FUNCTION_EXPORT(date_set,set date);



/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
rt_err_t rtc_init(void )
{
	if( RTC_ReadBackupRegister( RTC_BKP_DR0 ) != RTC_CONFIGED_FLAG )
	{
		rt_kprintf( "\n>RTC_CONFIGED_FLAG ERR" );
		/* RTC configuration	*/
		if( RTC_Config( ) == ERROR )
		{
			rt_kprintf( "\n%s(%d) RTC error", __func__, __LINE__ );
			goto lbl_rtc_err;
		}
		/* Display the RTC Time and Alarm */
		datetime( );
		rt_kprintf("\nRTC OK\n");
		goto lbl_rtc_ok;
	}else
	{
		rt_kprintf( "\nwait ForSynchro\n" );

		/* Enable the PWR clock */
		RCC_APB1PeriphClockCmd( RCC_APB1Periph_PWR, ENABLE );

		/* Allow access to RTC */
		PWR_BackupAccessCmd( ENABLE );

		/* Wait for RTC APB registers synchronisation */
		if( RTC_WaitForSynchro( ) == SUCCESS )
		{
			datetime( );
			rt_kprintf("\nRTC OK");
			goto lbl_rtc_ok;
		}else
		{
			rt_kprintf( "\n%s(%d) error", __func__, __LINE__ );
			goto lbl_rtc_err;
		}
	}
lbl_rtc_err:
	return 1;
lbl_rtc_ok:
	return 0;
}

/*
返回系统的时间戳
为了避免频繁计算
调用间隔小于10s的直接给累加上去


*/


unsigned long timestamp(void)
{
	RTC_TimeTypeDef ts;
	RTC_DateTypeDef ds;
	unsigned int year,mon,day;
	unsigned int hour,min, sec;

	static rt_time_t	last_get_tick=0;
	static unsigned long last_ts=0;

	__IO int32_t delta;
	delta=rt_tick_get()-last_get_tick;

	if(delta<RT_TICK_PER_SECOND*10)
	{
		return last_ts+delta/RT_TICK_PER_SECOND;

	}
	
	RTC_GetTime( RTC_Format_BIN, &ts );
	RTC_GetDate(RTC_Format_BIN,&ds);
	year=ds.RTC_Year+2000;
	mon=ds.RTC_Month;
	day=ds.RTC_Date;
	hour=ts.RTC_Hours;
	min=ts.RTC_Minutes;
	sec=ts.RTC_Seconds;
	if( 0 >= (int)( mon-= 2 ) )    /**//* 1..12 -> 11,12,1..10 */
	{
		mon		+= 12;              /**//* Puts Feb last since it has leap day */
		year	-= 1;
	}

	last_get_tick=rt_tick_get();
	last_ts= ( ( ( (unsigned long)( year / 4 - year / 100 + year / 400 + 367 * mon / 12 + day ) +
	               year * 365 - 719499
	               ) * 24 + hour    /**//* now have hours */
	           ) * 60 + min         /**//* now have minutes */
	         ) * 60 + sec;   

	return last_ts;
}
FINSH_FUNCTION_EXPORT(timestamp,get timestamp);


/************************************** The End Of File **************************************/

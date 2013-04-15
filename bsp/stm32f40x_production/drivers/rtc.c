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
#include "rtc.h"

__IO uint32_t			AsynchPrediv = 0, SynchPrediv = 0;
RTC_TimeTypeDef			RTC_TimeStructure;
RTC_InitTypeDef			RTC_InitStructure;
RTC_DateTypeDef			RTC_DateStructure;

static struct rt_device dev_rtc;


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t rt_rtc_control( rt_device_t dev, rt_uint8_t cmd, void *args )
{
	//rt_time_t *time;
	RT_ASSERT( dev != RT_NULL );

	switch( cmd )
	{
		case RT_DEVICE_CTRL_RTC_GET_TIME:
			// time = (rt_time_t *)args;
			/* read device */
			RTC_GetTime( RTC_Format_BIN, &RTC_TimeStructure );
			break;

		case RT_DEVICE_CTRL_RTC_SET_TIME:
		{
//        time = (rt_time_t *)args;

			/* Enable PWR and BKP clocks */
			//RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
			RCC_APB1PeriphClockCmd( RCC_APB1Periph_PWR, ENABLE );
			/* Allow access to BKP Domain */
			//PWR_BackupAccessCmd(ENABLE);
			PWR_BackupAccessCmd( ENABLE );
			/* Wait until last write operation on RTC registers has finished */
			//RTC_WaitForLastTask();
			RTC_SetTime( RTC_Format_BIN, &RTC_TimeStructure );
			if( RTC_SetTime( RTC_Format_BIN, &RTC_TimeStructure ) == ERROR )
			{
				rt_kprintf( "\n\r>> !! RTC Set Time failed. !! <<\n\r" );
			}else
			{
				rt_kprintf( "\n\r>> !! RTC Set Time success.~^_^~ !! <<\n\r" );
				RTC_TimeShow( );
			}
			RTC_WaitForSynchro( );

			/* Change the current time */
			//RTC_SetCounter(*time);

			/* Wait until last write operation on RTC registers has finished */
			//RTC_WaitForLastTask();

			RTC_WriteBackupRegister( RTC_BKP_DR0, 0x32F2 );
		}
		break;
		case RT_DEVICE_CTRL_RTC_SET_DATE:
		{
			RCC_APB1PeriphClockCmd( RCC_APB1Periph_PWR, ENABLE );
			PWR_BackupAccessCmd( ENABLE );
			RTC_SetDate( RTC_Format_BIN, &RTC_DateStructure );
			if( RTC_SetTime( RTC_Format_BIN, &RTC_TimeStructure ) == ERROR )
			{
				rt_kprintf( "\n\r>> !! RTC Set Date failed. !! <<\n\r" );
			}else
			{
				rt_kprintf( "\n\r>> !! RTC Set Date success.~^_^~ !! <<\n\r" );
				RTC_TimeShow( );
			}
			RTC_WaitForSynchro( );
		}
		break;
	}

	return RT_EOK;
}

/*******************************************************************************
* Function Name  : RTC_Configuration
* Description    : Configures the RTC.
* Input          : None
* Output         : None
* Return         : 0 reday,-1 error.
*******************************************************************************/
int RTC_Config( void )
{
	u32 count = 0x200000;
	/* Enable the PWR clock */
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_PWR, ENABLE );

	/* Allow access to RTC */
	PWR_BackupAccessCmd( ENABLE );

	RCC_LSEConfig( RCC_LSE_ON );

	/* Wait till LSE is ready */
	while( RCC_GetFlagStatus( RCC_FLAG_LSERDY ) == RESET && ( --count ) )
	{
		;
	}
	if( count == 0 )
	{
		return -1;
	}

	/* Select the RTC Clock Source */
	RCC_RTCCLKConfig( RCC_RTCCLKSource_LSE );

	SynchPrediv		= 0xFF;
	AsynchPrediv	= 0x7F;

	/* Enable the RTC Clock */
	RCC_RTCCLKCmd( ENABLE );

	/* Wait for RTC APB registers synchronisation */
	RTC_WaitForSynchro( );
	return 0;
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
void RTC_TimeShow( void )
{
	/* Get the current Time */
	//RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);
	RTC_GetTime( RTC_Format_BIN, &RTC_TimeStructure );
	rt_kprintf( "\n\r  The current time is :  %0.2d:%0.2d:%0.2d\n\r", RTC_TimeStructure.RTC_Hours, RTC_TimeStructure.RTC_Minutes, RTC_TimeStructure.RTC_Seconds );
	RTC_GetDate( RTC_Format_BIN, &RTC_DateStructure );
	rt_kprintf( "\n\r  The current Date is :  20%0.2d-%0.2d-%0.2d\n\r", RTC_DateStructure.RTC_Year, RTC_DateStructure.RTC_Month, RTC_DateStructure.RTC_Date );
	//rt_kprintf("xiao,xiao");
}

/**********************************************************可调用标准函数接口*****************************************/

void set_time( rt_uint32_t hour, rt_uint32_t minute, rt_uint32_t second )
{
	rt_device_t device;

	RTC_TimeStructure.RTC_Hours		= hour;
	RTC_TimeStructure.RTC_Minutes	= minute;
	RTC_TimeStructure.RTC_Seconds	= second;

	device = rt_device_find( "rtc" );
	if( device != RT_NULL )
	{
		rt_rtc_control( device, RT_DEVICE_CTRL_RTC_SET_TIME, &RTC_TimeStructure );
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
void set_date( rt_uint32_t year, rt_uint32_t month, rt_uint32_t date )
{
	rt_device_t device;

	RTC_DateStructure.RTC_Year	= year;
	RTC_DateStructure.RTC_Month = month;
	RTC_DateStructure.RTC_Date	= date;

	device = rt_device_find( "rtc" );
	if( device != RT_NULL )
	{
		rt_rtc_control( device, RT_DEVICE_CTRL_RTC_SET_DATE, &RTC_TimeStructure );
	}
}




static rt_err_t rtc_init( rt_device_t dev )

{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
	PWR_BackupAccessCmd(ENABLE);

	if( RTC_ReadBackupRegister( RTC_BKP_DR0 ) != 0x32F2 )
	{
		rt_kprintf( "rtc is not configured\n" );
		RTC_Config( );
		if( RTC_Config( ) != 0 )
		{
			rt_kprintf( "rtc configure fail...\r\n" );
			return;
		}else
		{
			/* Configure the RTC data register and RTC prescaler */
			RTC_InitStructure.RTC_AsynchPrediv	= AsynchPrediv;
			RTC_InitStructure.RTC_SynchPrediv	= SynchPrediv;
			RTC_InitStructure.RTC_HourFormat	= RTC_HourFormat_24;

			/* Check on RTC init */
			if( RTC_Init( &RTC_InitStructure ) == ERROR )
			{
				rt_kprintf( "\n\r  /!\\***** RTC Prescaler Config failed ********/!\\ \n\r" );
			}
		}
		rt_kprintf( "NOW  set  Arbitrary initial time 11:11:11\n" );
		RTC_TimeStructure.RTC_Hours		= 11;
		RTC_TimeStructure.RTC_Minutes	= 11;
		RTC_TimeStructure.RTC_Seconds	= 11;
	}else
	{
		/* Wait for RTC registers synchronization */
		if(RTC_WaitForSynchro( )==ERROR)
		{

		}
	}

	/* register rtc device */
	rtc.init	= RT_NULL;
	rtc.open	= rt_rtc_open;
	rtc.close	= RT_NULL;
	rtc.read	= rt_rtc_read;
	rtc.write	= RT_NULL;
	rtc.control = rt_rtc_control;

	/* no private */
	//rtc.user_data = RT_NULL;

	rt_device_register( &rtc, "rtc", RT_DEVICE_FLAG_RDWR );
#ifdef RT_USING_FINSH
	//RTC_TimeShow();
#endif

	return;
}


/***********************************************************
* Function:
* Description:    在此是否应该给打印上电，只是在要打印时上电
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t rtc_open( rt_device_t dev, rt_uint16_t oflag )
{
	return RT_EOK;
}

/***********************************************************
* Function:       // 函数名称
* Description:    // 函数功能、性能等的描述
* Input:          // 1.输入参数1，说明，包括每个参数的作用、取值说明及参数间关系
* Input:          // 2.输入参数2，说明，包括每个参数的作用、取值说明及参数间关系
* Output:         // 1.输出参数1，说明
* Return:         // 函数返回值的说明
* Others:         // 其它说明
***********************************************************/
static rt_size_t rtc_read( rt_device_t dev, rt_off_t pos, void* buff, rt_size_t count )
{
	return RT_EOK;
}

/***********************************************************
* Function:       // 函数名称
* Description:    // 函数功能、性能等的描述
* Input:          // 1.输入参数1，说明，包括每个参数的作用、取值说明及参数间关系
* Input:          // 2.输入参数2，说明，包括每个参数的作用、取值说明及参数间关系
* Output:         // 1.输出参数1，说明
* Return:         // 函数返回值的说明
* Others:         // 其它说明
***********************************************************/
static rt_size_t rtc_write( rt_device_t dev, rt_off_t pos, const void* buff, rt_size_t count )
{
	rt_size_t ret = RT_EOK;
	ret = rt_ringbuffer_put( &rb_printer_data, (unsigned char*)buff, count );
	return ret;
}

/***********************************************************
* Function:       // 函数名称
* Description:    // 函数功能、性能等的描述
* Input:          // 1.输入参数1，说明，包括每个参数的作用、取值说明及参数间关系
* Input:          // 2.输入参数2，说明，包括每个参数的作用、取值说明及参数间关系
* Output:         // 1.输出参数1，说明
* Return:         // 函数返回值的说明
* Others:         // 其它说明
***********************************************************/
static rt_err_t rtc_control( rt_device_t dev, rt_uint8_t cmd, void *arg )
{
	uint32_t	code = *(uint32_t*)arg;
	int			i;
	switch( cmd )
	{
		case PRINTER_CMD_CHRLINE_N:
			for( i = 0; i < code; i++ )
			{
				drivers1( );
				drivers2( );
				printer_stop( );
			}
			break;
		case PRINTER_CMD_DOTLINE_N:
			for( i = 0; i < code * 24; i++ )
			{
				drivers1( );
				drivers2( );
				printer_stop( );
			}
			break;
		case PRINTER_CMD_FACTORY:
			break;
		case PRINTER_CMD_GRAYLEVEL:
			printer_param.gray_level = code;
			break;
		case PRINTER_CMD_LINESPACE_DEFAULT:
			printer_param.line_space = code;
			break;
		case PRINTER_CMD_LINESPACE_N:
			if( code > 29 )
			{
				code = code % 29;
			} else
			{
				code = 4;
			}
			break;
		case PRINTER_CMD_MARGIN_LEFT:
			printer_param.margin_left = code;
			break;
		case PRINTER_CMD_MARGIN_RIGHT:
			printer_param.margin_right = code;
			break;
	}
	return RT_EOK;
}

/***********************************************************
* Function:
* Description:在此给打印机断电
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t rtc_close( rt_device_t dev )
{
	return RT_EOK;
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
void rtc_driver_init( void )
{
	dev_rtc.type		= RT_Device_Class_RTC;
	dev_rtc.init		= rtc_init;
	dev_rtc.open		= rtc_open;
	dev_rtc.close		= rtc_close;
	dev_rtc.read		= rtc_read;
	dev_rtc.write		= rtc_write;
	dev_rtc.control		= rtc_control;
	dev_rtc.user_data	= RT_NULL;

	rt_device_register( &dev_rtc, "rtc", RT_DEVICE_FLAG_RDWR );
	rt_device_init( &dev_rtc );
}

/************************************** The End Of File **************************************/

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


ALIGN( RT_ALIGN_SIZE )
static char thread_app_stack[256];
struct rt_thread thread_app;

/*
   应该在此处初始化必要的设备和事件集
 */
void rt_thread_entry_app( void* parameter )
{
	uint8_t rtc_need_init=1;
	while( 1 )
	{
		if( rtc_need_init)
		{
			if(rtc_init( )==0)
			{
				rtc_need_init=0;
				timestamp( );
			}	
		}
		rt_thread_delay( RT_TICK_PER_SECOND * 5 );
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
	                sizeof( thread_app_stack ),RT_THREAD_PRIORITY_MAX-3, 5 );
	rt_thread_startup( &thread_app );
	return 0;

}

/*@}*/

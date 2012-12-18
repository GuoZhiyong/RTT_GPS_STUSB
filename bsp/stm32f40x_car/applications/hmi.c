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

#include <stdio.h>

#include "stm32f4xx.h"
#include <board.h>
#include <rtthread.h>
#include <lcd\menu.h>

ALIGN( RT_ALIGN_SIZE )
static char thread_hmi_stack[512];
struct rt_thread thread_hmi;


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void rt_thread_entry_hmi( void* parameter )
{

	/* GPIOD Periph clock enable */
	Init_lcdkey();
	lcd_init();

	pMenuItem = &Menu_1_Idle;
	pMenuItem->show( );
	while( 1 )
	{
		pMenuItem->timetick( 10 );  // 每个子菜单下 显示的更新 操作  时钟源是 任务执行周期
		pMenuItem->keypress( 10 );  //每个子菜单的 按键检测  时钟源100ms timer
		KeyCheckFun( );
		rt_thread_delay( 5 );
		GPIO_ResetBits( GPIOD, GPIO_Pin_9 );
		rt_thread_delay( 5 );
		GPIO_SetBits( GPIOD, GPIO_Pin_9 );
		//rt_kprintf("%d\n",rt_tick_get());
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
void hmi_init( void )
{
	rt_thread_t tid;



	rt_thread_init( &thread_hmi,
	                "hmi_lcd",
	                rt_thread_entry_hmi,
	                RT_NULL,
	                &thread_hmi_stack[0],
	                sizeof( thread_hmi_stack ), 17, 5 );
	rt_thread_startup( &thread_hmi );
}

/************************************** The End Of File **************************************/

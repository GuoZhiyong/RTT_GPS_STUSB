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
#include <string.h>
#include "stm32f4xx.h"
#include <board.h>
#include <rtthread.h>
#include "jt808.h"
#include "menu_include.h"
#include "sed1520.h"

#include "sle4442.h"

#define KEY_NONE 0

typedef struct _KEY
{
	GPIO_TypeDef	*port;
	uint32_t		pin;
	uint32_t		tick;
	uint32_t		status;             /*记录每个按键的状态*/
}KEY;

static KEY		keys[] = {
	{ GPIOD, GPIO_Pin_3, 0, KEY_NONE }, /*menu*/
	{ GPIOC, GPIO_Pin_9, 0, KEY_NONE }, /*up*/
	{ GPIOA, GPIO_Pin_8, 0, KEY_NONE }, /*down*/
	{ GPIOC, GPIO_Pin_8, 0, KEY_NONE }, /*ok*/
};

static uint8_t	beep_high_ticks = 0;
static uint8_t	beep_low_ticks	= 0;
static uint8_t	beep_state		= 0;
static uint32_t beep_ticks;             /*持续时间计数*/
static uint16_t beep_count = 0;


/*
   50ms检查一次按键,只是置位对应的键，程序中判断组合键按下
 */

static uint32_t  keycheck( void )
{
	int			i;
	uint32_t	tmp_key = 0;
	for( i = 0; i < 4; i++ )
	{
		if( GPIO_ReadInputDataBit( keys[i].port, keys[i].pin ) )    /*键抬起*/
		{
			if( ( keys[i].tick > 50 ) && ( keys[i].tick < 500 ) )   /*短按,去抖*/
			{
				keys[i].status = ( 1 << i );
			}else
			{
				keys[i].status = 0;                                 /*清空对应的标志位*/
			}
			keys[i].tick = 0;
		}else /*键按下*/
		{
			keys[i].tick += 50;                                     /*每次增加50ms*/
			if( ( keys[i].tick % 1000 ) == 0 )
			{
				keys[i].status = ( 1 << i ) << 4;
				//keys[i].tick	= 0;
			}
		}
	}
	tmp_key = keys[0].status | keys[1].status | keys[2].status | keys[3].status;
	for( i = 0; i < 4; i++ )
	{
		keys[i].status = 0;
	}
#if 1
	if( tmp_key )
	{
		rt_kprintf( "%04x\r\n", tmp_key );
	}
#endif
	return ( tmp_key );
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
static void key_lcd_port_init( void )
{
	GPIO_InitTypeDef	GPIO_InitStructure;
	int					i;

	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA, ENABLE );
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOB, ENABLE );
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOC, ENABLE );
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOD, ENABLE );
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOE, ENABLE );

	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;

	for( i = 0; i < 4; i++ )
	{
		GPIO_InitStructure.GPIO_Pin = keys[i].pin;
		GPIO_Init( keys[i].port, &GPIO_InitStructure );
	}

	//OUT	(/MR  SHCP	 DS   STCP	 STCP)
	GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;
	GPIO_Init( GPIOE, &GPIO_InitStructure );

	//BUZZER

	if( jt808_param.id_0xF013 == 0x3017 )
	{
		GPIO_InitStructure.GPIO_Pin		= GPIO_Pin_6;
		GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_OUT;
		GPIO_InitStructure.GPIO_OType	= GPIO_OType_PP;
		GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_PuPd	= GPIO_PuPd_NOPULL;
		GPIO_Init( GPIOB, &GPIO_InitStructure );
	}
	//GPIO_SetBits(GPIOB,GPIO_Pin_6);
}

ALIGN( RT_ALIGN_SIZE )
static char thread_hmi_stack[2048];
struct rt_thread thread_hmi;
/*hmi线程*/
static void rt_thread_entry_hmi( void* parameter )
{
	uint32_t key;

	key_lcd_port_init( );
	lcd_init( );

	memset( hmi_15min_speed, 0x0, sizeof( hmi_15min_speed ) );
	hmi_15min_speed_curr = 0;

	pMenuItem = &Menu_1_Idle;
	pMenuItem->show( );
	pMenuItem->tick = rt_tick_get( );
	while( 1 )
	{
		CheckICCard( );
		key = keycheck( );
		if( key )                                               /*有键按下，打开背光*/
		{
			pMenuItem->tick = rt_tick_get( );
			pMenuItem->keypress( key );                         //每个子菜单的 按键检测  时钟源50ms timer
		}
		pMenuItem->timetick( rt_tick_get( ) );                  // 每个子菜单下 显示的更新 操作  时钟源是 任务执行周期

		if( beep_count )                                        /*声音提示*/
		{
			beep_ticks--;
			if( beep_ticks == 0 )
			{
				if( beep_state == 1 )                           /*响的状态*/
				{
					if( jt808_param.id_0xF013 == 0x3017 )
					{
						GPIO_ResetBits( GPIOB, GPIO_Pin_6 );    /*发声*/
					}else
					{
						ctrlbit_buzzer = 0x0;
					}
					beep_ticks	= beep_low_ticks;
					beep_state	= 0;
				}else
				{
					beep_count--;
					if( beep_count )                            /*没响够*/
					{
						if( jt808_param.id_0xF013 == 0x3017 )
						{
							GPIO_SetBits( GPIOB, GPIO_Pin_6 );  /*发声*/
						}else
						{
							ctrlbit_buzzer = 0x80;
						}
						beep_ticks	= beep_high_ticks;
						beep_state	= 1;
					}
				}
			}
		}
		rt_thread_delay( RT_TICK_PER_SECOND / 20 ); /*50ms调用一次*/
	}
}

/*
   调用弹出窗体显示信息，并持续一定时间
 */
void pop_msg( char *msg, uint32_t interval )
{
	Menu_Popup.parent	= pMenuItem;
	pMenuItem			= &Menu_Popup;
	pMenuItem->tick		= interval; /*临时用tick传递一下参数*/
	pMenuItem->show( );
	pMenuItem->msg( (void*)msg );
}

/*
   蜂鸣器发声
   响 high tick
   灭 low
 */
void beep( uint8_t high_50ms_count, uint8_t low_50ms_count, uint16_t count )
{
	beep_high_ticks = high_50ms_count;
	beep_low_ticks	= low_50ms_count;
	beep_state		= 1;                    /*发声*/
	beep_ticks		= beep_high_ticks;
	beep_count		= count;
	if( jt808_param.id_0xF013 == 0x3017 )
	{
		GPIO_SetBits( GPIOB, GPIO_Pin_6 );  /*发声*/
	}else
	{
		ctrlbit_buzzer = 0x80;
	}
}

/**/
void hmi_init( void )
{
	rt_thread_init( &thread_hmi,
	                "hmi",
	                rt_thread_entry_hmi,
	                RT_NULL,
	                &thread_hmi_stack[0],
	                sizeof( thread_hmi_stack ), 17, 5 );
	rt_thread_startup( &thread_hmi );
}

/************************************** The End Of File **************************************/

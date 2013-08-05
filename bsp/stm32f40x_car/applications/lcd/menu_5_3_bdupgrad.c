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
#include "Menu_Include.h"
#include <string.h>
#include "sed1520.h"

#include "gps.h"


/*
   是否在升级状态，禁用按键判断
   0;开始 等待升级
   1:结束，无论成功与否
   2:升级过程中，不响应按键

 */

#define BD_UPGRADE_IDLE 0
#define BD_UPGRADE_END	1
#define BD_UPGRADING	2

static uint8_t		fupgrading	= BD_UPGRADE_IDLE;
static rt_thread_t	tid_upgrade = RT_NULL; /*开始更新*/

/**/
static void show( void )
{
	lcd_fill( 0 );
	lcd_text12( ( 122 - 16 * 6 ) / 2, 8, "U盘更新文件就绪?", 16, LCD_MODE_SET );
	lcd_text12( ( 122 - 14 * 6 ) / 2, 20, "按[确认键]开始", 14, LCD_MODE_SET );
	fupgrading=BD_UPGRADE_IDLE;
	lcd_update_all( );
}

/*
   提供回调函数用以显示信息
 */

static void msg( void *p )
{
	//char buf[32];
	unsigned int	len;
	char			*pinfo;
	lcd_fill( 0 );
	lcd_text12( 0, 3, "北斗", 4, LCD_MODE_SET );
	lcd_text12( 0, 17, "升级", 4, LCD_MODE_SET );
	pinfo	= (char*)p;
	len		= strlen( pinfo );
	lcd_text12( 35, 10, pinfo + 1, len - 1, LCD_MODE_SET );
	if( pinfo[0] == 'E' ) /*出错或结束*/
	{
		fupgrading	= BD_UPGRADE_END;
		tid_upgrade = RT_NULL;
		rt_kprintf( "\nfupgrading=%d", fupgrading );
	}

	lcd_update_all( );
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
static void keypress( unsigned int key )
{
	switch( key )
	{
		case KEY_MENU:
			if( fupgrading < BD_UPGRADING )
			{
				pMenuItem = &Menu_5_other;
				pMenuItem->show( );
			}

			break;
		case KEY_OK:
			if( fupgrading != BD_UPGRADE_IDLE )
			{
				break;
			}
			tid_upgrade = rt_thread_create( "upgrade", thread_gps_upgrade_udisk, (void*)msg, 1024, 5, 5 );
			if( tid_upgrade != RT_NULL )
			{
				fupgrading = BD_UPGRADING;
				msg( "I等待U盘升级" );
				rt_thread_startup( tid_upgrade );
			}else
			{
				fupgrading = BD_UPGRADE_IDLE;
				msg( "E线程创建失败" );
			}
			break;
		case KEY_UP:
			break;
		case KEY_DOWN:
			break;
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
static void timetick( unsigned int tick )
{
}

MENUITEM Menu_5_3_bdupgrade =
{
	"北斗信息或升级",
	14,		   0,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

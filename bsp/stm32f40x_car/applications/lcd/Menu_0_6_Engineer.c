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
#include  <string.h>
#include "Menu_Include.h"
#include "sed1520.h"

/**/
static uint8_t	pos;

static char		*op[] =
{
	"清除所有数据",
	"打印机",
	"修改参数",
};

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void msg( void *p )
{
}

/**/
static void display( void )
{
	uint8_t index = pos & 0xFE;
	lcd_fill( 0 );

	lcd_update_all( );
}

/**/
static void show( void )
{
	pMenuItem->tick = rt_tick_get( );

	display( );
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
			pMenuItem = &Menu_0_loggingin;
			pMenuItem->show( );
			break;
		case KEY_OK:

			break;
		case KEY_UP:
			if( pos == 0 )
			{
				pos = 2;
			}
			pos--;
			display( );
			break;
		case KEY_DOWN:
			pos++;
			pos %= 2;
			display( );
			break;
	}
}

/**/

MENUITEM Menu_0_6_Engineer =
{
	"工程模式",
	8,				  0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};
/************************************** The End Of File **************************************/


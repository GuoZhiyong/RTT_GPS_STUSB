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

static int8_t	pos;

static char		deviceid[16];
static char		deveiceid_count;

/**/

static void msg( void *p )
{
}

/**/
static void display( void )
{
	char * num = "0123456789";
	lcd_fill( 0 );
	if( pos < 12 )
	{
		lcd_text12( 0, 4, "车辆ID", 3, LCD_MODE_INVERT );
		lcd_text12( 20, 4, deviceid, strlen( deviceid ), LCD_MODE_SET );
		lcd_text12( 0, 18, num, 10, LCD_MODE_SET );
		lcd_text12( 0 + pos * 6, 18, num + pos, 1, LCD_MODE_INVERT );
	}else
	{
		lcd_text12( 16, 16, "车辆ID设置完成", 15, LCD_MODE_SET );
	}
	lcd_update_all( );
}

/**/
static void show( void )
{
	pos				= 0;
	deveiceid_count = 0;
	memset( deviceid, 0, 16 );
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
	char * num = "0123456789";
	switch( key )
	{
		case KEY_MENU:
			pMenuItem = &Menu_0_loggingin;
			pMenuItem->show( );
			break;
		case KEY_OK:
			if( deveiceid_count > 11 )
			{
				pMenuItem = &Menu_1_Idle;
				pMenuItem->show( );
			}else
			{
				deviceid[deveiceid_count] = *( num + pos );
				deveiceid_count++;
				pos = 0;
				display( );
			}
			break;
		case KEY_UP:
			if( pos == 0 )
			{
				pos = 12;
			}
			pos--;
			display( );
			break;
		case KEY_DOWN:
			pos++;
			pos %= 12;
			display( );
			break;
	}
}

/**/
static void timetick( unsigned int systick )
{
}

MENUITEM Menu_0_5_DeviceID =
{
	"设备号设置",
	10,			 0,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

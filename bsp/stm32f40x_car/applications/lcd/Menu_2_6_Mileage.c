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
#include <stdlib.h>
#include "sed1520.h"
#include "jt808.h"
#include "jt808_gps.h"

/**/
static void msg( void *p )
{
}

/**/
static void show( void )
{
	char buf[32];
	pMenuItem->tick=rt_tick_get();

	lcd_fill( 0 );
	sprintf( buf, "20%02d/%02d/%02d %02d:%02d",
	         YEAR( mytime_now ),
	         MONTH( mytime_now ),
	         DAY( mytime_now ),
	         HOUR( mytime_now ),
	         MINUTE( mytime_now ));
	lcd_text12( 0, 4, (char*)buf, strlen( buf ), LCD_MODE_SET );
	
	sprintf( buf, "总里程:%06d 公里", jt808_param.id_0xF020 / 1000 );
	lcd_text12( 0, 16, (char*)buf, strlen( buf ), LCD_MODE_SET );
	lcd_update_all( );
}

/**/
static void keypress( unsigned int key )
{
	switch( key )
	{
		case KEY_MENU:
			pMenuItem = &Menu_2_InforCheck;
			pMenuItem->show( );
			break;
		case KEY_OK:
			break;
		case KEY_UP:
			break;
		case KEY_DOWN:
			break;
	}
}

MENUITEM Menu_2_6_Mileage =
{
	"里程信息查看",
	12,				  0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};
/************************************** The End Of File **************************************/

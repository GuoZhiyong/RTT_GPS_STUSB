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

static unsigned char page;


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void display( void )
{
	char buf[32];
	lcd_fill( 0 );
	switch( page )
	{
		case 0:
			sprintf( buf, "车 牌 号:%s", jt808_param.id_0x0083 );
			lcd_text12( 0, 4, (char*)buf, strlen( buf ), LCD_MODE_SET );
			sprintf( buf, "车辆类型:%s", jt808_param.id_0xF00A );
			lcd_text12( 0, 18, (char*)buf, strlen( buf ), LCD_MODE_SET );
			break;
		case 1:
			sprintf( buf, "入网ID:%02x%02x%02x%02x%02x%02x", mobile[0],mobile[1],mobile[2],mobile[3],mobile[4],mobile[5]);
			lcd_text12( 0, 4, (char*)buf, strlen( buf ), LCD_MODE_SET );
			sprintf( buf, "[%s]", jt808_param.id_0xF005 );
			lcd_text12( 0, 18, (char*)buf, strlen( buf ), LCD_MODE_SET );
			break;
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
static void msg( void *p )
{
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
static void show( void )
{
	pMenuItem->tick = rt_tick_get( );
	page			= 0;
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
		case KEY_OK:
			pMenuItem = &Menu_2_InforCheck;
			pMenuItem->show( );
			break;
		case KEY_UP:
			if( page )
			{
				page--;
			}
			page %= 2;
			display( );
			break;
		case KEY_DOWN:
			page++;
			page %= 2;
			display( );
			break;
	}
}

MENUITEM Menu_2_4_CarInfor =
{
	"车辆信息查看",
	12,				  0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

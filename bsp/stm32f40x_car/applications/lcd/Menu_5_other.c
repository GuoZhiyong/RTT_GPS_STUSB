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

unsigned char	noselect_5[] = { 0x3C, 0x7E, 0xC3, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C };  //空心
unsigned char	select_5[] = { 0x3C, 0x7E, 0xFF, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C };    //实心

DECL_BMP( 8, 8, select_5 ); DECL_BMP( 8, 8, noselect_5 );

static unsigned char	menu_pos	= 0;
static PMENUITEM		psubmenu[7] =
{
	&Menu_5_1_TelDis,
	&Menu_5_2_TelAtd,
	&Menu_5_3_bdupgrade,
	&Menu_5_5_can,
	&Menu_5_6_Concuss,
	&Menu_5_7_Version,
	&Menu_5_8_Usb,
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
static void menuswitch( void )
{
	unsigned char i = 0;

	lcd_fill( 0 );
	lcd_text12( 0, 3, "其它", 4, LCD_MODE_SET );
	lcd_text12( 0, 17, "信息", 4, LCD_MODE_SET );
	for( i = 0; i < 8; i++ )
	{
		lcd_bitmap( 30 + i * 11, 5, &BMP_noselect_5, LCD_MODE_SET );
	}
	lcd_bitmap( 30 + menu_pos * 11, 5, &BMP_select_5, LCD_MODE_SET );
	lcd_text12( 30, 19, (char*)( psubmenu[menu_pos]->caption ), psubmenu[menu_pos]->len, LCD_MODE_SET );
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

	menu_pos = 0;
	menuswitch( );
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
			pMenuItem = &Menu_1_Idle;
			pMenuItem->show( );
			break;
		case KEY_OK:
			pMenuItem = psubmenu[menu_pos]; //疲劳驾驶
			pMenuItem->show( );
			break;
		case KEY_UP:
			if( menu_pos == 0 )
			{
				menu_pos = 7;
			} 
			menu_pos--;
			menuswitch( );
			break;
		case KEY_DOWN:
			menu_pos++;
			if( menu_pos > 6 )
			{
				menu_pos = 0;
			}
			menuswitch( );
			break;
	}
}

MENUITEM Menu_5_other =
{
	"其他信息",
	8,				  0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

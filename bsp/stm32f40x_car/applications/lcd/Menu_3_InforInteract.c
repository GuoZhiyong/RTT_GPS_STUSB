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

#define  DIS_Dur_width_inter 11

unsigned char	noselect_inter[] = { 0x3C, 0x7E, 0xC3, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C };  //空心
unsigned char	select_inter[] = { 0x3C, 0x7E, 0xFF, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C };    //实心
DECL_BMP( 8, 8, select_inter );
DECL_BMP( 8, 8, noselect_inter );

static unsigned char	menu_pos	= 0;
static PMENUITEM		psubmenu[] =
{
	&Menu_3_1_CenterQuesSend,
	&Menu_3_2_FullorEmpty,
	&Menu_3_3_ElectronicInfor,
	&Menu_3_4_Multimedia,
	//&Menu_3_5_MultimediaTrans,
	&Menu_3_6_Record,
	&Menu_3_7_Affair,
	&Menu_3_8_LogOut,
};


/**/
void menuswitch( void )
{
	unsigned char i = 0;

	lcd_fill( 0 );
	lcd_text12( 0, 3, "信息", 4, LCD_MODE_SET );
	lcd_text12( 0, 17, "交互", 4, LCD_MODE_SET );
	for( i = 0; i < 8; i++ )
	{
		lcd_bitmap( 30 + i * DIS_Dur_width_inter, 5, &BMP_noselect_inter, LCD_MODE_SET );
	}
	lcd_bitmap( 30 + menu_pos * DIS_Dur_width_inter, 5, &BMP_select_inter, LCD_MODE_SET );
	lcd_text12( 30, 19, (char*)( psubmenu[menu_pos]->caption ), psubmenu[menu_pos]->len, LCD_MODE_SET );
	lcd_update_all( );
}

/**/
static void msg( void *p )
{
}

/**/
static void show( void )
{
	pMenuItem->tick = rt_tick_get( );
	menu_pos		= 0;
	menuswitch( );
}

/**/
static void keypress( unsigned int key )
{
	switch( key )
	{
		case KEY_MENU:
			CounterBack = 0;
			pMenuItem	= &Menu_4_InforTirExspd;    //
			pMenuItem->show( );
			break;
		case KEY_OK:
			pMenuItem = psubmenu[menu_pos];         //鉴权注册
			pMenuItem->show( );
			break;
		case KEY_UP:
			if( menu_pos == 0 )
			{
				menu_pos = 6;
			} else
			{
				menu_pos--;
			}
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

MENUITEM Menu_3_InforInteract =
{
	"交互信息",
	8,				  0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

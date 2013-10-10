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
#include "sed1520.h"

#if 1

unsigned char	noselect_log[] = { 0x3C, 0x7E, 0xC3, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C };    //空心
unsigned char	select_log[] = { 0x3C, 0x7E, 0xFF, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C };      //实心

DECL_BMP( 8, 8, select_log );
DECL_BMP( 8, 8, noselect_log );

static char pos=0;

PMENUITEM mnu[4] = {
	&Menu_0_1_license,
	&Menu_0_2_CarType,
	&Menu_0_3_vin,
	&Menu_0_4_Colour,
};

/**/
static void display(void)
{
	char i;
	lcd_fill( 0 );
	lcd_text12( 0, 3, "注册", 4, LCD_MODE_SET );
	lcd_text12( 0, 17, "输入", 4, LCD_MODE_SET );

	for( i = 0; i < 4; i++ )
	{
		lcd_bitmap( 35 + i * 12, 5, &BMP_noselect_log, LCD_MODE_SET );
	}

	lcd_bitmap(  35 + pos * 12, 5, &BMP_select_log, LCD_MODE_SET );

	lcd_text12( 35, 19, mnu[pos]->caption,mnu[pos]->len, LCD_MODE_INVERT );

	lcd_update_all( );
}

/**/
static void msg( void *p )
{
}

/**/
static void show( void )
{
	pos=0;		/*bitter 从0 开始*/
	display();
}

/**/
static void keypress( unsigned int key )
{
	switch( key )
	{
		case KEY_MENU:
			#if NEED_TODO
			if( menu_color_flag )
			{
				menu_type_flag	= 0;
				menu_color_flag = 0;

				pMenuItem = &Menu_1_Idle; //进入信息查看界面
				pMenuItem->show( );
			}
			#endif
			break;
		case KEY_OK:
			pMenuItem = mnu[pos];  //车牌号输入
			pMenuItem->show( );
			break;
		case KEY_UP:
			break;
		case KEY_DOWN:
			break;
	}
}

/* */
static void timetick( unsigned int systick )
{
}

MENUITEM Menu_0_loggingin =
{
	"车辆设置",
	8,		   0,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

#else

unsigned char	noselect_log[] = { 0x3C, 0x7E, 0xC3, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C };    //空心
unsigned char	select_log[] = { 0x3C, 0x7E, 0xFF, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C };      //实心

unsigned char	CarSet_0 = 1;
//unsigned char CarSet_0_Flag=1;

DECL_BMP( 8, 8, select_log );
DECL_BMP( 8, 8, noselect_log );

/**/
void Selec_123( u8 par )
{
	u8 i = 0;
	if( par == 1 )
	{
		lcd_bitmap( 35, 5, &BMP_select_log, LCD_MODE_SET );
		for( i = 0; i < 3; i++ )
		{
			lcd_bitmap( 47 + i * 12, 5, &BMP_noselect_log, LCD_MODE_SET );
		}
	}else if( par == 2 )
	{
		lcd_bitmap( 35, 5, &BMP_noselect_log, LCD_MODE_SET );
		lcd_bitmap( 47, 5, &BMP_select_log, LCD_MODE_SET );
		for( i = 0; i < 2; i++ )
		{
			lcd_bitmap( 59 + i * 12, 5, &BMP_noselect_log, LCD_MODE_SET );
		}
	}else if( par == 3 )
	{
		for( i = 0; i < 2; i++ )
		{
			lcd_bitmap( 35 + i * 12, 5, &BMP_noselect_log, LCD_MODE_SET );
		}
		lcd_bitmap( 59, 5, &BMP_select_log, LCD_MODE_SET );
		lcd_bitmap( 71, 5, &BMP_noselect_log, LCD_MODE_SET );
	}else if( par == 4 )
	{
		for( i = 0; i < 3; i++ )
		{
			lcd_bitmap( 35 + i * 12, 5, &BMP_noselect_log, LCD_MODE_SET );
		}
		lcd_bitmap( 71, 5, &BMP_select_log, LCD_MODE_SET );
	}
}

/**/
void CarSet_0_fun( u8 set_type )
{
	lcd_fill( 0 );
	lcd_text12( 0, 3, "注册", 4, LCD_MODE_SET );
	lcd_text12( 0, 17, "输入", 4, LCD_MODE_SET );
	Selec_123( CarSet_0_counter );
	switch( set_type )
	{
		case 1:
			lcd_text12( 35, 19, "车牌号输入", 10, LCD_MODE_INVERT );
			break;
		case 2:
			lcd_text12( 35, 19, "车辆类型选择", 12, LCD_MODE_INVERT );
			break;
		case 3:
			lcd_text12( 35, 19, "VIN输入", 7, LCD_MODE_INVERT );
			break;
		case 4:
			lcd_text12( 35, 19, "车牌颜色输入", 12, LCD_MODE_INVERT );
			break;
		default:
			break;
	}
	lcd_update_all( );
}

/**/
static void msg( void *p )
{
}

/**/
static void show( void )
{
	CounterBack = 0;
	CarSet_0_fun( CarSet_0_counter );
}

/**/
static void keypress( unsigned int key )
{
	switch( key )
	{
		case KEY_MENU:
			if( menu_color_flag )
			{
				menu_type_flag	= 0;
				menu_color_flag = 0;

				pMenuItem = &Menu_1_Idle; //进入信息查看界面
				pMenuItem->show( );
			}
			break;
		case KEY_OK:

			if( CarSet_0_counter == 1 )
			{
				pMenuItem = &Menu_0_1_license;  //车牌号输入
				pMenuItem->show( );
			}else if( CarSet_0_counter == 2 )
			{
				pMenuItem = &Menu_0_2_CarType;  //type
				pMenuItem->show( );
			}else if( CarSet_0_counter == 3 )
			{
				pMenuItem = &Menu_0_3_vin;      //颜色
				pMenuItem->show( );
			}else if( CarSet_0_counter == 4 )
			{
				pMenuItem = &Menu_0_4_Colour;   //颜色
				pMenuItem->show( );
			}
			break;
		case KEY_UP:
			break;
		case KEY_DOWN:
			break;
	}
}

/* */
static void timetick( unsigned int systick )
{
}

MENUITEM Menu_0_loggingin =
{
	"车辆设置",
	8,		   0,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

#endif

/************************************** The End Of File **************************************/

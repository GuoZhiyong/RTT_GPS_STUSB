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
unsigned char	*car_status_str[4] = { "空车", "半载", "预留", "满载" };

static uint8_t	pos			= 0;
static uint8_t	selected	= 0; /*是否已选择*/

/**/
static void display( )
{
	unsigned char i = 0;

	lcd_fill( 0 );
	if( selected == 0 )
	{
		lcd_text12( 12, 3, "车辆状态选择", 12, LCD_MODE_SET );
		for( i = 0; i < 4; i++ )
		{
			lcd_text12( i * 30, 19, car_status_str[i], 4, LCD_MODE_SET );
		}
		lcd_text12(30 * pos, 19, car_status_str[pos], 4, LCD_MODE_INVERT );
	}else
	{
		lcd_text12( 12, 3, "车辆状态: ", 10, LCD_MODE_SET );
		lcd_text12( 12+60, 3, car_status_str[pos], 4, LCD_MODE_INVERT );
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
	pMenuItem->tick = rt_tick_get( );
	pos				= jt808_param.id_0xF021;
	selected		= 0;
	display( );
}

/**/
static void keypress( unsigned int key )
{
	uint32_t i;
	
	switch( key )
	{
		case KEY_MENU:
			pMenuItem = &Menu_3_InforInteract;
			pMenuItem->show( );
			break;
		case KEY_OK:
			if( selected ) /*已经选择*/
			{
				pMenuItem = &Menu_3_InforInteract;
				pMenuItem->show( );
				break;
			}
			selected = 1;
			jt808_param.id_0xF021=pos;
			i=jt808_status&0xFFFFFCFF;	/*bit 8.9 清零*/
			jt808_status=i|(uint32_t)(pos<<8);
			display( );
			break;
		case KEY_UP:
			if( pos == 0 )
			{
				pos = 4;
			}
			pos--;
			display( );
			break;
		case KEY_DOWN:
			pos++;
			pos %= 4;
			display( );
			break;
	}
}

MENUITEM Menu_3_2_FullorEmpty =
{
	"车辆状态",
	8,				  0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

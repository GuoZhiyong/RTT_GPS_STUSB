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
#include  <string.h>

#if 1

#define  ENTER_CODE1 "001100"
#define  ENTER_CODE2 "999999"


static uint8_t pos		= 0;
static char	* num	= "01234567890";
static char	input[9];
static char	input_pos = 0;

/*显示输入*/
static void display( void )
{
	uint8_t i;
	lcd_fill( 0 );
	lcd_text12( 0, 4, "请输入6位密码:", 14, LCD_MODE_SET );
	lcd_text12( 84, 4, (char*)input, input_pos, LCD_MODE_SET ); //-1+14

	lcd_text12( 0, 18, "0123456789", 10, LCD_MODE_SET );
	lcd_text12( 0 + pos * 6, 18, num + pos, 1, LCD_MODE_INVERT );
	i = strlen( jt808_param.id_0xF012 );

	lcd_text12( 120 - i * 6, 20, jt808_param.id_0xF012, i, LCD_MODE_SET );
	lcd_update_all( );
}

/**/
static void msg( void *p )
{
}

/*密码输入*/
static void show( void )
{
	pMenuItem->tick=rt_tick_get();
	input_pos = 0;
	memset( input, 0, sizeof( input ) );
	display( );
}

/*按键检查*/
static void keypress( unsigned int key )
{
	switch( key )
	{
		case KEY_MENU:
			pMenuItem = &Menu_1_Idle;
			pMenuItem->show( );
			break;
		case KEY_OK:
			if( input_pos < 6 )
			{
				input[input_pos] = num[pos];
				input_pos++;
				pos=0;
				display();
			}else
			{
				if( strncmp( (char*)input, ENTER_CODE1, 6 ) == 0 )	/**/
				{
					pMenuItem = &Menu_0_4_Colour;
					pMenuItem->show( );
				}
				else if( strncmp( (char*)input, ENTER_CODE2, 6 ) == 0 ) /*工程设置*/
				{
					pMenuItem = &Menu_0_6_Engineer;
					pMenuItem->show();
					
				}
				else
				{
					input_pos=0;
					pos=0;
					display();
				}
			}
			break;
		case KEY_UP:
			if( pos == 0 )
			{
				pos = 10;
			}
			pos--;
			display( );
			break;
		case KEY_DOWN:
			pos++;
			pos %= 10;
			display( );
			break;
	}
}

MENUITEM Menu_0_0_password =
{
	"密码设置",
	8,
	0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};

#else

#define  Sim_width1 6
#define  ENTER_CODE "001100"

u8	set_car_codetype		= 0;
u8	Password_correctFlag	= 0; // 密码正确
u8	password_Code[10];
u8	password_SetFlag	= 1, password_Counter = 0;
u8	password_icon[]		= { 0x0C, 0x06, 0xFF, 0x06, 0x0C };

DECL_BMP( 8, 5, password_icon );

/**/
static void password_Set( u8 par )
{
	uint8_t i;
	lcd_fill( 0 );
	lcd_text12( 0, 3, "请输入6位密码:", 14, LCD_MODE_SET );
	if( password_SetFlag > 1 )
	{
		lcd_text12( 84, 3, (char*)password_Code, password_SetFlag - 1, LCD_MODE_SET ); //-1+14
	}
	lcd_bitmap( par * Sim_width1, 14, &BMP_password_icon, LCD_MODE_SET );
	lcd_text12( 0, 19, "0123456789", 10, LCD_MODE_SET );
	i = strlen( jt808_param.id_0xF012 );

	lcd_text12( 120 - i * 6, 20, jt808_param.id_0xF012, i, LCD_MODE_SET );
	lcd_update_all( );
}

/**/
static void msg( void *p )
{
}

/*密码输入*/
static void show( void )
{
	password_SetFlag	= 1;
	password_Counter	= 0;

	memset( password_Code, 0, sizeof( password_Code ) );
	password_Set( password_Counter );
}

/*按键检查*/
static void keypress( unsigned int key )
{
	switch( key )
	{
		case KEY_MENU:
			if( Password_correctFlag == 1 )
			{
				if( set_car_codetype == 1 )
				{
					set_car_codetype	= 0;
					CarSet_0_counter	= 1; //设置第1项
					pMenuItem			= &Menu_0_loggingin;
				}else
				{
					pMenuItem = &Menu_1_Idle;
				}
				pMenuItem->show( );
				memset( password_Code, 0, sizeof( password_Code ) );
				password_SetFlag	= 1;
				password_Counter	= 0;
			}
			break;
		case KEY_OK:
			if( ( password_SetFlag >= 1 ) && ( password_SetFlag <= 6 ) )
			{
				if( password_Counter <= 9 )
				{
					password_Code[password_SetFlag - 1] = password_Counter + '0';
				}
				password_Counter = 0;
				password_SetFlag++;
				password_Set( 0 );
			}else if( password_SetFlag == 7 )
			{
				if( strncmp( (char*)password_Code, ENTER_CODE, 6 ) == 0 )
				{
					password_SetFlag		= 8;
					Password_correctFlag	= 1;
					set_car_codetype		= 1;
					lcd_fill( 0 );
					lcd_text12( 36, 3, "密码正确", 8, LCD_MODE_SET );
					lcd_text12( 0, 19, "按菜单键进入设置信息", 20, LCD_MODE_SET );
					lcd_update_all( );
				}else
				{
					password_SetFlag = 9;
					lcd_fill( 0 );
					lcd_text12( 36, 3, "密码错误", 8, LCD_MODE_SET );
					lcd_text12( 12, 19, "按确认键重新设置", 16, LCD_MODE_SET );
					lcd_update_all( );
				}
			}else if( password_SetFlag == 9 )
			{
				pMenuItem = &Menu_0_0_password;
				pMenuItem->show( );
			}
			break;
		case KEY_UP:
			if( ( password_SetFlag >= 1 ) && ( password_SetFlag <= 6 ) )
			{
				if( password_Counter == 0 )
				{
					password_Counter = 9;
				} else if( password_Counter >= 1 )
				{
					password_Counter--;
				}
				password_Set( password_Counter );
			}

			break;
		case KEY_DOWN:
			if( ( password_SetFlag >= 1 ) && ( password_SetFlag <= 6 ) )
			{
				password_Counter++;
				if( password_Counter > 9 )
				{
					password_Counter = 0;
				}
				password_Set( password_Counter );
			}

			break;
	}
}

MENUITEM Menu_0_0_password =
{
	"密码设置",
	8,
	0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	( void* )0
};
#endif
/************************************** The End Of File **************************************/

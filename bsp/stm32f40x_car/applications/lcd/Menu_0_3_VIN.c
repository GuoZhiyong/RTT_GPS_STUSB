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

#if 1

#define  ASC_LEN 36

static char		* asc = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static int8_t	pos;

static char		vin[18];
static uint8_t	vin_pos;

/**/

static void msg( void *p )
{
}

/**/
static void display( void )
{
	uint8_t page, index;
	page	= pos / 20;
	index	= pos % 20;

	lcd_fill( 0 );
	if( vin_pos < 17 )
	{
		lcd_text12( 0, 4, "VIN", 3, LCD_MODE_INVERT );
		lcd_text12( 20, 4, vin, strlen( vin ), LCD_MODE_SET );
		lcd_text12( 0, 20, asc + page * 20, ( ASC_LEN - page * 20 ) < 20 ? ( ASC_LEN - page * 20 ) : 20, LCD_MODE_SET );
		lcd_text12( index * 6, 20, asc + pos, 1, LCD_MODE_INVERT );
	}else
	{
		lcd_text12( 16, 16, "车辆VIN设置完成", 15, LCD_MODE_SET );
	}
	lcd_update_all( );
}

/**/
static void show( void )
{
	vin_pos = 0;
	pos		= 0;
	memset( vin, 0, 18 );
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
			if( vin_pos > 16 )
			{
				pMenuItem = &Menu_0_4_Colour;
				pMenuItem->show( );
			}else
			{
				vin[vin_pos] = *( asc + pos );
				vin_pos++;
				display( );
			}
			break;
		case KEY_UP:
			if( pos == 0 )
			{
				pos = ASC_LEN;
			}
			pos--;
			display( );
			break;
		case KEY_DOWN:
			pos++;
			pos %= ASC_LEN;
			display( );
			break;
	}
}

/**/
static void timetick( unsigned int systick )
{
}

MENUITEM Menu_0_3_vin =
{
	"VIN设置",
	7,		  0,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

#else
#define  Sim_width1 6

static u8		VIN_SetFlag		= 1;
static u8		VIN_SetCounter	= 0;

unsigned char	select_vin[] = { 0x0C, 0x06, 0xFF, 0x06, 0x0C };
unsigned char	ABC[36][1] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z" };

DECL_BMP( 8, 5, select_vin );


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void Vin_Set( u8 par, u8 type1_2 )
{
	lcd_fill( 0 );
	lcd_text12( 0, 3, (char*)Menu_Vin_Code, VIN_SetFlag - 1, LCD_MODE_SET );

	if( type1_2 == 1 )
	{
		lcd_bitmap( par * Sim_width1, 14, &BMP_select_vin, LCD_MODE_SET );
		lcd_text12( 0, 19, "0123456789ABCDEFGHIJ", 20, LCD_MODE_SET );
	}else
	{
		lcd_bitmap( ( par - 20 ) * Sim_width1, 14, &BMP_select_vin, LCD_MODE_SET );
		lcd_text12( 0, 19, "KLMNOPQRSTUVWXYZ", 16, LCD_MODE_SET );
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
	CounterBack = 0;
	Vin_Set( VIN_SetCounter, 1 );
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
			memset( Menu_Vin_Code, 0, sizeof( Menu_Vin_Code ) );
			VIN_SetFlag		= 1;
			VIN_SetCounter	= 0;
			break;
		case KEY_OK:
			if( ( VIN_SetFlag >= 1 ) && ( VIN_SetFlag <= 17 ) )
			{
				Menu_Vin_Code[VIN_SetFlag - 1] = ABC[VIN_SetCounter][0];
				//rt_kprintf("\r\n VIN_set=%d,%s",VIN_SetFlag,Menu_Vin_Code);
				VIN_SetFlag++;
				VIN_SetCounter = 0;
				Vin_Set( 0, 1 );
			}else if( VIN_SetFlag == 18 )
			{
				//rt_kprintf("\r\n VIN_set=%d,%s(ok)",VIN_SetFlag,Menu_Vin_Code);
				VIN_SetFlag = 19;
				lcd_fill( 0 );
				lcd_text12( 0, 5, (char*)Menu_Vin_Code, 17, LCD_MODE_SET );
				lcd_text12( 25, 19, "VIN设置完成", 11, LCD_MODE_SET );
				lcd_update_all( );
				//rt_kprintf("\r\n VIN_set=%d,%s(ok--)",VIN_SetFlag,Menu_Vin_Code);
			}else if( VIN_SetFlag == 19 )
			{
				//rt_kprintf("\r\n VIN_set=%d,%s(return)",VIN_SetFlag,Menu_Vin_Code);
				VIN_SetFlag			= 1;
				CarSet_0_counter	= 4;

				pMenuItem = &Menu_0_loggingin;
				pMenuItem->show( );
			}

			break;
		case KEY_UP:
			if( ( VIN_SetFlag >= 1 ) && ( VIN_SetFlag <= 17 ) )
			{
				if( VIN_SetCounter == 0 )
				{
					VIN_SetCounter = 35;
				} else if( VIN_SetCounter >= 1 )
				{
					VIN_SetCounter--;
				}
				if( VIN_SetCounter < 20 )
				{
					Vin_Set( VIN_SetCounter, 1 );
				} else
				{
					Vin_Set( VIN_SetCounter, 2 );
				}
			}
			break;
		case KEY_DOWN:
			if( ( VIN_SetFlag >= 1 ) && ( VIN_SetFlag <= 17 ) )
			{
				VIN_SetCounter++;
				if( VIN_SetCounter > 35 )
				{
					VIN_SetCounter = 0;
				}
				if( VIN_SetCounter < 20 )
				{
					Vin_Set( VIN_SetCounter, 1 );
				} else
				{
					Vin_Set( VIN_SetCounter, 2 );
				}
			}
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
static void timetick( unsigned int systick )
{
}

MENUITEM Menu_0_3_vin =
{
	"VIN设置",
	7,		  0,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};
#endif

/************************************** The End Of File **************************************/

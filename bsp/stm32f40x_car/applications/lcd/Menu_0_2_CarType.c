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

static uint8_t	invert	= 0;
static uint8_t	count	= 0;

char			*car_type2[] = { "客运", "货运", "危品", "出租" };
char			*car_type1[] = { "大型", "中型", "小型", "微型" };


static int8_t	row = 0, col = 0;

static int8_t	row0_selected, row1_selected;

/**/
static void display( void )
{
	invert++;
	invert %= 2;

	lcd_fill( 0 );

	if( row <2 ) 
	{
		lcd_text12( 20, 4, car_type1[0], 4, LCD_MODE_SET );
		lcd_text12( 44, 4, car_type1[1], 4, LCD_MODE_SET );
		lcd_text12( 68, 4, car_type1[2], 4, LCD_MODE_SET );
		lcd_text12( 92, 4, car_type1[3], 4, LCD_MODE_SET );
		lcd_text12( 20, 18, car_type2[0], 4, LCD_MODE_SET );
		lcd_text12( 44, 18, car_type2[1], 4, LCD_MODE_SET );
		lcd_text12( 68, 18, car_type2[2], 4, LCD_MODE_SET );
		lcd_text12( 92, 18, car_type2[3], 4, LCD_MODE_SET );

		if( row == 0 )                                                          /*操作第一行*/
		{
			lcd_text12( 20 + col * 24, 4, car_type1[col], 4, invert * 2 + 1 );  /*闪烁 1 set 3 invert*/
		}else if( row == 1 )
		{
			lcd_text12( 20 + row0_selected * 24, 4, car_type1[row0_selected], 4, LCD_MODE_INVERT );
			lcd_text12( 20 + col * 24, 18, car_type2[col], 4, invert * 2 + 1 ); /*闪烁 1 set 3 invert*/
		}
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
	row				= 0;
	col				= 0;
	row0_selected	= -1;
	row0_selected	= -1;
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

			break;
		case KEY_OK:
			if( row > 2 )
			{
				pMenuItem = &Menu_1_Idle;
				pMenuItem->show( );
			}
			else if( row == 0 )
			{
				row0_selected = col;
			}
			else if( row == 1 )
			{
				row1_selected = col;
			}
			else if(row==2)
			{
				strcpy( jt808_param.id_0xF00A, car_type1[row0_selected] );
				strcat( jt808_param.id_0xF00A, car_type2[col] );
				param_save( );
				lcd_text12( 13, 16, "车辆类型设置完成", 16, LCD_MODE_SET );
			}
			row++;
			col		= 0;
			invert	= 1;
			display( );
			break;
		case KEY_UP:
			if( col == 0 )
			{
				col = 4;
			}
			col--;
			display( );
			break;
		case KEY_DOWN:
			col++;
			col %= 0x04;
			display( );
			break;
	}
}

/*每50ms调用一次*/
static void timetick( unsigned int systick )
{
	count++;
	if( count == 10 )
	{
		count=0;
		display( );
	}


	/*CounterBack++;
	   if(CounterBack!=MaxBankIdleTime*5)
	   return;
	   CounterBack=0;
	   pMenuItem=&Menu_0_loggingin;
	   pMenuItem->show();

	   CarType_counter=0;
	   CarType_Type=0;
	 */
}

MENUITEM Menu_0_2_CarType =
{
	"车辆类型设置",
	12,
	0,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

#else

static uint8_t	invert	= 0;
static uint8_t	count	= 0;

char			*car_type2[] = { "客运", "货运", "危品", "出租" };
char			*car_type1[] = { "大型", "中型", "小型", "微型" };


static int8_t	row = 0, col = 0;

static int8_t	row0_selected, row1_selected;

/**/
static void display( void )
{
	invert++;
	invert %= 2;

	lcd_fill( 0 );

	if( row > 1 ) /*设置完成*/
	{
		strcpy( jt808_param.id_0xF00A, car_type1[row0_selected] );
		strcat( jt808_param.id_0xF00A, car_type2[col] );
		param_save( );
		lcd_text12( 13, 16, "车辆类型设置完成", 16, LCD_MODE_SET );
	}else
	{
		lcd_text12( 20, 4, car_type1[0], 4, LCD_MODE_SET );
		lcd_text12( 44, 4, car_type1[1], 4, LCD_MODE_SET );
		lcd_text12( 68, 4, car_type1[2], 4, LCD_MODE_SET );
		lcd_text12( 92, 4, car_type1[3], 4, LCD_MODE_SET );
		lcd_text12( 20, 18, car_type2[0], 4, LCD_MODE_SET );
		lcd_text12( 44, 18, car_type2[1], 4, LCD_MODE_SET );
		lcd_text12( 68, 18, car_type2[2], 4, LCD_MODE_SET );
		lcd_text12( 92, 18, car_type2[3], 4, LCD_MODE_SET );

		if( row == 0 )                                                          /*操作第一行*/
		{
			lcd_text12( 20 + col * 24, 4, car_type1[col], 4, invert * 2 + 1 );  /*闪烁 1 set 3 invert*/
		}else if( row == 1 )
		{
			lcd_text12( 20 + row0_selected * 24, 4, car_type1[row0_selected], 4, LCD_MODE_INVERT );
			lcd_text12( 20 + col * 24, 18, car_type2[col], 4, invert * 2 + 1 ); /*闪烁 1 set 3 invert*/
		}
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
	row				= 0;
	col				= 0;
	row0_selected	= -1;
	row0_selected	= -1;
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
			if( row > 1 )
			{
				pMenuItem = &Menu_0_3_vin;
				pMenuItem->show( );
			}
			if( row == 0 )
			{
				row0_selected = col;
			}
			if( row == 1 )
			{
				row1_selected = col;
			}
			row++;
			col		= 0;
			invert	= 1;
			display( );
			break;
		case KEY_UP:
			if( col == 0 )
			{
				col = 4;
			}
			col--;
			display( );
			break;
		case KEY_DOWN:
			col++;
			col %= 0x04;
			display( );
			break;
	}
}

/*每50ms调用一次*/
static void timetick( unsigned int systick )
{
	count++;
	if( count == 10 )
	{
		count=0;
		display( );
	}


	/*CounterBack++;
	   if(CounterBack!=MaxBankIdleTime*5)
	   return;
	   CounterBack=0;
	   pMenuItem=&Menu_0_loggingin;
	   pMenuItem->show();

	   CarType_counter=0;
	   CarType_Type=0;
	 */
}

MENUITEM Menu_0_2_CarType =
{
	"车辆类型设置",
	12,
	0,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};



#endif


/************************************** The End Of File **************************************/

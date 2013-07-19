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

char			*car_type1	= "客运货运危品出租";
char			*car_type2	= "大型中型小型微型";

static int8_t	row = 0, col = 0;

static int8_t	row0_selected,row1_selected;

/**/
static void display( void )
{
	invert++;
	invert %= 2;

	lcd_fill( 0 );
	lcd_text12( 20, 4, car_type1, 16, LCD_MODE_SET );
	lcd_text12( 20, 18, car_type2, 16, LCD_MODE_SET );
	if( row == 0 )                                                                  /*操作第一行*/
	{
		lcd_text12( 20 + col * 24, 4, car_type1 + col * 4, 4, invert * 2 + 1 );     /*闪烁 1 set 3 invert*/
	}else if( row == 1 )
	{
		lcd_text12( 20 + row0_selected * 24, 4, car_type1 + row0_selected * 4, 4, LCD_MODE_INVERT );
		lcd_text12( 20 + col * 24, 18, car_type2 + col * 4, 4, invert * 2 + 1 );    /*闪烁 1 set 3 invert*/
	}else
	{
		lcd_fill( 0 );
		lcd_text12( 13, 16, "车辆类型设置完成", 16, LCD_MODE_SET );
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
			col = 0;
			invert=1;
			display();
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
	count %= 10;
	if( count == 0 ) 
	{
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

IMG_DEF			test_scr_CarType = { 12, 12, test_00 };

unsigned char	CarType_counter = 0;
unsigned char	CarType_Type	= 0;


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void CarType( unsigned char type_Sle, unsigned char sel )
{
	switch( type_Sle )
	{
		case 1:
			lcd_fill( 0 );
			if( sel == 0 )
			{
				lcd_text12( 24, 3, "车辆类型选择", 12, LCD_MODE_SET );
				lcd_text12( 0, 19, "1.  客运车", 10, LCD_MODE_SET );
			}else
			{
				lcd_text12( 12, 10, "车辆类型:客运车", 15, LCD_MODE_SET );
			}
			lcd_update_all( );
			break;
		case 2:
			lcd_fill( 0 );
			if( sel == 0 )
			{
				lcd_text12( 24, 3, "车辆类型选择", 12, LCD_MODE_SET );
				lcd_text12( 0, 19, "2.  货运车", 10, LCD_MODE_SET );
			}else
			{
				lcd_text12( 12, 10, "车辆类型:货运车", 15, LCD_MODE_SET );
			}
			lcd_update_all( );
			break;
		case 3:
			lcd_fill( 0 );
			if( sel == 0 )
			{
				lcd_text12( 24, 3, "车辆类型选择", 12, LCD_MODE_SET );
				lcd_text12( 0, 19, "3.  危品车", 10, LCD_MODE_SET );
			}else
			{
				lcd_text12( 12, 10, "车辆类型:危品车", 15, LCD_MODE_SET );
			}
			lcd_update_all( );
			break;
		case 4:
			lcd_fill( 0 );
			if( sel == 0 )
			{
				lcd_text12( 24, 3, "车辆类型选择", 12, LCD_MODE_SET );
				lcd_text12( 0, 19, "4.  大型车", 10, LCD_MODE_SET );
			}else
			{
				lcd_text12( 12, 10, "车辆类型:大型车", 15, LCD_MODE_SET );
			}
			lcd_update_all( );
			break;
		case 5:
			lcd_fill( 0 );
			if( sel == 0 )
			{
				lcd_text12( 24, 3, "车辆类型选择", 12, LCD_MODE_SET );
				lcd_text12( 0, 19, "5.  中型车", 10, LCD_MODE_SET );
			}else
			{
				lcd_text12( 12, 10, "车辆类型:中型车", 15, LCD_MODE_SET );
			}
			lcd_update_all( );
			break;
		case 6:
			lcd_fill( 0 );
			if( sel == 0 )
			{
				lcd_text12( 24, 3, "车辆类型选择", 12, LCD_MODE_SET );
				lcd_text12( 0, 19, "6.  小型车", 10, LCD_MODE_SET );
			}else
			{
				lcd_text12( 12, 10, "车辆类型:小型车", 15, LCD_MODE_SET );
			}
			lcd_update_all( );
			break;
		case 7:
			lcd_fill( 0 );
			if( sel == 0 )
			{
				lcd_text12( 24, 3, "车辆类型选择", 12, LCD_MODE_SET );
				lcd_text12( 0, 19, "7.  微型车", 10, LCD_MODE_SET );
			}else
			{
				lcd_text12( 12, 10, "车辆类型:微型车", 15, LCD_MODE_SET );
			}
			lcd_update_all( );
			break;
		case 8:
			lcd_fill( 0 );
			if( sel == 0 )
			{
				lcd_text12( 24, 3, "车辆类型选择", 12, LCD_MODE_SET );
				lcd_text12( 0, 19, "8.  出租车", 10, LCD_MODE_SET );
			}else
			{
				lcd_text12( 12, 10, "车辆类型:出租车", 15, LCD_MODE_SET );
			}
			lcd_update_all( );
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
	lcd_fill( 0 );
	lcd_text12( 24, 3, "车辆类型选择", 12, LCD_MODE_SET );
	lcd_text12( 0, 19, "按确认键选择车辆类型", 20, LCD_MODE_SET );
	lcd_update_all( );

	CarType_counter = 1;
	CarType_Type	= 1;

	CarType( CarType_counter, 0 );
	//--printf("\r\n车辆类型选择 = %d",CarType_counter);
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

			CarType_counter = 0;
			CarType_Type	= 0;
			break;
		case KEY_OK:
			if( CarType_Type == 1 )
			{
				CarType_Type = 2;
				CarType( CarType_counter, 1 );
				//printf("\r\nCarType_Type = %d",CarType_Type);
			}else if( CarType_Type == 2 )
			{
				CarType_Type = 3;
				lcd_fill( 0 );
				lcd_text12( 12, 10, "车辆类型选择完毕", 16, LCD_MODE_SET );
				lcd_update_all( );

				//写入车辆类型
				if( ( CarType_counter >= 1 ) && ( CarType_counter <= 8 ) )
				{
					memset( Menu_VechileType, 0, sizeof( Menu_VechileType ) );
				}

				if( CarType_counter == 1 )
				{
					memcpy( Menu_VechileType, "客运车", 6 );
				} else if( CarType_counter == 2 )
				{
					memcpy( Menu_VechileType, "货运车", 6 );
				} else if( CarType_counter == 3 )
				{
					memcpy( Menu_VechileType, "危品车", 6 );
				} else if( CarType_counter == 4 )
				{
					memcpy( Menu_VechileType, "大型车", 6 );
				} else if( CarType_counter == 5 )
				{
					memcpy( Menu_VechileType, "中型车", 6 );
				} else if( CarType_counter == 6 )
				{
					memcpy( Menu_VechileType, "小型车", 6 );
				} else if( CarType_counter == 7 )
				{
					memcpy( Menu_VechileType, "微型车", 6 );
				} else if( CarType_counter == 8 )
				{
					memcpy( Menu_VechileType, "出租车", 6 );
				}
			}else if( CarType_Type == 3 )
			{
				CarSet_0_counter	= 3; //设置第3项
				pMenuItem			= &Menu_0_loggingin;
				pMenuItem->show( );

				CarType_counter = 0;
				CarType_Type	= 0;
			}
			break;
		case KEY_UP:
			if( CarType_Type == 1 )
			{
				if( CarType_counter == 1 )
				{
					CarType_counter = 8;
				} else
				{
					CarType_counter--;
				}
				//printf("\r\n  up  车辆类型选择 = %d",CarType_counter);
				CarType( CarType_counter, 0 );
			}
			break;
		case KEY_DOWN:
			if( CarType_Type == 1 )
			{
				if( CarType_counter >= 8 )
				{
					CarType_counter = 1;
				} else
				{
					CarType_counter++;
				}

				//printf("\r\n down 车辆类型选择 = %d",CarType_counter);
				CarType( CarType_counter, 0 );
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

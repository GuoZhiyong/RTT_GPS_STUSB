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

static uint8_t display_row = 0;


/*
   显示
   没有的时刻怎么办?有可能不连续
display_row     0  =>    hmi_15min_speed_curr
	            1  =>    hmi_15min_speed_curr-1  
	            2  =>    hmi_15min_speed_curr-2     
 */
static void display( void )
{
	unsigned char t[50];
	uint8_t i,pos,row,hour,minute;	
	
	lcd_fill( 0 );

	for(i=0;i<3;i++)   /*显示三行*/
	{
		if(hmi_15min_speed_curr>=(display_row+i))
		{
			pos=hmi_15min_speed_curr-display_row-i;
		}
		else
		{
			pos=15+hmi_15min_speed_curr-display_row-i;
		}	
		if(hmi_15min_speed[pos].time!=0) /*有数据*/
		{
			hour=HOUR(hmi_15min_speed[pos].time);
			minute=MINUTE(hmi_15min_speed[pos].time);
			sprintf( (char*)t, "[%02d] %02d:%02d %02dkmh",display_row+i, hour,minute, hmi_15min_speed[pos].speed );
		}
		else
		{
			sprintf( (char*)t, "[%02d] --:-- --",display_row+i);
		}
		lcd_text12( 10, i*11, (char*)t, strlen(t), LCD_MODE_SET );
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
	display_row = 0;
	display( );
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
			if( display_row > 2 )
			{
				display_row -= 3;
				display( );
			}
			break;
		case KEY_DOWN:
			if( display_row < 12 )
			{
				display_row += 3;
				display( );
			}
			break;
	}
}

MENUITEM Menu_2_2_Speed15 =
{
	"停车前15min速度",
	15,				  0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

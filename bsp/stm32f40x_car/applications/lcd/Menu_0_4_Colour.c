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

u8				comfirmation_flag	= 0;
u8				col_screen			= 0;
u8				CarBrandCol_Cou		= 1;

unsigned char	car_col[13] = { "车牌颜色:蓝色" };


/**/
void car_col_fun( u8 par )
{
	//车牌颜色编码表
	if( par == 1 )
	{
		memcpy( Menu_VecLogoColor, "蓝色", 4 ); //   1
	}else if( par == 2 )
	{
		memcpy( Menu_VecLogoColor, "黄色", 4 ); //   2
	}else if( par == 3 )
	{
		memcpy( Menu_VecLogoColor, "黑色", 4 ); //   3
	}else if( par == 4 )
	{
		memcpy( Menu_VecLogoColor, "白色", 4 ); //   4
	}else if( par == 5 )
	{
		memcpy( Menu_VecLogoColor, "其他", 4 ); 
		par = 9;
	}                                           //   9

	Menu_color_num = par;

	memcpy( car_col + 9, Menu_VecLogoColor, 4 );
	lcd_fill( 0 );
	lcd_text12( 20, 10, (char*)car_col, 13, LCD_MODE_SET );
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
	col_screen	= 1;
	car_col_fun( 1 );
}

/**/
static void keypress( unsigned int key )
{
	switch( key )
	{
		case KEY_MENU:
			if( comfirmation_flag == 4 )
			{
				pMenuItem = &Menu_1_Idle;
				pMenuItem->show( );
			}else
			{
				pMenuItem = &Menu_0_loggingin;
				pMenuItem->show( );
			}
			col_screen			= 0;
			CarBrandCol_Cou		= 1;
			comfirmation_flag	= 0;
			break;
		case KEY_OK:
			if( col_screen == 1 )
			{
				col_screen			= 2;
				CarSet_0_counter	= 1;    //
				menu_color_flag		= 1;    //车牌颜色设置完成
				lcd_fill( 0 );
				lcd_text12( 20, 3, (char*)car_col, 13, LCD_MODE_SET );
				lcd_text12( 12, 18, "按确认键查看信息", 16, LCD_MODE_SET );
				lcd_update_all( );
			}else if( col_screen == 2 )
			{
				menu_color_flag = 0;

				col_screen			= 3;
				comfirmation_flag	= 1; //保存设置信息标志
				lcd_fill( 0 );
				lcd_text12( 0, 0, Menu_Car_license, 8, LCD_MODE_SET );
				lcd_text12( 54, 0, Menu_VechileType, 6, LCD_MODE_SET );
				lcd_text12( 96, 0, (char*)Menu_VecLogoColor, 4, LCD_MODE_SET );

				lcd_text12( 0, 11, "VIN", 3, LCD_MODE_SET );
				lcd_text12( 19, 11, (char*)Menu_Vin_Code, 17, LCD_MODE_SET );
				lcd_text12( 24, 22, "确定", 4, LCD_MODE_INVERT );
				lcd_text12( 72, 22, "取消", 4, LCD_MODE_SET );
				lcd_update_all( );
			}else if( comfirmation_flag == 1 )
			{
				col_screen			= 0;
				comfirmation_flag	= 4;
				//保存设置的信息
				lcd_fill( 0 );
				lcd_text12( 18, 3, "保存已设置信息", 14, LCD_MODE_SET );
				lcd_text12( 0, 18, "按菜单键进入待机界面", 20, LCD_MODE_SET );
				lcd_update_all( );

				//车牌号
				memset(jt808_param.id_0xF006, 0, 32 );
				memcpy(jt808_param.id_0xF006,Menu_Car_license,strlen(Menu_Car_license));
				//车辆类型
				memset(jt808_param.id_0xF008, 0, 32 );
				memcpy(jt808_param.id_0xF008,Menu_Car_license,strlen(Menu_Car_license));
				//车辆VIN
				memset(jt808_param.id_0xF005, 0, 32 );
				memcpy(jt808_param.id_0xF005,Menu_Vin_Code,17);

				// 车牌颜色
				jt808_param.id_0xF007 = Menu_color_num;
				
				//  存储
				param_save();
				
			}else if( comfirmation_flag == 2 )
			{
				col_screen			= 0;
				comfirmation_flag	= 3;
				lcd_fill( 0 );
				lcd_text12( 6, 3, "请确认是否重新设置", 18, LCD_MODE_SET );
				lcd_text12( 12, 18, "按确认键重新设置", 16, LCD_MODE_SET );
				lcd_update_all( );
			}else if( comfirmation_flag == 3 )
			{
				col_screen			= 0;
				comfirmation_flag	= 0;
				//重新设置
				pMenuItem = &Menu_0_loggingin;
				pMenuItem->show( );

				comfirmation_flag	= 0;
				col_screen			= 0;
				CarBrandCol_Cou		= 1;
			}

			break;
		case KEY_UP:
			if( col_screen == 1 )
			{
				CarBrandCol_Cou--;
				if( CarBrandCol_Cou < 1 )
				{
					CarBrandCol_Cou = 5;
				}
				car_col_fun( CarBrandCol_Cou );
			}else if( col_screen == 3 )
			{
				comfirmation_flag = 1;
				lcd_fill( 0 );
				lcd_text12( 0, 0, Menu_Car_license, 8, LCD_MODE_SET );
				lcd_text12( 54, 0, Menu_VechileType, 6, LCD_MODE_SET );
				lcd_text12( 96, 0, (char*)Menu_VecLogoColor, 4, LCD_MODE_SET );

				lcd_text12( 0, 11, "VIN", 3, LCD_MODE_SET );
				lcd_text12( 19, 11, (char*)Menu_Vin_Code, 17, LCD_MODE_SET );
				lcd_text12( 24, 22, "确定", 4, LCD_MODE_INVERT );
				lcd_text12( 72, 22, "取消", 4, LCD_MODE_SET );
				lcd_update_all( );
			}

			break;
		case KEY_DOWN:
			if( col_screen == 1 )
			{
				CarBrandCol_Cou++;
				if( CarBrandCol_Cou > 5 )
				{
					CarBrandCol_Cou = 1;
				}
				car_col_fun( CarBrandCol_Cou );
			}else if( col_screen == 3 )
			{
				comfirmation_flag = 2;
				lcd_fill( 0 );
				lcd_text12( 0, 0, Menu_Car_license, 8, LCD_MODE_SET );
				lcd_text12( 54, 0, Menu_VechileType, 6, LCD_MODE_SET );
				lcd_text12( 96, 0, (char*)Menu_VecLogoColor, 4, LCD_MODE_SET );

				lcd_text12( 0, 11, "VIN", 3, LCD_MODE_SET );
				lcd_text12( 19, 11, (char*)Menu_Vin_Code, 17, LCD_MODE_SET );
				lcd_text12( 24, 22, "确定", 4, LCD_MODE_SET );
				lcd_text12( 72, 22, "取消", 4, LCD_MODE_INVERT );
				lcd_update_all( );
			}

			break;
	}
}

/**/
static void timetick( unsigned int systick )
{
/*
   CounterBack++;
   if(CounterBack!=MaxBankIdleTime*5)
   return;
   CounterBack=0;
   pMenuItem=&Menu_0_loggingin;
   pMenuItem->show();


   col_screen=0;
   CarBrandCol_Cou=1;
   comfirmation_flag=0;*/
}

MENUITEM Menu_0_4_Colour =
{
	"车辆颜色设置",
	12,0,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

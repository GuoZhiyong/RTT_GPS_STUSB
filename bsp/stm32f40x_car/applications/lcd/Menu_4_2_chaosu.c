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
static unsigned char	ChaoSuNumScreen = 1;
static unsigned char	ChaoScreen		= 1; //新加


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void drawChaoSu_1( void )
{
	unsigned char	screenNUMchaosu = 0;
	unsigned char	Num[20]			= { "1.    驾驶证号码" };
	unsigned char	Code[19]		= { "                  " }; //

	screenNUMchaosu = ChaoSuNumScreen / 2;
	Num[0]			+= screenNUMchaosu;

	memcpy( Code, ChaosuJilu[screenNUMchaosu].PCard, 18 );
	lcd_fill( 0 );
	lcd_text12( 0, 3, ( char*)Num, sizeof( Num ), LCD_MODE_SET );
	lcd_text12( 15, 20, (char*)Code, sizeof( Code ), LCD_MODE_SET );
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
static void drawChaoSu_2( void )
{
	unsigned char	t_s[30], t_e[30], sp[30];
	unsigned char	screenNUMchaosu = 0;

	screenNUMchaosu = ChaoSuNumScreen / 2;

	lcd_fill( 0 );
	rt_kprintf( (char*)t_s, "S: %02d/%02d/%02d %02d:%02d:%02d", ChaosuJilu[screenNUMchaosu - 1].StartTime[0], ChaosuJilu[screenNUMchaosu - 1].StartTime[1], ChaosuJilu[screenNUMchaosu - 1].StartTime[2], ChaosuJilu[screenNUMchaosu - 1].StartTime[3], ChaosuJilu[screenNUMchaosu - 1].StartTime[4], ChaosuJilu[screenNUMchaosu - 1].StartTime[5] );
	rt_kprintf( (char*)t_e, "E: %02d/%02d/%02d %02d:%02d:%02d", ChaosuJilu[screenNUMchaosu - 1].EndTime[0], ChaosuJilu[screenNUMchaosu - 1].EndTime[1], ChaosuJilu[screenNUMchaosu - 1].EndTime[2], ChaosuJilu[screenNUMchaosu - 1].EndTime[3], ChaosuJilu[screenNUMchaosu - 1].EndTime[4], ChaosuJilu[screenNUMchaosu - 1].EndTime[5] );
	rt_kprintf( (char*)sp, "MaxSpeed:%04dKm/h", ChaosuJilu[screenNUMchaosu - 1].Speed );
	lcd_text12( 2, 2, (char*)t_s, strlen( (char*)t_s ), LCD_MODE_SET );
	lcd_text12( 2, 12, (char*)t_e, strlen( (char*)t_e ), LCD_MODE_SET );
	lcd_text12( 2, 22, (char*)sp, strlen( (char*)sp ), LCD_MODE_SET );
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

	ErrorRecord				= 0;    //疲劳超速记录错误清0
	StartDisTiredExpspeed	= 0;
	expsp_Flag				= 0;    //查看疲劳超速报警记录过程标志清0;

	lcd_fill( 0 );
	lcd_text12( 0, 10, "按确认键查看超速记录", 20, LCD_MODE_SET );
	lcd_update_all( );

//读超速驾驶记录
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
	unsigned char	temp		= 0;
	unsigned char	exspeed_num = 0;

	switch( key )
	{
		case KEY_MENU:
			pMenuItem = &Menu_4_InforTirExspd;
			pMenuItem->show( );

			CounterBack				= 0;
			ErrorRecord				= 0;    //疲劳超速记录错误清0
			StartDisTiredExpspeed	= 0;
			expsp_Flag				= 0;    //查看疲劳报警记录过程标志清0;

			ChaoScreen = 1;

			break;
		case KEY_OK:
			if( ChaoScreen == 1 )
			{
				ChaoScreen = 2;
#if NEED_TODO
				exspeed_num = Api_DFdirectory_Query( tired_warn, 0 ); //查询当前疲劳驾驶记录数目
#endif
				if( exspeed_num > 0 )
				{
					expsp_Flag = 1;
					//rt_kprintf("\r\n  已有  疲劳驾驶 的记录 %d 条\r\n",TiredDrv_write);
					if( exspeed_num >= 3 )
					{
						ReadEXspeed( 3 );
					} else
					{
						ReadEXspeed( exspeed_num );
					}
					Dis_chaosu( data_tirexps );
				}else
				{
					expsp_Flag = 2;
					//rt_kprintf("\r\n无超速驾驶记录  read\r\n");
				}
			}else if( ChaoScreen == 2 )
			{
				ChaoScreen = 3;
				if( expsp_Flag == 3 )
				{
					expsp_Flag				= 4;
					ChaoSuNumScreen			= 0;
					StartDisTiredExpspeed	= 1;
					lcd_fill( 0 );
					lcd_text12( 18, 10, "按下翻逐条查看", 14, LCD_MODE_SET );
					lcd_update_all( );
				}
			}

			break;
		case KEY_UP:
			if( expsp_Flag == 4 )
			{
				if( ChaoSuNumScreen > 0 )
				{
					ChaoSuNumScreen--;
				}
				if( ChaoSuNumScreen < 1 )
				{
					ChaoSuNumScreen = 1;
				}
				if( ChaoSuNumScreen % 2 == 1 )
				{
					drawChaoSu_1( );
				} else
				{
					if( ErrorRecord == 0 )
					{
						StartDisTiredExpspeed = 0;
						drawChaoSu_2( ); //开始时间  结束时间
					}
				}
			}
			break;
		case KEY_DOWN:
			if( expsp_Flag == 4 )
			{
				ChaoSuNumScreen++;


				/*if(ExpSpdRec_write>=3)
				   temp=6;
				   else
				   temp=ExpSpdRec_write*2;*/
				if( ChaoSuNumScreen >= temp )
				{
					ChaoSuNumScreen = temp;
				}

				if( ChaoSuNumScreen % 2 == 1 )
				{
					drawChaoSu_1( );
				} else
				{
					if( ErrorRecord == 0 )
					{
						StartDisTiredExpspeed = 0;
						drawChaoSu_2( ); //开始时间  结束时间
					}
				}
			}
			break;
	}
}

MENUITEM Menu_4_2_chaosu =
{
	"超速驾驶查看",
	12,				  0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

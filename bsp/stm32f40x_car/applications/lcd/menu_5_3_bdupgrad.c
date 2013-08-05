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
#include "hmi.h"
#include "gps.h"


/*
   是否在升级状态，禁用按键判断
   0;开始 等待升级
   1:结束，无论成功与否
   2:升级过程中，不响应按键

 */

#define BD_IDLE 0                           /*空闲*/
#define BD_BUSY 1                           /*操作过程中*/
#define BD_COMPLETE 2                           /*操作完成,不论成功失败*/


static uint8_t		fupgrading	= BD_IDLE;
static rt_thread_t	tid_upgrade = RT_NULL;  /*开始更新*/

#define OPER_MENU_SELECT	0xFF            /*为了跟pos相对应*/
#define OPER_CHECK_VER		0               /*检查版本*/
#define OPER_UPGRADE		1               /*升级*/
#define OPER_SWITCH_MODE	2               /*工作模式切换*/
#define OPER_RESTART		3               /*重新启动*/

static uint8_t oper_mode = OPER_MENU_SELECT;

struct _stu_menu
{
	uint8_t left;
	uint8_t top;
	char	* caption;
};
static struct _stu_menu m[4] =
{
	{ 12, 4,  "版本查询" },
	{ 72, 4,  " U盘升级" },
	{ 12, 18, "模式切换" },
	{ 72, 18, "模块重启" },
};
static uint8_t			pos;

char					*bd_mode[3] = { "单北斗", "单GPS", "双模" };
char					*bd_restart[2] = { "冷启动", "热启动" };

/*显示菜单*/
static void menu_disp( void )
{
	uint8_t i;
	lcd_fill( 0 );
	switch( oper_mode )
	{
		case OPER_MENU_SELECT:
			for( i = 0; i < 4; i++ )
			{
				lcd_text12( m[i].left, m[i].top, m[i].caption, 8, LCD_MODE_SET );
			}
			lcd_text12( m[pos].left, m[pos].top, m[pos].caption, 8, LCD_MODE_INVERT );
			break;
		case OPER_SWITCH_MODE:

			lcd_text12( 0, 4, "选择北斗工作模式", 16, LCD_MODE_SET );
			for( i = 0; i < 3; i++ )
			{
				lcd_text12( i * 40, 18, bd_mode[i], strlen( bd_mode[i] ), LCD_MODE_SET );
			}

			lcd_text12( pos * 40, 18, bd_mode[pos], strlen( bd_mode[pos] ), LCD_MODE_INVERT );
			break;
		case OPER_RESTART:
			lcd_text12( 0, 4, "重启北斗模块", 12, LCD_MODE_SET );
			for( i = 0; i < 2; i++ )
			{
				lcd_text12( 16 + i * 48, 18, bd_restart[i], 6, LCD_MODE_SET );
			}
			lcd_text12( pos * 40, 18, bd_restart[pos], strlen( bd_restart[pos] ), LCD_MODE_INVERT );

			break;
	}
	lcd_update_all( );
}

/**/
static void show( void )
{
	fupgrading	= BD_IDLE;
	oper_mode=OPER_MENU_SELECT;
	pos			= 0;
	menu_disp( );
}

/*
   提供回调函数用以显示信息
 */

static void msg( void *p )
{
	//char buf[32];
	unsigned int	len;
	char			*pinfo;
	if( fupgrading )
	{
		lcd_fill( 0 );

		pinfo	= (char*)p;
		len		= strlen( pinfo );
		lcd_text12( 35, 10, pinfo + 1, len - 1, LCD_MODE_SET );
		if( pinfo[0] == 'E' )                   /*出错或结束*/
		{
			fupgrading=BD_COMPLETE;
			tid_upgrade=RT_NULL;
		}
		lcd_update_all( );
	}else
	{
		menu_disp( );
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
static void keypress( unsigned int key )
{
	if(fupgrading==BD_BUSY)
	{
		return;
	}
	switch( key )
	{
		case KEY_MENU:
			if( fupgrading == BD_IDLE )
			{
				pMenuItem = &Menu_5_other;
				pMenuItem->show( );
			}else
			{
			fupgrading=BD_IDLE;
			oper_mode = OPER_MENU_SELECT;
			pos=0;
				menu_disp( );
			}

			break;
		case KEY_OK:

			if(fupgrading==BD_COMPLETE)
			{
				fupgrading=BD_IDLE;
				oper_mode = OPER_MENU_SELECT;
				pos=0;
				menu_disp();
				break;
			}
			if( oper_mode == OPER_MENU_SELECT )
			{
				switch( pos )
				{
					case  0:
					case  1:
						if( pos & 0x01 )
						{
							tid_upgrade = rt_thread_create( "upgrade", thread_gps_upgrade_udisk, (void*)msg, 1024, 5, 5 );
						}else
						{
							tid_upgrade = rt_thread_create( "bd_ver", thread_gps_check_ver, (void*)msg, 1024, 5, 5 );
						}
						if( tid_upgrade != RT_NULL )
						{
							fupgrading = BD_BUSY;
							rt_thread_startup( tid_upgrade );
						}else
						{
							msg( "E操作执行失败" );
						}
						break;
					case 2:
						pos			= gps_status.Position_Moule_Status - 1;
						oper_mode	= OPER_SWITCH_MODE;
						menu_disp( );
						break;
					case 3:
						pos = 0;
						menu_disp( );
						oper_mode = OPER_RESTART;
						break;
				}
			}
			else if( oper_mode == OPER_SWITCH_MODE )
			{
				gps_mode(pos+1);
				oper_mode=OPER_MENU_SELECT;
				pos=0;
				pop_msg("模式切换完成",RT_TICK_PER_SECOND*3);
			}
			else if( oper_mode == OPER_RESTART)
			{
				
				oper_mode=OPER_MENU_SELECT;
				pos=0;
				pop_msg("模块重启完成",RT_TICK_PER_SECOND*3);
			}
			break;
		case KEY_UP:
			if( oper_mode == OPER_MENU_SELECT )
			{
				if( pos == 0 )
				{
					pos = 4;
				}
			}else if( oper_mode == OPER_SWITCH_MODE )
			{
				if( pos == 0 )
				{
					pos = 3;
				}
			}
			else if( oper_mode == OPER_RESTART)
			{
				if( pos == 0 )
				{
					pos = 2;
				}
			}
			pos--;
			menu_disp( );
			break;
		case KEY_DOWN:
			pos++;
			if( oper_mode == OPER_MENU_SELECT )
			{
				pos%=4;
			}
			if( oper_mode == OPER_SWITCH_MODE)
			{
				pos%=3;
			}
			if( oper_mode == OPER_RESTART)
			{
				pos%=2;
			}
			menu_disp( );
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
static void timetick( unsigned int tick )
{
}

MENUITEM Menu_5_3_bdupgrade =
{
	"北斗模块操作",
	12,			   0,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

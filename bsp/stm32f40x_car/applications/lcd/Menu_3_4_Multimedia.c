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


/*
   要能查看记录?

   图片记录
    获取图片记录数据 允许导出或发送
   拍照上传
   当前拍照
 */
static uint8_t pos;

#define SCR_PHOTO_MENU		0
#define SCR_PHOTO_SELECT	1
#define SCR_PHOTO_TAKE		2

static uint8_t scr_mode = SCR_PHOTO_MENU;  /*当前显示的界面状态*/

/**/
static void display( void )
{
	lcd_fill( 0 );
	switch( scr_mode )
	{
		case SCR_PHOTO_MENU:
			pos &= 0x01;
			lcd_text12( 5, 4, "1.图片记录", 10, 3 - pos * 2 );
			lcd_text12( 5, 20, "2.拍照上传", 10, pos * 2 + 1 );
			break;
		case SCR_PHOTO_SELECT:
			break;
		case SCR_PHOTO_TAKE:
			break;
	}
	lcd_update_all( );
}

/*处理拍照及上传的过程*/
static void msg( void *p )
{
	pMenuItem->tick = rt_tick_get( );
}

/**/
static void show( void )
{
	pMenuItem->tick = rt_tick_get( );
	pos				= 0;
	scr_mode		= SCR_PHOTO_MENU;
	display( );
}

/*按键处理，拍照或上传过程中如何判断?*/
static void keypress( unsigned int key )
{
	switch( key )
	{
		case KEY_MENU:
			pMenuItem = &Menu_3_InforInteract;
			pMenuItem->show( );
			break;
		case KEY_OK:
			if( scr_mode == SCR_PHOTO_MENU )
			{
				if( pos == 0 ) /*图片记录*/
				{
				}else
				{
				}
			}

			break;
		case KEY_UP:
			pos--;
			display( );
			break;
		case KEY_DOWN:
			pos++;
			display( );
			break;
	}
}

MENUITEM Menu_3_4_Multimedia =
{
	"图片拍照",
	8,				  0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

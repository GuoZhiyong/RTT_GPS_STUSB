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


/*显示信息*/
static void msg( void *p )
{
}

/**/
static void show( void )
{
}

/**/
static void keypress( unsigned int key )
{
	switch( key )
	{
		case KEY_MENU:
			break;
		case KEY_OK:
			break;
		case KEY_UP:
			break;
		case KEY_DOWN:
			break;
	}
}

/**/
static void timetick( unsigned int systick )
{
}

MENUITEM Menu_3_6_Record =
{
	"消息",
	4,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

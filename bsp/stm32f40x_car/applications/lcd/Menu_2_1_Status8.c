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

#include "jt808_vehicle.h"

char* caption[10]={"紧急","启动","输入","远光","车门","喇叭","左转","右转","刹车","雨刷"};


static void draw(void)
{
	uint8_t i;
	lcd_fill(0);
	for(i=0;i<5;i++)
	{
		lcd_text12(i*24,4,caption[i],4,PIN_IN[i].value*2+1);  // SET=1 INVERT=3
		lcd_text12(i*24,20,caption[i+5],4,PIN_IN[i+5].value*2+1);  // SET=1 INVERT=3
	}
	lcd_update_all();
}


/**/
static void msg( void *p )
{

}

/**/
static void show( void )
{
	pMenuItem->tick=rt_tick_get();
	draw();
}

/**/
static void keypress( unsigned int key )
{
	switch( key )
	{
		case KEY_MENU:
			pMenuItem = &Menu_2_InforCheck;
			pMenuItem->show( );
			CounterBack = 0;
			break;
		default:	
			draw();
			break;
	}
}


static void timetick(unsigned int tick)
{
	draw();
	timetick_default(tick);
}


MENUITEM Menu_2_1_Status8 =
{
	"信号线状态",
	10,0,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

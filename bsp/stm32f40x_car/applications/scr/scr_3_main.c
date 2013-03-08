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
#include "scr.h"


extern SCR scr_3_1_recorderdata;
extern SCR  scr_3_2_signal;

static SCR_ITEM scr_item[] =
{
	{ "主菜单",			6,	0				 },
	{ "1.记录仪数据",	12, &scr_3_1_recorderdata },
	{ "2.信号采集",		10, &scr_3_2_signal },
	{ "3.定位信息",		10, &scr_3_1_recorderdata },
	{ "4.车辆参数设置", 14, &scr_3_1_recorderdata },
	{ "5.上网参数设置", 14, &scr_3_1_recorderdata },
	{ "6.高级属性",		10, &scr_3_1_recorderdata },
	{ "7.短信息",		8, &scr_3_1_recorderdata },
	{ "8.电话管理",		10, &scr_3_1_recorderdata },
	{ "9.音视频管理",	12, &scr_3_1_recorderdata },
};


static uint8_t	selectpos = 1;  /*选定的位置*/


/*显示菜单*/
static void menudisplay( void )
{
	lcd_fill( 0 );
	if( selectpos & 0x01 )      /*是单数*/
	{
		lcd_text12( 0, 0, scr_item[selectpos-1].text, scr_item[selectpos-1].len, LCD_MODE_SET );
		lcd_fill_rect(0,16,121,28,LCD_MODE_SET);
		lcd_text12( 0, 16, scr_item[selectpos].text, scr_item[selectpos].len, LCD_MODE_INVERT);
	}
	else
	{
		lcd_fill_rect(0,0,121,12,LCD_MODE_SET);
		lcd_text12( 0, 0, scr_item[selectpos].text, scr_item[selectpos].len, LCD_MODE_INVERT);
		lcd_text12( 0, 16, scr_item[selectpos+1].text, scr_item[selectpos+1].len, LCD_MODE_SET );
	}
	lcd_update_all( );
	
}

/***/
static void show( void )
{
	selectpos = 1;
	menudisplay( );
}

/*按键处理*/
static void keypress(void *thiz,unsigned int key )
{
	switch( key )
	{
		case KEY_MENU_PRESS:
			*((PSCR)thiz)=*((PSCR)thiz->parent);
			((PSCR)thiz)->show();
			break;
		case KEY_UP_PRESS:
			selectpos--;
			if( selectpos == 0 ) selectpos = 9;
			menudisplay();
			break;
		case KEY_DOWN_PRESS:
			selectpos++;
			if( selectpos == 10 )selectpos = 1;
			menudisplay();
			break;
		case KEY_OK_PRESS:
			scr_item[selectpos].scr->parent=(PSCR)thiz;
			*((PSCR)thiz)=scr_item[selectpos].scr;
			((PSCR)thiz)->show();
			break;
	}
}

/*系统时间*/
static void timetick(void *thiz, unsigned int systick )
{
}

/**/
static void msg(  void *p )
{
}

SCR scr_3_main =
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};

/************************************** The End Of File **************************************/

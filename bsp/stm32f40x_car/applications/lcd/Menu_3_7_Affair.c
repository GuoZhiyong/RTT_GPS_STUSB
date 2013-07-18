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
#include <stdio.h>
#include <string.h>
#include "sed1520.h"

static uint8_t	count, pos;

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void display( void )
{
	char buf[32];
	EVENT* evt;
	uint8_t index=pos&0xFE;	/*对齐到偶数页*/
	lcd_fill( 0 );
	if( count == 0 )
	{
		//lcd_text12( ( 122 - 6 * 12 ) / 2, 18, "[无事件信息]", 6*12,LCD_MODE_SET );
		strcpy(buf,"[无事件信息]");
		lcd_text12( ( 122 - 6 * 12 ) / 2, 18, buf, strlen(buf),LCD_MODE_SET );
	}
	else
	{
		evt=(EVENT*)(event_buf+64*index);
		sprintf(buf,"%02d %s",evt->id,evt->body);
		lcd_text12(0,4,buf,strlen(buf),3-(pos&0x01)*2);  /*SET=1 INVERT=3*/
		if((index+1)<count)
		{
			evt=(EVENT*)(event_buf+64*index+64);
			sprintf(buf,"%02d %s",evt->id,evt->body);
			lcd_text12(0,18,buf,strlen(buf),(pos&0x01)*2+1);  /*SET=1 INVERT=3*/
		}	
	}
	lcd_update_all( );
}

/**/
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
	count			= jt808_event_get();
	rt_kprintf("count=%d\n",count);
	pos				= 0;
	display( );
}

/**/
static void keypress( unsigned int key )
{
	uint8_t buf[32];
	switch( key )
	{
		case KEY_MENU:
			if( event_buf!= RT_NULL )
			{
				rt_free( event_buf );
				event_buf=RT_NULL;
			}
			pMenuItem = &Menu_3_InforInteract;
			pMenuItem->show( );
			break;
		case KEY_OK:  /*事件报告*/
			buf[0]=((EVENT*)(event_buf+pos*64))->id;
			jt808_tx(0x0301,buf,1);
			break;
		case KEY_UP:
			if( pos )
			{
				pos--;
			}
			display( );
			break;
		case KEY_DOWN:
			if( pos < (count-1) )
			{
				pos++;
			}
			display( );
			break;
	}
}

/*检查是否回到主界面*/
void timetick( unsigned int tick )
{
	if( ( tick - pMenuItem->tick ) >= 100 * 10 )
	{
		if( event_buf != RT_NULL )
		{
			rt_free(event_buf );
			event_buf=RT_NULL;
		}
		pMenuItem = &Menu_1_Idle;
		pMenuItem->show( );
	}
}

MENUITEM Menu_3_7_Affair =
{
	"事件信息",
	8,		   0,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

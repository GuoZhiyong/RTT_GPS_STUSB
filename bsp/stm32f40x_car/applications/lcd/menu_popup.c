/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		// �ļ���
 * Author:			// ����
 * Date:			// ����
 * Description:		// ģ������
 * Version:			// �汾��Ϣ
 * Function List:	// ��Ҫ�������书��
 *     1. -------
 * History:			// ��ʷ�޸ļ�¼
 *     <author>  <time>   <version >   <desc>
 *     David    96/10/12     1.0     build this moudle
 ***********************************************************/
#include "Menu_Include.h"
#include "sed1520.h"

#include <string.h>
static unsigned int tick_end;

static show_parent( void )
{
	MENUITEM *tmp;

	if( pMenuItem->parent != (void*)0 )
	{
		tmp					= pMenuItem->parent;
		pMenuItem->parent	= (void*)0;
		pMenuItem			= tmp;
	}else
	{
		pMenuItem = &Menu_1_Idle;
	}
	pMenuItem->show( );
}

/*��ʾ��Ϣ*/
static void msg( void *p )
{
	uint8_t len = strlen( (char*)p );
	lcd_fill( 0 );
	lcd_text12( ( 122 - len*6 ) >> 1, 10, (char*)p, len, LCD_MODE_SET );
	lcd_update_all( );
}

/*��ʾ*/
static void show( void )
{
	tick_end = rt_tick_get( ) + pMenuItem->tick; /*����ʱ��*/
}

/**/
static void keypress( unsigned int key )
{
	switch( key )
	{
		case KEY_MENU:
		case KEY_OK:
		case KEY_UP:
		case KEY_DOWN:
			show_parent( );
			break;
	}
}

/**/
static void timetick( unsigned int systick )
{
	if( systick >= tick_end )
	{
		show_parent( );
	}
}

MENUITEM Menu_Popup =
{
	"��Ϣ",
	4,0,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

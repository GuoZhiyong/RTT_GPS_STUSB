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


/*
   Ҫ�ܲ鿴��¼?

   ͼƬ��¼
    ��ȡͼƬ��¼���� ����������
   �����ϴ�
   ��ǰ����
 */
static uint8_t pos;

#define SCR_PHOTO_MENU		0
#define SCR_PHOTO_SELECT	1
#define SCR_PHOTO_TAKE		2

static uint8_t scr_mode = SCR_PHOTO_MENU;  /*��ǰ��ʾ�Ľ���״̬*/

/**/
static void display( void )
{
	lcd_fill( 0 );
	switch( scr_mode )
	{
		case SCR_PHOTO_MENU:
			pos &= 0x01;
			lcd_text12( 5, 4, "1.ͼƬ��¼", 10, 3 - pos * 2 );
			lcd_text12( 5, 20, "2.�����ϴ�", 10, pos * 2 + 1 );
			break;
		case SCR_PHOTO_SELECT:
			break;
		case SCR_PHOTO_TAKE:
			break;
	}
	lcd_update_all( );
}

/*�������ռ��ϴ��Ĺ���*/
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

/*�����������ջ��ϴ�����������ж�?*/
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
				if( pos == 0 ) /*ͼƬ��¼*/
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
	"ͼƬ����",
	8,				  0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

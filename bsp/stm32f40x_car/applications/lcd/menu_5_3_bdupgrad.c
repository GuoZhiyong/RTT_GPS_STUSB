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
#include <string.h>
#include "sed1520.h"

#include "gps.h"


/*
   �Ƿ�������״̬�����ð����ж�
   0;��ʼ �ȴ�����
   1:���������۳ɹ����
   2:���������У�����Ӧ����

 */

#define BD_UPGRADE_IDLE 0
#define BD_UPGRADE_END	1
#define BD_UPGRADING	2

static uint8_t		fupgrading	= BD_UPGRADE_IDLE;
static rt_thread_t	tid_upgrade = RT_NULL; /*��ʼ����*/

/**/
static void show( void )
{
	lcd_fill( 0 );
	lcd_text12( ( 122 - 16 * 6 ) / 2, 8, "U�̸����ļ�����?", 16, LCD_MODE_SET );
	lcd_text12( ( 122 - 14 * 6 ) / 2, 20, "��[ȷ�ϼ�]��ʼ", 14, LCD_MODE_SET );
	fupgrading=BD_UPGRADE_IDLE;
	lcd_update_all( );
}

/*
   �ṩ�ص�����������ʾ��Ϣ
 */

static void msg( void *p )
{
	//char buf[32];
	unsigned int	len;
	char			*pinfo;
	lcd_fill( 0 );
	lcd_text12( 0, 3, "����", 4, LCD_MODE_SET );
	lcd_text12( 0, 17, "����", 4, LCD_MODE_SET );
	pinfo	= (char*)p;
	len		= strlen( pinfo );
	lcd_text12( 35, 10, pinfo + 1, len - 1, LCD_MODE_SET );
	if( pinfo[0] == 'E' ) /*��������*/
	{
		fupgrading	= BD_UPGRADE_END;
		tid_upgrade = RT_NULL;
		rt_kprintf( "\nfupgrading=%d", fupgrading );
	}

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
static void keypress( unsigned int key )
{
	switch( key )
	{
		case KEY_MENU:
			if( fupgrading < BD_UPGRADING )
			{
				pMenuItem = &Menu_5_other;
				pMenuItem->show( );
			}

			break;
		case KEY_OK:
			if( fupgrading != BD_UPGRADE_IDLE )
			{
				break;
			}
			tid_upgrade = rt_thread_create( "upgrade", thread_gps_upgrade_udisk, (void*)msg, 1024, 5, 5 );
			if( tid_upgrade != RT_NULL )
			{
				fupgrading = BD_UPGRADING;
				msg( "I�ȴ�U������" );
				rt_thread_startup( tid_upgrade );
			}else
			{
				fupgrading = BD_UPGRADE_IDLE;
				msg( "E�̴߳���ʧ��" );
			}
			break;
		case KEY_UP:
			break;
		case KEY_DOWN:
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
	"������Ϣ������",
	14,		   0,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

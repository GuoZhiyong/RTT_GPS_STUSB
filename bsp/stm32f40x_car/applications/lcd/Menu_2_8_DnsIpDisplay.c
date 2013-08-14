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
#include  <string.h>

#include "sed1520.h"

static uint8_t pos;

/*��ʾ������Ϣ*/
static void display( )
{
	char	buf[24];
	uint8_t page;

	lcd_fill( 0 );

	page = pos >> 1;

	if( pos & 0x01 )
	{
		lcd_text12( 0, 4, gsm_socket[page].ip_addr, strlen( gsm_socket[page].ip_addr ), LCD_MODE_SET );
		sprintf( buf, "�˿�:%d", gsm_socket[page].port );
		lcd_text12( 0, 18, buf, strlen( buf ), LCD_MODE_SET );
	}else
	{
		if( gsm_socket[page].state == CONNECTED )
		{
			sprintf( buf, "����%d   ������", page );
		}else
		{
			sprintf( buf, "����%d   δ����", page );
		}
		lcd_text12( 0, 4, buf, 14, LCD_MODE_SET );
		lcd_text12( 0, 18, gsm_socket[page].ipstr, strlen( gsm_socket[page].ipstr ), LCD_MODE_SET );
	}
	lcd_update_all( );
}

/**/
static void msg( void *p )
{
}

/**/
static void show( void )
{
	pMenuItem->tick = rt_tick_get( );
	pos				= 0;
	display( );
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
			pMenuItem = &Menu_2_InforCheck;
			pMenuItem->show( );
			break;
		case KEY_OK:
			break;
		case KEY_UP:
			if( pos == 0 )
			{
				pos =6;
			}
			pos--;
			display( );
			break;
		case KEY_DOWN:
			pos++;
			pos %= 6;
			display( );
			break;
	}
}

MENUITEM Menu_2_8_DnsIpDisplay =
{
	"��������",
	8,				  0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

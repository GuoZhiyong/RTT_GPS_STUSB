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
static uint8_t	count;
static uint8_t	pos;

/**/
static void phonebook_display( void )
{
	uint8_t *p;
	uint8_t len_tel, len_man;
	char	buf[32];
	lcd_fill( 0 );
	if( count == 0 )
	{
		lcd_fill( 0 );
		lcd_text12( ( 122 - 12 * 6 ) >> 1, 14, "[�绰��Ϊ��]", 12, LCD_MODE_SET );
	}else
	{
		p		= phonebook_buf + pos * 64; /*��ʼ��һ��'P'*/
		len_tel = p[2];
		len_man = p[len_tel + 3];
		memset( buf, 0, 32 );
		sprintf( buf, "[%02d] ", pos );
		if( len_man > 14 )
		{
			len_man = 14;
		}
		strncpy( buf + 5, (char*)( p + len_tel + 4 ), len_man );
		lcd_text12( 0, 4, buf, strlen( buf ), LCD_MODE_SET );
		lcd_text12( 0, 18, (char*)( p + 3 ), len_tel, LCD_MODE_SET );
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
	count			= jt808_phonebook_get( );
	pos				= 0;
	phonebook_display( );
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
			pMenuItem = &Menu_5_other;
			pMenuItem->show( );
			break;
		case KEY_OK:

			break;
		case KEY_UP:
			if( pos )
			{
				pos--;
				phonebook_display( );
			}
			break;
		case KEY_DOWN:
			if( pos < count - 1 )
			{
				pos++;
				phonebook_display( );
			}
			break;
	}
}

MENUITEM Menu_5_1_TelDis =
{
	"�绰���鿴",
	10,				  0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

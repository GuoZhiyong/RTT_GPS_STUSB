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
#include  <string.h>
#include "Menu_Include.h"
#include "sed1520.h"

/**/
static uint8_t	pos;

static char		*op[] =
{
	"�����������",
	"��ӡ��",
	"�޸Ĳ���",
};

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void msg( void *p )
{
}

/**/
static void display( void )
{
	uint8_t index = pos & 0xFE;
	lcd_fill( 0 );

	lcd_update_all( );
}

/**/
static void show( void )
{
	pMenuItem->tick = rt_tick_get( );

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
			pMenuItem = &Menu_0_loggingin;
			pMenuItem->show( );
			break;
		case KEY_OK:

			break;
		case KEY_UP:
			if( pos == 0 )
			{
				pos = 2;
			}
			pos--;
			display( );
			break;
		case KEY_DOWN:
			pos++;
			pos %= 2;
			display( );
			break;
	}
}

/**/

MENUITEM Menu_0_6_Engineer =
{
	"����ģʽ",
	8,				  0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};
/************************************** The End Of File **************************************/


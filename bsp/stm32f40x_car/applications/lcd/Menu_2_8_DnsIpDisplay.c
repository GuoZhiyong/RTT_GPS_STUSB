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
static void display()
{

	lcd_fill(0);
	lcd_text12((122-8*6)>>1,14,"�д�����",8,LCD_MODE_SET);
	lcd_update_all( );
}

/**/
static void msg( void *p )
{
}

/**/
static void show( void )
{
	pMenuItem->tick=rt_tick_get();
	pos=0;
	display();
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
			CounterBack = 0;
			break;
		case KEY_OK:
			break;
		case KEY_UP:
			break;
		case KEY_DOWN:
			break;
	}
}


MENUITEM Menu_2_8_DnsIpDisplay =
{
	"��������",
	8,0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

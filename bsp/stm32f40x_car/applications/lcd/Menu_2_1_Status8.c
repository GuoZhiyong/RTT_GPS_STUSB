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
#include "Lcd.h"
#include <string.h>

static u8 signal_counter = 0;


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
	char	*pinfor;
	char	len = 0;

	pinfor	= (char*)p;
	len		= strlen( pinfor );

	lcd_fill( 0 );
	lcd_text12( 0, 10, pinfor, len, LCD_MODE_SET );
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
static void show( void )
{
	msg( XinhaoStatus );
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
	switch( KeyValue )
	{
		case KeyValueMenu:
			pMenuItem = &Menu_2_InforCheck;
			pMenuItem->show( );
			CounterBack = 0;
			break;
		default:	
			msg( XinhaoStatus );
			break;
	}
	KeyValue = 0;
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
static void timetick( unsigned int systick )
{
	signal_counter++;
	if( signal_counter >= 10 )
	{
		signal_counter = 0;
		msg( XinhaoStatus );
	}

	CounterBack++;
	if( CounterBack != MaxBankIdleTime )
	{
		return;
	}
	CounterBack = 0;

	pMenuItem = &Menu_1_Idle;
	pMenuItem->show( );
}

MENUITEM Menu_2_1_Status8 =
{
	"�ź���״̬",
	10,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

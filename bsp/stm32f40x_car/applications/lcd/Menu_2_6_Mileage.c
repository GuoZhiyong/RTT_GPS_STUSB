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
#include <stdlib.h>
#include "sed1520.h"
#include "jt808.h"
#include "jt808_gps.h"

/**/
static void msg( void *p )
{
}

/**/
static void show( void )
{
	char buf[32];
	pMenuItem->tick=rt_tick_get();

	lcd_fill( 0 );
	sprintf( buf, "20%02d/%02d/%02d %02d:%02d",
	         YEAR( mytime_now ),
	         MONTH( mytime_now ),
	         DAY( mytime_now ),
	         HOUR( mytime_now ),
	         MINUTE( mytime_now ));
	lcd_text12( 0, 4, (char*)buf, strlen( buf ), LCD_MODE_SET );
	
	sprintf( buf, "�����:%06d ����", jt808_param.id_0xF020 / 1000 );
	lcd_text12( 0, 16, (char*)buf, strlen( buf ), LCD_MODE_SET );
	lcd_update_all( );
}

/**/
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
			break;
		case KEY_DOWN:
			break;
	}
}

MENUITEM Menu_2_6_Mileage =
{
	"�����Ϣ�鿴",
	12,				  0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};
/************************************** The End Of File **************************************/

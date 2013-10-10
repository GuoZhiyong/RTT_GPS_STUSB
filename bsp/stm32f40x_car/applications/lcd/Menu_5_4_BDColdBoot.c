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
#include "jt808_gps.h"

u8 RertartGps_screen = 0;


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

	lcd_fill( 0 );
	lcd_text12( 24, 10, "��ȷ�ϼ�������", 12, LCD_MODE_SET );
	lcd_update_all( );

	RertartGps_screen = 0;
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

			RertartGps_screen = 0;
			break;
		case KEY_OK:
			if( RertartGps_screen == 0 )
			{
				RertartGps_screen = 1;
				lcd_fill( 0 );
				lcd_text12( 6, 10, "����ģ���������ɹ�", 18, LCD_MODE_INVERT );
				lcd_update_all( );

				//---- ȫ������ ------


				/*
				                             $CCSIR,1,1*48
				                             $CCSIR,2,1*48
				                             $CCSIR,3,1*4A
				 */
				gps_mode( gps_status.Position_Moule_Status );
			}
			break;
		case KEY_UP:
			break;
		case KEY_DOWN:
			break;
	}
}

MENUITEM Menu_5_4_bdColdBoot =
{
	"����ģ��������",
	14,				  0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

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
unsigned char	*car_status_str[4] = { "�ճ�", "����", "Ԥ��", "����" };

static uint8_t	pos			= 0;
static uint8_t	selected	= 0; /*�Ƿ���ѡ��*/

/**/
static void display( )
{
	unsigned char i = 0;

	lcd_fill( 0 );
	if( selected == 0 )
	{
		lcd_text12( 12, 3, "����״̬ѡ��", 12, LCD_MODE_SET );
		for( i = 0; i < 4; i++ )
		{
			lcd_text12( i * 30, 19, car_status_str[i], 4, LCD_MODE_SET );
		}
		lcd_text12(30 * pos, 19, car_status_str[pos], 4, LCD_MODE_INVERT );
	}else
	{
		lcd_text12( 12, 3, "����״̬: ", 10, LCD_MODE_SET );
		lcd_text12( 12+60, 3, car_status_str[pos], 4, LCD_MODE_INVERT );
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
	pos				= jt808_param.id_0xF021;
	selected		= 0;
	display( );
}

/**/
static void keypress( unsigned int key )
{
	uint32_t i;
	
	switch( key )
	{
		case KEY_MENU:
			pMenuItem = &Menu_3_InforInteract;
			pMenuItem->show( );
			break;
		case KEY_OK:
			if( selected ) /*�Ѿ�ѡ��*/
			{
				pMenuItem = &Menu_3_InforInteract;
				pMenuItem->show( );
				break;
			}
			selected = 1;
			jt808_param.id_0xF021=pos;
			i=jt808_status&0xFFFFFCFF;	/*bit 8.9 ����*/
			jt808_status=i|(uint32_t)(pos<<8);
			display( );
			break;
		case KEY_UP:
			if( pos == 0 )
			{
				pos = 4;
			}
			pos--;
			display( );
			break;
		case KEY_DOWN:
			pos++;
			pos %= 4;
			display( );
			break;
	}
}

MENUITEM Menu_3_2_FullorEmpty =
{
	"����״̬",
	8,				  0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

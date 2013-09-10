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

#define INPUT	0
#define SELECT	1
#define EDIT	2

static uint8_t mode = INPUT;

/*����λ�� 0 ���� 1-6 ��ĸ������*/
static uint8_t	input_pos;  /*��ǰ����ĸ�����������һ��*/
static int8_t	modify_pos; /*Ҫ�޸ĵ�λ��*/
static int8_t	pos;        /*��ѡ��λ��*/
/*��ǰ��ѡ��*/

/*���ƺ���*/
static char		chepai[9];

static char		* hz = { "���򼽽����ɼ��ڻ�����������³ԥ�������¹����崨���Ʋ��¸������¸۰�" };
static char		* asc = { "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" };

static uint8_t	timeout_invert	= 0;
static uint8_t	fInvert			= 0;

/**/
static uint8_t min( uint8_t a, uint8_t b )
{
	if( a > b )
	{
		return b;
	} else
	{
		return a;
	}
}

/**/
static void display( void )
{
	uint8_t page, index;
	lcd_fill( 0 );

	lcd_text12( 0, 4, "���ƺ�:", 7, LCD_MODE_SET );
	lcd_text12( 50, 4, chepai, strlen( chepai ), LCD_MODE_SET );

	//rt_kprintf( "\nfmodify=%d,modify_pos=%d,input_pos=%d", fModify, modify_pos, input_pos );
	if( mode==SELECT ) /*ѡ��״̬*/
	{
		if( fInvert )
		{
			if( modify_pos >= input_pos )
			{
				modify_pos = 0;
			}else if( modify_pos < 0 )
			{
				modify_pos = input_pos - 1;
			}

			if( modify_pos == 0 )
			{
				lcd_text12( 50, 4, chepai, 2, LCD_MODE_INVERT );
			}else
			{
				lcd_text12( 50 + modify_pos * 6 + 6, 4, chepai + modify_pos + 1, 1, LCD_MODE_INVERT );
			}
		}
	}else if (mode==INPUT)/*����״̬*/
	{
		if( input_pos == 0 ) /*��������*/
		{
			if( pos < 0 )
			{
				pos = ( strlen( hz ) >> 1 ) - 1;
			}else
			{
				pos %= ( strlen( hz ) >> 1 );
			}
			page	= pos / 10;
			index	= pos % 10;
			lcd_text12( 0, 20, hz + page * 20, min( 20, strlen( hz ) - page * 20 ), LCD_MODE_SET );
			lcd_text12( index * 12, 20, hz + page * 20 + index * 2, 2, LCD_MODE_INVERT );
		}else if( input_pos > 7 )
		{
			pMenuItem = &Menu_0_2_CarType;
			pMenuItem->show( );
		}else /*������ĸ����*/
		{
			if( pos < 0 )
			{
				pos = strlen( asc ) - 1;
			}else
			{
				pos %= strlen( asc );
			}
			page	= pos / 20;
			index	= pos % 20;
			lcd_text12( 0, 20, asc + page * 20, min( 20, strlen( asc ) - page * 20 ), LCD_MODE_SET );
			lcd_text12( index * 6, 20, asc + page * 20 + index, 1, LCD_MODE_INVERT );
		}
	}
	else		/*�༭�޸�*/
	{
		if( modify_pos == 0 ) /*��������*/
		{
			lcd_text12( 50, 4, chepai, 2, LCD_MODE_INVERT );
			if( pos < 0 )
			{
				pos = ( strlen( hz ) >> 1 ) - 1;
			}else
			{
				pos %= ( strlen( hz ) >> 1 );
			}
			page	= pos / 10;
			index	= pos % 10;
			lcd_text12( 0, 20, hz + page * 20, min( 20, strlen( hz ) - page * 20 ), LCD_MODE_SET );
			lcd_text12( index * 12, 20, hz + page * 20 + index * 2, 2, LCD_MODE_INVERT );
		}else /*������ĸ����*/
		{
			lcd_text12( 50 + modify_pos * 6 + 6, 4, chepai + modify_pos + 1, 1, LCD_MODE_INVERT );
			if( pos < 0 )
			{
				pos = strlen( asc ) - 1;
			}else
			{
				pos %= strlen( asc );
			}
			page	= pos / 20;
			index	= pos % 20;
			lcd_text12( 0, 20, asc + page * 20, min( 20, strlen( asc ) - page * 20 ), LCD_MODE_SET );
			lcd_text12( index * 6, 20, asc + page * 20 + index, 1, LCD_MODE_INVERT );
		}
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
	input_pos	= 0;
	pos			= 0;
	mode		= INPUT;
	memset( chepai, 0, 9 );
	display( );
}

/**/
static void keypress( unsigned int key )
{
	switch( key )
	{
		case KEY_MENU:
			if( mode == INPUT )
			{
				if( input_pos ) /*�Ѿ�������0 2 3 4  5 6 7*/
				{
					mode		= SELECT;
					modify_pos	= input_pos - 1;
				}
			}else
			{
				mode = INPUT;
			}
			display( );
			break;
		case KEY_OK:
			if( mode == SELECT )
			{
				mode = EDIT;
				pos=0;
			}else if( mode == INPUT )
			{
				if( input_pos == 0 )
				{
					memcpy( chepai, hz + pos * 2, 2 );
				}else
				{
					chepai[input_pos + 1] = *( asc + pos );
				}
				input_pos++;
				pos = 0;
			}else				/*�༭״̬*/
			{
				if( modify_pos == 0 )
				{
					memcpy( chepai, hz + pos * 2, 2 );
				}else
				{
					chepai[modify_pos + 1] = *( asc + pos );
				}
				mode = SELECT;
			}
			display( );
			break;
		case KEY_UP:
		case KEY_UP_REPEAT:
			if( mode == SELECT )
			{
				modify_pos--;
			}else
			{
				pos--;
			}
			display( );
			break;
		case KEY_DOWN:
		case KEY_DOWN_REPEAT:
			if( mode == SELECT )
			{
				modify_pos++;
			} else
			{
				pos++;
			}
			display( );
			break;
	}
}

/*��˸Ҫ�༭������*/
static void timetick( unsigned int systick )
{
	timeout_invert++;
	if( timeout_invert == 10 )
	{
		timeout_invert	= 0;
		fInvert			= ~fInvert;
		display( );
	}
}

MENUITEM Menu_0_1_license =
{
	"���ƺ�����",
	10,
	0,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

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

#include "camera.h"


/*
   Ҫ�ܲ鿴��¼?

   ͼƬ��¼
    ��ȡͼƬ��¼���� ����������
   �����ϴ�
   ��ǰ����
 */
static int16_t pos;

#define SCR_PHOTO_MENU				0
#define SCR_PHOTO_SELECT_ITEM		1
#define SCR_PHOTO_SELECT_DETAILED	2
#define SCR_PHOTO_TAKE				3

static uint8_t	scr_mode = SCR_PHOTO_MENU; /*��ǰ��ʾ�Ľ���״̬*/

static uint8_t	*pHead		= RT_NULL;
static uint16_t pic_count	= 0;

/**/
static void display( void )
{
	uint16_t			pic_page_start; /*ÿ�ĸ�ͼƬ��¼Ϊһ��page*/
	uint8_t				buf[32], buf_time[16];
	uint16_t			i;
	MYTIME				t;
	TypeDF_PackageHead	* pcurrhead;

	lcd_fill( 0 );
	switch( scr_mode )
	{
		case SCR_PHOTO_MENU:
			pos &= 0x01;
			lcd_text12( 5, 4, "1.ͼƬ��¼", 10, 3 - pos * 2 );
			lcd_text12( 5, 18, "2.�����ϴ�", 10, pos * 2 + 1 );
			break;
		case SCR_PHOTO_SELECT_ITEM:
			if( pic_count ) /*��ͼƬ*/
			{
				if( pos >= pic_count )
				{
					pos = 0;
				}
				if( pos < 0 )
				{
					pos = pic_count - 1;
				}
				pic_page_start = pos & 0xFFFC; /*ÿ4��1��*/
				for( i = pic_page_start; i < pic_page_start + 4; i++ )
				{
					if( i >= pic_count )
					{
						break;
					}

					pcurrhead	= (TypeDF_PackageHead*)( pHead + i * sizeof( TypeDF_PackageHead ) );
					t			= pcurrhead->Time;

					sprintf( buf, "%02d>%02d-%02d-%02d %02d:%02d:%02d",
					         i+1, YEAR( t ), MONTH( t ), DAY( t ), HOUR( t ), MINUTE( t ), SEC( t ));
					if( i == pos )
					{
						lcd_asc0608( 0, 8 * ( i & 0x03 ), buf, LCD_MODE_INVERT );
					} else
					{
						lcd_asc0608( 0, 8 * ( i & 0x03 ), buf, LCD_MODE_SET );
					}
				}
			}else /*û��ͼƬ*/
			{
				lcd_text12( 25, 12, "û��ͼƬ��¼", 12, LCD_MODE_SET );
			}
			break;
		case SCR_PHOTO_SELECT_DETAILED:/*��ʾͼƬ��ϸ��Ϣ*/
			pcurrhead	= (TypeDF_PackageHead*)( pHead + pos * sizeof( TypeDF_PackageHead ) );
			t			= pcurrhead->Time;
			
			sprintf( buf, "%02d-%02d-%02d %02d:%02d:%02d",
					 YEAR( t ), MONTH( t ), DAY( t ), HOUR( t ), MINUTE( t ), SEC( t ));
			lcd_asc0608( 0, 0, buf, LCD_MODE_SET );

			sprintf(buf,"chn=%d trig=%d del=%d",pcurrhead->Channel_ID,pcurrhead->TiggerStyle,pcurrhead->State);
			lcd_asc0608( 0, 8, buf, LCD_MODE_SET );
			sprintf(buf,"size=%d",pcurrhead->Len-64);
			lcd_asc0608( 0, 16, buf, LCD_MODE_SET );
			lcd_asc0608(70,24,"usb",LCD_MODE_SET);
			lcd_asc0608(104,24,"rep",LCD_MODE_SET);
		
			break;
		case SCR_PHOTO_TAKE:
			break;
	}
	lcd_update_all( );
}

/*�������ռ��ϴ��Ĺ���*/
static void msg( void *p )
{
	pMenuItem->tick = rt_tick_get( );
}

/**/
static void show( void )
{
	pMenuItem->tick = rt_tick_get( );
	pos				= 0;
	scr_mode		= SCR_PHOTO_MENU;
	display( );
}

/*�����������ջ��ϴ�����������ж�?*/
static void keypress( unsigned int key )
{
	switch( key )
	{
		case KEY_MENU:

			if( scr_mode == SCR_PHOTO_MENU )
			{
				pMenuItem = &Menu_3_InforInteract;
				pMenuItem->show( );
				break;
			}
			if( scr_mode == SCR_PHOTO_SELECT_ITEM ) /*���˵����˵�*/
			{
				scr_mode = SCR_PHOTO_MENU;
				rt_free( pHead );
				pHead = RT_NULL;
			}else if( scr_mode == SCR_PHOTO_SELECT_DETAILED) /*���˵�������Ƭ��Ŀ��ʾ*/
			{
				scr_mode = SCR_PHOTO_SELECT_ITEM;
			}
			display();
			break;
		case KEY_OK:
			if( scr_mode == SCR_PHOTO_MENU )
			{
				if( pos == 0 ) /*ͼƬ��¼*/
				{
					pHead		= Cam_Flash_SearchPicHead( 0x00000000, 0xFFFFFFFF, 0, 0xFF, &pic_count );
					scr_mode	= SCR_PHOTO_SELECT_ITEM;
					
				}else /*ͼƬ����*/
				{
				}
			}else if( scr_mode == SCR_PHOTO_SELECT_ITEM )
			{
				scr_mode = SCR_PHOTO_SELECT_DETAILED;
			}
			else if(scr_mode == SCR_PHOTO_SELECT_DETAILED)
			{

			}
			display( );
			break;
		case KEY_UP:
			pos--;
			display( );
			break;
		case KEY_DOWN:
			pos++;
			display( );
			break;
	}
}

MENUITEM Menu_3_4_Multimedia =
{
	"��ý����Ϣ",
	10,				  0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

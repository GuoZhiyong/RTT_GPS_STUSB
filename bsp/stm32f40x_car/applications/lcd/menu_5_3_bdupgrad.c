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
#include "sed1520.h"
#include "hmi.h"
#include "gps.h"


/*
   �Ƿ�������״̬�����ð����ж�
   0;��ʼ �ȴ�����
   1:���������۳ɹ����
   2:���������У�����Ӧ����

 */

#define BD_IDLE 0                           /*����*/
#define BD_BUSY 1                           /*����������*/
#define BD_COMPLETE 2                           /*�������,���۳ɹ�ʧ��*/


static uint8_t		fupgrading	= BD_IDLE;
static rt_thread_t	tid_upgrade = RT_NULL;  /*��ʼ����*/

#define OPER_MENU_SELECT	0xFF            /*Ϊ�˸�pos���Ӧ*/
#define OPER_CHECK_VER		0               /*���汾*/
#define OPER_UPGRADE		1               /*����*/
#define OPER_SWITCH_MODE	2               /*����ģʽ�л�*/
#define OPER_RESTART		3               /*��������*/

static uint8_t oper_mode = OPER_MENU_SELECT;

struct _stu_menu
{
	uint8_t left;
	uint8_t top;
	char	* caption;
};
static struct _stu_menu m[4] =
{
	{ 12, 4,  "�汾��ѯ" },
	{ 72, 4,  " U������" },
	{ 12, 18, "ģʽ�л�" },
	{ 72, 18, "ģ������" },
};
static uint8_t			pos;

char					*bd_mode[3] = { "������", "��GPS", "˫ģ" };
char					*bd_restart[2] = { "������", "������" };

/*��ʾ�˵�*/
static void menu_disp( void )
{
	uint8_t i;
	lcd_fill( 0 );
	switch( oper_mode )
	{
		case OPER_MENU_SELECT:
			for( i = 0; i < 4; i++ )
			{
				lcd_text12( m[i].left, m[i].top, m[i].caption, 8, LCD_MODE_SET );
			}
			lcd_text12( m[pos].left, m[pos].top, m[pos].caption, 8, LCD_MODE_INVERT );
			break;
		case OPER_SWITCH_MODE:

			lcd_text12( 0, 4, "ѡ�񱱶�����ģʽ", 16, LCD_MODE_SET );
			for( i = 0; i < 3; i++ )
			{
				lcd_text12( i * 40, 18, bd_mode[i], strlen( bd_mode[i] ), LCD_MODE_SET );
			}

			lcd_text12( pos * 40, 18, bd_mode[pos], strlen( bd_mode[pos] ), LCD_MODE_INVERT );
			break;
		case OPER_RESTART:
			lcd_text12( 0, 4, "��������ģ��", 12, LCD_MODE_SET );
			for( i = 0; i < 2; i++ )
			{
				lcd_text12( 16 + i * 48, 18, bd_restart[i], 6, LCD_MODE_SET );
			}
			lcd_text12( pos * 40, 18, bd_restart[pos], strlen( bd_restart[pos] ), LCD_MODE_INVERT );

			break;
	}
	lcd_update_all( );
}

/**/
static void show( void )
{
	fupgrading	= BD_IDLE;
	oper_mode=OPER_MENU_SELECT;
	pos			= 0;
	menu_disp( );
}

/*
   �ṩ�ص�����������ʾ��Ϣ
 */

static void msg( void *p )
{
	//char buf[32];
	unsigned int	len;
	char			*pinfo;
	if( fupgrading )
	{
		lcd_fill( 0 );

		pinfo	= (char*)p;
		len		= strlen( pinfo );
		lcd_text12( 35, 10, pinfo + 1, len - 1, LCD_MODE_SET );
		if( pinfo[0] == 'E' )                   /*��������*/
		{
			fupgrading=BD_COMPLETE;
			tid_upgrade=RT_NULL;
		}
		lcd_update_all( );
	}else
	{
		menu_disp( );
	}
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
	if(fupgrading==BD_BUSY)
	{
		return;
	}
	switch( key )
	{
		case KEY_MENU:
			if( fupgrading == BD_IDLE )
			{
				pMenuItem = &Menu_5_other;
				pMenuItem->show( );
			}else
			{
			fupgrading=BD_IDLE;
			oper_mode = OPER_MENU_SELECT;
			pos=0;
				menu_disp( );
			}

			break;
		case KEY_OK:

			if(fupgrading==BD_COMPLETE)
			{
				fupgrading=BD_IDLE;
				oper_mode = OPER_MENU_SELECT;
				pos=0;
				menu_disp();
				break;
			}
			if( oper_mode == OPER_MENU_SELECT )
			{
				switch( pos )
				{
					case  0:
					case  1:
						if( pos & 0x01 )
						{
							tid_upgrade = rt_thread_create( "upgrade", thread_gps_upgrade_udisk, (void*)msg, 1024, 5, 5 );
						}else
						{
							tid_upgrade = rt_thread_create( "bd_ver", thread_gps_check_ver, (void*)msg, 1024, 5, 5 );
						}
						if( tid_upgrade != RT_NULL )
						{
							fupgrading = BD_BUSY;
							rt_thread_startup( tid_upgrade );
						}else
						{
							msg( "E����ִ��ʧ��" );
						}
						break;
					case 2:
						pos			= gps_status.Position_Moule_Status - 1;
						oper_mode	= OPER_SWITCH_MODE;
						menu_disp( );
						break;
					case 3:
						pos = 0;
						menu_disp( );
						oper_mode = OPER_RESTART;
						break;
				}
			}
			else if( oper_mode == OPER_SWITCH_MODE )
			{
				gps_mode(pos+1);
				oper_mode=OPER_MENU_SELECT;
				pos=0;
				pop_msg("ģʽ�л����",RT_TICK_PER_SECOND*3);
			}
			else if( oper_mode == OPER_RESTART)
			{
				
				oper_mode=OPER_MENU_SELECT;
				pos=0;
				pop_msg("ģ���������",RT_TICK_PER_SECOND*3);
			}
			break;
		case KEY_UP:
			if( oper_mode == OPER_MENU_SELECT )
			{
				if( pos == 0 )
				{
					pos = 4;
				}
			}else if( oper_mode == OPER_SWITCH_MODE )
			{
				if( pos == 0 )
				{
					pos = 3;
				}
			}
			else if( oper_mode == OPER_RESTART)
			{
				if( pos == 0 )
				{
					pos = 2;
				}
			}
			pos--;
			menu_disp( );
			break;
		case KEY_DOWN:
			pos++;
			if( oper_mode == OPER_MENU_SELECT )
			{
				pos%=4;
			}
			if( oper_mode == OPER_SWITCH_MODE)
			{
				pos%=3;
			}
			if( oper_mode == OPER_RESTART)
			{
				pos%=2;
			}
			menu_disp( );
			break;
	}
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
static void timetick( unsigned int tick )
{
}

MENUITEM Menu_5_3_bdupgrade =
{
	"����ģ�����",
	12,			   0,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

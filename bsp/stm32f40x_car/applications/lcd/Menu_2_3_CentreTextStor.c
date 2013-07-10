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

uint8_t * ptextmsg;             /*���ݽ�������Ϣ������TEXTMSG_HEAD*/

/*�鿴ģʽ*/
#define VIEW_ITEM	0
#define VIEW_DETAIL 0xFF

uint8_t view_mode = VIEW_ITEM;
uint16_t text_pos=0; /*����ģʽ��,��ʾ�Ŀ�ʼλ��*/
uint16_t text_len=0; /*��ʾ���ݵĳ���*/
/**/
static void display_item( void )
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
static void display_detail( void )
{
}

/**/
static void msg( void *p )
{
}

/*��ʾ�����·���Ϣ*/
static void show( void )
{
	uint8_t *pinfo;
	uint8_t res;
	if( jt808_textmsg_get( 0, ptextmsg ) )
	{
		display_item( );
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
	u8	CurrentDisplen	= 0;
	u8	i				= 0;

	switch( key )
	{
		case KEY_MENU:
			if( ptextmsg != RT_NULL )
			{
				rt_free( ptextmsg );
			}
			pMenuItem = &Menu_2_InforCheck;
			pMenuItem->show( );
			break;
		case KEY_OK: /*�鿴ģʽ�л�*/
			view_mode ^= 0xFF;
			if( view_mode == VIEW_ITEM )
			{
				display_item( );
			}else
			{
				display_detail( );
			}
			lcd_update_all( );
			break;
		case KEY_UP:
			if( view_mode == VIEW_ITEM )
			{
				jt808_textmsg_get( 1, ptextmsg );
			}else
			{
				//if(text_pos>
			}
			lcd_update_all( );

			break;
		case KEY_DOWN:
			lcd_update_all( );
			break;
	}
}

/**/
static void timetick( unsigned int systick )
{
	CounterBack++;
	if( CounterBack != MaxBankIdleTime )
	{
		return;
	} else
	{
		pMenuItem = &Menu_1_Idle;
		pMenuItem->show( );
	}
}

MENUITEM Menu_2_3_CentreTextStor =
{
	"�ı���Ϣ�鿴",
	12,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

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
#include <stdio.h>
#include <string.h>
#include "sed1520.h"

static uint8_t	count, pos;

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void display( void )
{
	char buf[32];
	EVENT* evt;
	uint8_t index=pos&0xFE;	/*���뵽ż��ҳ*/
	lcd_fill( 0 );
	if( count == 0 )
	{
		//lcd_text12( ( 122 - 6 * 12 ) / 2, 18, "[���¼���Ϣ]", 6*12,LCD_MODE_SET );
		strcpy(buf,"[���¼���Ϣ]");
		lcd_text12( ( 122 - 6 * 12 ) / 2, 18, buf, strlen(buf),LCD_MODE_SET );
	}
	else
	{
		evt=(EVENT*)(event_buf+64*index);
		sprintf(buf,"%02d %s",evt->id,evt->body);
		lcd_text12(0,4,buf,strlen(buf),3-(pos&0x01)*2);  /*SET=1 INVERT=3*/
		if((index+1)<count)
		{
			evt=(EVENT*)(event_buf+64*index+64);
			sprintf(buf,"%02d %s",evt->id,evt->body);
			lcd_text12(0,18,buf,strlen(buf),(pos&0x01)*2+1);  /*SET=1 INVERT=3*/
		}	
	}
	lcd_update_all( );
}

/**/
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
	count			= jt808_event_get();
	rt_kprintf("count=%d\n",count);
	pos				= 0;
	display( );
}

/**/
static void keypress( unsigned int key )
{
	uint8_t buf[32];
	switch( key )
	{
		case KEY_MENU:
			if( event_buf!= RT_NULL )
			{
				rt_free( event_buf );
				event_buf=RT_NULL;
			}
			pMenuItem = &Menu_3_InforInteract;
			pMenuItem->show( );
			break;
		case KEY_OK:  /*�¼�����*/
			buf[0]=((EVENT*)(event_buf+pos*64))->id;
			jt808_tx(0x0301,buf,1);
			break;
		case KEY_UP:
			if( pos )
			{
				pos--;
			}
			display( );
			break;
		case KEY_DOWN:
			if( pos < (count-1) )
			{
				pos++;
			}
			display( );
			break;
	}
}

/*����Ƿ�ص�������*/
void timetick( unsigned int tick )
{
	if( ( tick - pMenuItem->tick ) >= 100 * 10 )
	{
		if( event_buf != RT_NULL )
		{
			rt_free(event_buf );
			event_buf=RT_NULL;
		}
		pMenuItem = &Menu_1_Idle;
		pMenuItem->show( );
	}
}

MENUITEM Menu_3_7_Affair =
{
	"�¼���Ϣ",
	8,		   0,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

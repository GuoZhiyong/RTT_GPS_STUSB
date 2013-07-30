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

#include "gps.h"

static uint8_t menu_pos=0;
static uint8_t fupgrading=0;	/*�Ƿ�������״̬�����ð����ж�*/
static rt_thread_t tid_upgrade=RT_NULL;	/*��ʼ����*/



/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void menu_set(void)
{
	lcd_fill( 0 );
	lcd_text12( 0, 3, "����", 4, LCD_MODE_SET );
	lcd_text12( 0, 17, "����", 4, LCD_MODE_SET );
	if( menu_pos == 0 )
	{
		lcd_text12( 35, 3, "1.��������", 10, LCD_MODE_INVERT );
		lcd_text12( 35, 19, "2.USB����", 9, LCD_MODE_SET );
	}else
	{
		lcd_text12( 35, 3, "1.��������", 10, LCD_MODE_SET );
		lcd_text12( 35, 19, "2.USB����", 9, LCD_MODE_INVERT );
	}
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
	pMenuItem->tick=rt_tick_get();

	menu_pos=0;
	menu_set();
}

/*
�ṩ�ص�����������ʾ��Ϣ
*/
#if 0
static void msg( unsigned int res )
{
	char buf[32];
	unsigned int len;
	lcd_fill( 0 );
	lcd_text12( 0, 3, "����", 4, LCD_MODE_SET );
	lcd_text12( 0, 17, "����", 4, LCD_MODE_SET );
	switch(res)
	{
		case BDUPG_RES_UART_OK:
			lcd_text12( 35, 10,"���ڸ������",12, LCD_MODE_SET );
			fupgrading=0;
			tid_upgrade=RT_NULL;
			break;
		case BDUPG_RES_USB_OK:
			lcd_text12( 35, 10,"USB�������",11, LCD_MODE_SET );
			fupgrading=0;
			tid_upgrade=RT_NULL;
			break;			
		case BDUPG_RES_THREAD:
			lcd_text12( 35, 10,"��������ʧ��",12, LCD_MODE_SET );
			fupgrading=0;
			tid_upgrade=RT_NULL;
			break;
		case BDUPG_RES_TIMEOUT:
			lcd_text12( 35, 10,"��ʱ����",8, LCD_MODE_SET );
			fupgrading=0;
			tid_upgrade=RT_NULL;
			break;
		case BDUPG_RES_RAM:
			lcd_text12( 35, 10,"����RAMʧ��",11, LCD_MODE_SET );
			fupgrading=0;
			tid_upgrade=RT_NULL;
			break;
		case BDUPG_RES_UART_READY:
			lcd_text12( 35, 10,"���ڿ�ʼ����",12, LCD_MODE_SET );
			break;
		case BDUPG_RES_USB_READY:
			lcd_text12( 35, 10,"����u��",7, LCD_MODE_SET );
			break;	
		case BDUPG_RES_USB_NOEXIST:
			lcd_text12( 35, 10,"U�̲�����",10, LCD_MODE_SET );
			fupgrading=0;
			tid_upgrade=RT_NULL;
			break;
		case BDUPG_RES_USB_NOFILE:
			lcd_text12( 35, 10,"�ļ�������",10, LCD_MODE_SET );
			fupgrading=0;
			tid_upgrade=RT_NULL;
			break;
		case BDUPG_RES_USB_FILE_ERR:
			lcd_text12( 35, 10,"�ļ���ʽ����",12, LCD_MODE_SET );
			fupgrading=0;
			tid_upgrade=RT_NULL;
			break;
		case BDUPG_RES_USB_MODULE_H:
			break;
		case BDUPG_RES_USB_MODULE_L:
			break;
		case BDUPG_RES_USB_FILE_VER:
			break;
		default:
			if(res<0xffff)	/*Ĭ�ϲ��ᷢ����ô�������*/
			{
				len=sprintf(buf,"���͵�%d��",res);
				lcd_text12( 35, 10, buf, len, LCD_MODE_SET );
			}	
			
			break;
			
	}
	lcd_update_all( );
}
#endif

static void msg( void *p)
{
	//char buf[32];
	unsigned int len;
	char *pinfo;
	lcd_fill( 0 );
	lcd_text12( 0, 3, "����", 4, LCD_MODE_SET );
	lcd_text12( 0, 17, "����", 4, LCD_MODE_SET );
	pinfo=(char *)p;
	len=strlen(pinfo);
	lcd_text12( 35, 10,pinfo+1,len-1, LCD_MODE_SET );
	if(*pinfo=='E')	/*��������*/
	{
		fupgrading=0;
		tid_upgrade=RT_NULL;
	}

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
static void keypress( unsigned int key )
{
	if(fupgrading)	return;
	switch( key )
	{
		case KEY_MENU:
			pMenuItem=&Menu_5_other;
			pMenuItem->show();
			CounterBack=0;
			
			if(tid_upgrade) /*����������*/
			{
				rt_thread_delete(tid_upgrade);
			}
			
			break;
		case KEY_OK:
			if(BD_upgrad_contr==1)
				menu_set();
			if(menu_pos==0)
			{
				tid_upgrade = rt_thread_create( "upgrade", thread_gps_upgrade_uart, (void*)msg, 1024, 5, 5 );
				if( tid_upgrade != RT_NULL )
				{
					msg("I�ȴ���������");
					rt_thread_startup( tid_upgrade );
				}else
				{
					msg("E�̴߳���ʧ��");
				}
			}else /*U������*/
			{
				tid_upgrade = rt_thread_create( "upgrade", thread_gps_upgrade_udisk, (void*)msg, 1024, 5, 5 );
				if( tid_upgrade != RT_NULL )
				{
					msg("I�ȴ�U������");
					rt_thread_startup( tid_upgrade );
				}else
				{
					msg("E�̴߳���ʧ��");
				}
			}
			break;
		case KEY_UP:
			menu_pos=menu_pos^0x01;
			menu_set();
			break;
		case KEY_DOWN:
			menu_pos=menu_pos^0x01;
			menu_set();
			break;
	}
}

static void timetick( unsigned int tick )
{


}


MENUITEM Menu_5_3_bdupgrade =
{
	"��������",
	8,0,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

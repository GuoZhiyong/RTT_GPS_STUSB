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
#define  DIS_Dur_width_check 11

unsigned char			noselect_check[] = { 0x3C, 0x7E, 0xC3, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C };  //����
unsigned char			select_check[] = { 0x3C, 0x7E, 0xFF, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C };    //ʵ��

static unsigned char	menu_pos = 0;

DECL_BMP( 8, 8, select_check ); DECL_BMP( 8, 8, noselect_check );

static PMENUITEM psubmenu[8] =
{
	&Menu_2_1_Status8,          //�ź���
	&Menu_2_2_Speed15,          //15speed
	&Menu_2_3_CentreTextStor,   //�ı���Ϣ(��Ϣ1-��Ϣ8)
	&Menu_2_4_CarInfor,         //������Ϣ
	&Menu_2_5_DriverInfor,      //��ʻԱ��Ϣ
	&Menu_2_6_Mileage,          //�����Ϣ
	&Menu_2_7_RequestProgram,   //������Ϣ�㲥
	&Menu_2_8_DnsIpDisplay
};


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void menuswitch( void )
{
	unsigned char i;
	lcd_fill( 0 );
	lcd_text12( 0, 3, "��Ϣ", 4, LCD_MODE_SET );
	lcd_text12( 0, 17, "�鿴", 4, LCD_MODE_SET );
	for( i = 0; i < 8; i++ )
	{
		lcd_bitmap( 30 + i * DIS_Dur_width_check, 5, &BMP_noselect_check, LCD_MODE_SET );
	}
	lcd_bitmap( 30 + menu_pos * DIS_Dur_width_check, 5, &BMP_select_check, LCD_MODE_SET );
	lcd_text12( 30, 19, (char*)( psubmenu[menu_pos]->caption ), psubmenu[menu_pos]->len, LCD_MODE_SET );
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
	pMenuItem->tick=rt_tick_get();
	menu_pos = 0;
	menuswitch( );
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
			CounterBack = 0;
			pMenuItem	= &Menu_3_InforInteract; //scr_CarMulTrans;
			pMenuItem->show( );
			break;
		case KEY_OK:
			pMenuItem = psubmenu[menu_pos];
			pMenuItem->show( );
			break;
		case KEY_UP:
			if( menu_pos == 0 )
			{
				menu_pos = 7;
			} else
			{
				menu_pos--;
			}
			menuswitch( );
			break;
		case KEY_DOWN:
			menu_pos++;
			if( menu_pos > 7 )
			{
				menu_pos = 0;
			}
			menuswitch( );
			break;
	}
}


MENUITEM Menu_2_InforCheck =
{
	"�鿴��Ϣ",
	8,0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

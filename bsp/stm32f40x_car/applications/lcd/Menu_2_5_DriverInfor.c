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


/*
   static unsigned char Jiayuan_screen=0;  //  ��ʾ�鿴/���ͽ���   =1ʱѡ���ǲ鿴���Ƿ��ͽ���
   static unsigned char CheckJiayuanFlag=0;//  1:������ʾ��ʻԱ��Ϣ   2:���뷢�ͼ�ʻԱ��Ϣ
   static unsigned char Jiayuan_1_2=0;     // 0:��ʾ�ڲ鿴����   1:��ʾ�ڷ��ͽ���
 */
typedef struct _DIS_DIRVER_INFOR
{
	unsigned char	DIS_SELECT_check_send;
	unsigned char	DIS_ENTER_check_send;
	unsigned char	DIS_SHOW_check_send;
}DIS_DIRVER_INFOR;

DIS_DIRVER_INFOR DIS_DRIVER_inform_temp;

//��ʻԱ����
void Display_jiayuan( unsigned char NameCode )
{
//unsigned char i=0;
	lcd_fill( 0 );
	if( NameCode == 1 )
	{
		lcd_text12( 30, 3, "��ʻԱ����", 10, LCD_MODE_SET );
		lcd_text12( 42, 19, jt808_param.id_0xF008, strlen( jt808_param.id_0xF008 ), LCD_MODE_SET );
		//lcd_text12(48,19,(char *)Driver_Info.DriveName,strlen(Driver_Info.DriveName),LCD_MODE_SET);
	}else
	{
		lcd_text12( 30, 3, "��ʻ֤����", 10, LCD_MODE_SET );
		lcd_text12( 0, 19, jt808_param.id_0xF009, strlen( jt808_param.id_0xF009 ), LCD_MODE_SET );
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
static void Dis_DriverInfor( unsigned char type, unsigned char disscreen )
{
	lcd_fill( 0 );
	if( type == 1 )
	{
		if( disscreen == 1 )
		{
			lcd_text12( 0, 3, "1.��ʻԱ��Ϣ�鿴", 16, LCD_MODE_INVERT );
			lcd_text12( 0, 19, "2.��ʻԱ��Ϣ����", 16, LCD_MODE_SET );
		}else if( disscreen == 2 )
		{
			lcd_text12( 0, 3, "1.��ʻԱ��Ϣ�鿴", 16, LCD_MODE_SET );
			lcd_text12( 0, 19, "2.��ʻԱ��Ϣ����", 16, LCD_MODE_INVERT );
		}
	}else if( type == 2 )
	{
		if( disscreen == 1 )
		{
			lcd_text12( 0, 10, "��ȷ�Ϸ��ͼ�ʻԱ��Ϣ", 20, LCD_MODE_SET );
		} else if( disscreen == 2 )
		{
			lcd_text12( 5, 10, "��ʻԱ��Ϣ���ͳɹ�", 18, LCD_MODE_SET );
		}
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

	Dis_DriverInfor( 1, 1 );
	DIS_DRIVER_inform_temp.DIS_SELECT_check_send	= 1;
	DIS_DRIVER_inform_temp.DIS_ENTER_check_send		= 1;
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
			pMenuItem = &Menu_2_InforCheck;
			pMenuItem->show( );

			memset( &DIS_DRIVER_inform_temp, 0, sizeof( DIS_DRIVER_inform_temp ) );
			break;
		case KEY_OK:
			if( DIS_DRIVER_inform_temp.DIS_ENTER_check_send == 1 )
			{
				DIS_DRIVER_inform_temp.DIS_ENTER_check_send		= 2;
				DIS_DRIVER_inform_temp.DIS_SELECT_check_send	= 0;            //�鿴���߷����Ѿ�ѡ��

				if( DIS_DRIVER_inform_temp.DIS_SHOW_check_send == 0 )           //����鿴��ʻԱ��Ϣ����
				{
					Display_jiayuan( 1 );
				} else if( DIS_DRIVER_inform_temp.DIS_SHOW_check_send == 1 )    //���뷢�ͼ�ʻԱ��Ϣ����
				{
					Dis_DriverInfor( 2, 1 );
				}
			}else if( DIS_DRIVER_inform_temp.DIS_ENTER_check_send == 2 )
			{
				DIS_DRIVER_inform_temp.DIS_ENTER_check_send = 3;
				if( DIS_DRIVER_inform_temp.DIS_SHOW_check_send == 0 )           //���ز鿴�ͷ��ͽ���
				{
					Dis_DriverInfor( 1, 1 );
					DIS_DRIVER_inform_temp.DIS_SELECT_check_send	= 1;
					DIS_DRIVER_inform_temp.DIS_ENTER_check_send		= 1;
				}else if( DIS_DRIVER_inform_temp.DIS_SHOW_check_send == 1 )     //��ʾ���ͳɹ�
				{
					Dis_DriverInfor( 2, 2 );
					//SD_ACKflag.f_DriverInfoSD_0702H=1;
					DIS_DRIVER_inform_temp.DIS_ENTER_check_send		= 0;        //    1
					DIS_DRIVER_inform_temp.DIS_SELECT_check_send	= 0;
					DIS_DRIVER_inform_temp.DIS_SHOW_check_send		= 0;
				}
			}
			break;
		case KEY_UP:
			if( DIS_DRIVER_inform_temp.DIS_ENTER_check_send == 2 )
			{
				if( DIS_DRIVER_inform_temp.DIS_SHOW_check_send == 0 )           //�鿴
				{
					Display_jiayuan( 1 );
				} else if( DIS_DRIVER_inform_temp.DIS_SHOW_check_send == 1 )    //����
				{
					Dis_DriverInfor( 2, 1 );
				}
			}else if( DIS_DRIVER_inform_temp.DIS_SELECT_check_send == 1 )       //ѡ�����鿴���߷���
			{
				DIS_DRIVER_inform_temp.DIS_SHOW_check_send = 0;
				Dis_DriverInfor( 1, 1 );
			}
			break;
		case KEY_DOWN:
			if( DIS_DRIVER_inform_temp.DIS_ENTER_check_send == 2 )
			{
				if( DIS_DRIVER_inform_temp.DIS_SHOW_check_send == 0 )           //�鿴
				{
					Display_jiayuan( 2 );
				} else if( DIS_DRIVER_inform_temp.DIS_SHOW_check_send == 1 )    //����
				{
					Dis_DriverInfor( 2, 1 );
				}
			}else if( DIS_DRIVER_inform_temp.DIS_SELECT_check_send == 1 )       //ѡ�����鿴���߷���
			{
				DIS_DRIVER_inform_temp.DIS_SHOW_check_send = 1;
				Dis_DriverInfor( 1, 2 );
			}
			break;
	}
}

MENUITEM Menu_2_5_DriverInfor =
{
	"��ʻԱ��Ϣ�鿴",
	14,				  0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

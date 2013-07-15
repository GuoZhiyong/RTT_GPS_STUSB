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

static unsigned char	vech_num[16] = { "���ƺ�:        " };
static unsigned char	vech_type[25] = { "��������:            " };
static unsigned char	vech_ID[19] = { "����ID:            " };
static unsigned char	vech_VIN[20] = { "VIN                 " };

static unsigned char	updown_flag = 0;

//��ʻԱ����
void Display_driver( u8 drivercar )
{
	u8 color_disp[4];
	switch( drivercar )
	{
		case 1:

			//���ƺ�JT808Conf_struct.Vechicle_Info.Vech_Num
			memcpy( vech_num + 7, jt808_param.id_0xF006, 8 );
			
			//������ɫ
			memset( color_disp, 0, sizeof( color_disp ) );
			switch( jt808_param.id_0xF007 )
			{
				case 1: memcpy( color_disp, "��ɫ", 4 ); break;
				case 2: memcpy( color_disp, "��ɫ", 4 ); break;
				case 3: memcpy( color_disp, "��ɫ", 4 ); break;
				case 4: memcpy( color_disp, "��ɫ", 4 ); break;
				case 9: memcpy( color_disp, "����", 4 ); break;
				default: memcpy( color_disp, "��ɫ", 4 ); break;
			}
			lcd_fill( 0 );
			lcd_text12( 10, 3, (char*)vech_num, 15, LCD_MODE_SET );
			lcd_text12( 10, 19, "������ɫ:", 9, LCD_MODE_SET );
			lcd_text12( 64, 19, color_disp, 4, LCD_MODE_SET );
			lcd_update_all( );
			break;

		case 2: //����ID Vechicle_Info.DevicePhone
			lcd_fill( 0 );
			memcpy( vech_type + 9, jt808_param.id_0xF008, 6 );
			lcd_text12( 0, 3, (char*)vech_type, 19, LCD_MODE_SET );
			//��ȡ�豸�ٶ�ȡ����GPS�ٶȻ����ٶ����ٶ�
			#if NEED_TODO
			if( JT808Conf_struct.DF_K_adjustState )
			{
				lcd_text12( 0, 18, "�豸�ٶ�:�������ٶ�", 19, LCD_MODE_SET );
			} else
			{
				lcd_text12( 0, 18, "�豸�ٶ�:GPS�ٶ�", 16, LCD_MODE_SET );
			}
			#endif
			lcd_update_all( );
			break;
		case 3:                                                  //  ����ID
			lcd_fill( 0 );
			memcpy( vech_ID + 7, jt808_param.id_0xF005 + 3, 12 );
			lcd_text12( 0, 3, (char*)vech_ID, 19, LCD_MODE_SET );
			memcpy( vech_VIN + 3, jt808_param.id_0xF005, 17 );    //����VIN
			lcd_text12( 0, 19, (char*)vech_VIN, 20, LCD_MODE_SET );
			lcd_update_all( );
			break;
		default:
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
	lcd_fill( 0 );
	lcd_text12( 24, 3, "������Ϣ�鿴", 12, LCD_MODE_SET );
	lcd_text12( 24, 19, "�鿴�밴ѡ��", 12, LCD_MODE_SET );
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
	switch( key )
	{
		case KEY_MENU:
			pMenuItem = &Menu_2_InforCheck;
			pMenuItem->show( );
			break;
		case KEY_OK:
			updown_flag = 1;
			Display_driver( 1 );
			break;
		case KEY_UP:
			if( updown_flag == 1 )
			{
				Display_driver( 1 );
			}
			if( updown_flag == 2 )
			{
				Display_driver( 2 ); updown_flag = 1;
			}
			break;
		case KEY_DOWN:
			if( updown_flag == 1 )
			{
				Display_driver( 2 ); updown_flag = 2;
			}else
			if( updown_flag == 2 )
			{
				Display_driver( 3 ); updown_flag = 2;
			}
			break;
	}

}



MENUITEM Menu_2_4_CarInfor =
{
	"������Ϣ�鿴",
	12,0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

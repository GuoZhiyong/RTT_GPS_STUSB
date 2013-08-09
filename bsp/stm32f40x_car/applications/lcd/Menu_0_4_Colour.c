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

#if 1

static char				* color[] = { "������", "��ɫ", "��ɫ", "��ɫ", "��ɫ", "����" };

static uint8_t			pos;


/**/
static void display( void )
{
	lcd_fill( 0 );

	lcd_text12( 16, 12, "������ɫ", 8, LCD_MODE_SET );
	lcd_bitmap( 70, 14, &BMP_arrow_left, LCD_MODE_SET );
	lcd_text12( 80, 12, color[pos], strlen(color[pos]), LCD_MODE_INVERT );
	lcd_bitmap( 82+strlen(color[pos])*6, 14, &BMP_arrow_right, LCD_MODE_SET );

	lcd_update_all( );
}

/**/
static void msg( void *p )
{
}

/**/
static void show( void )
{
	pMenuItem->tick=rt_tick_get();
	pos=0;
	display( );
}

/**/
static void keypress( unsigned int key )
{
	char	color_index[] = { 0, 1, 2, 3, 4, 9 };
	switch( key )
	{
		case KEY_MENU:
			pMenuItem = &Menu_1_Idle;
			pMenuItem->show( );
			break;
		case KEY_OK:
			jt808_param.id_0x0084=color_index[pos];
			if(pos==0)		/*δ����,����VIN*/
			{
				pMenuItem = &Menu_0_3_vin;
			}
			else
			{
				pMenuItem = &Menu_0_1_license;
			}	
			pMenuItem->show( );
			break;
		case KEY_UP:
			if( pos == 0 )
			{
				pos = 6;
			}
			pos--;
			display( );
			break;
		case KEY_DOWN:
			pos++;
			pos %= 6;
			display( );
			break;
	}
}


MENUITEM Menu_0_4_Colour =
{
	"������ɫ����",
	12,			   0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};

#else
u8				comfirmation_flag	= 0;
u8				col_screen			= 0;
u8				CarBrandCol_Cou		= 1;

unsigned char	car_col[13] = { "������ɫ:��ɫ" };

/**/
void car_col_fun( u8 par )
{
	//������ɫ�����
	if( par == 1 )
	{
		memcpy( Menu_VecLogoColor, "��ɫ", 4 ); //   1
	}else if( par == 2 )
	{
		memcpy( Menu_VecLogoColor, "��ɫ", 4 ); //   2
	}else if( par == 3 )
	{
		memcpy( Menu_VecLogoColor, "��ɫ", 4 ); //   3
	}else if( par == 4 )
	{
		memcpy( Menu_VecLogoColor, "��ɫ", 4 ); //   4
	}else if( par == 5 )
	{
		memcpy( Menu_VecLogoColor, "����", 4 );
		par = 9;
	}                                           //   9

	Menu_color_num = par;

	memcpy( car_col + 9, Menu_VecLogoColor, 4 );
	lcd_fill( 0 );
	lcd_text12( 20, 10, (char*)car_col, 13, LCD_MODE_SET );
	lcd_update_all( );
}

/**/
static void msg( void *p )
{
}

/**/
static void show( void )
{
	CounterBack = 0;
	col_screen	= 1;
	car_col_fun( 1 );
}

/**/
static void keypress( unsigned int key )
{
	switch( key )
	{
		case KEY_MENU:
			if( comfirmation_flag == 4 )
			{
				pMenuItem = &Menu_1_Idle;
				pMenuItem->show( );
			}else
			{
				pMenuItem = &Menu_0_loggingin;
				pMenuItem->show( );
			}
			col_screen			= 0;
			CarBrandCol_Cou		= 1;
			comfirmation_flag	= 0;
			break;
		case KEY_OK:
			if( col_screen == 1 )
			{
				col_screen			= 2;
				CarSet_0_counter	= 1;    //
				menu_color_flag		= 1;    //������ɫ�������
				lcd_fill( 0 );
				lcd_text12( 20, 3, (char*)car_col, 13, LCD_MODE_SET );
				lcd_text12( 12, 18, "��ȷ�ϼ��鿴��Ϣ", 16, LCD_MODE_SET );
				lcd_update_all( );
			}else if( col_screen == 2 )
			{
				menu_color_flag = 0;

				col_screen			= 3;
				comfirmation_flag	= 1; //����������Ϣ��־
				lcd_fill( 0 );
				lcd_text12( 0, 0, Menu_Car_license, 8, LCD_MODE_SET );
				lcd_text12( 54, 0, Menu_VechileType, 6, LCD_MODE_SET );
				lcd_text12( 96, 0, (char*)Menu_VecLogoColor, 4, LCD_MODE_SET );

				lcd_text12( 0, 11, "VIN", 3, LCD_MODE_SET );
				lcd_text12( 19, 11, (char*)Menu_Vin_Code, 17, LCD_MODE_SET );
				lcd_text12( 24, 22, "ȷ��", 4, LCD_MODE_INVERT );
				lcd_text12( 72, 22, "ȡ��", 4, LCD_MODE_SET );
				lcd_update_all( );
			}else if( comfirmation_flag == 1 )
			{
				col_screen			= 0;
				comfirmation_flag	= 4;
				//�������õ���Ϣ
				lcd_fill( 0 );
				lcd_text12( 18, 3, "������������Ϣ", 14, LCD_MODE_SET );
				lcd_text12( 0, 18, "���˵��������������", 20, LCD_MODE_SET );
				lcd_update_all( );

				//���ƺ�
				memset( jt808_param.id_0xF006, 0, 32 );
				memcpy( jt808_param.id_0xF006, Menu_Car_license, strlen( Menu_Car_license ) );
				//��������
				memset( jt808_param.id_0xF008, 0, 32 );
				memcpy( jt808_param.id_0xF008, Menu_Car_license, strlen( Menu_Car_license ) );
				//����VIN
				memset( jt808_param.id_0xF005, 0, 32 );
				memcpy( jt808_param.id_0xF005, Menu_Vin_Code, 17 );

				// ������ɫ
				jt808_param.id_0xF007 = Menu_color_num;

				//  �洢
				param_save( );
			}else if( comfirmation_flag == 2 )
			{
				col_screen			= 0;
				comfirmation_flag	= 3;
				lcd_fill( 0 );
				lcd_text12( 6, 3, "��ȷ���Ƿ���������", 18, LCD_MODE_SET );
				lcd_text12( 12, 18, "��ȷ�ϼ���������", 16, LCD_MODE_SET );
				lcd_update_all( );
			}else if( comfirmation_flag == 3 )
			{
				col_screen			= 0;
				comfirmation_flag	= 0;
				//��������
				pMenuItem = &Menu_0_loggingin;
				pMenuItem->show( );

				comfirmation_flag	= 0;
				col_screen			= 0;
				CarBrandCol_Cou		= 1;
			}

			break;
		case KEY_UP:
			if( col_screen == 1 )
			{
				CarBrandCol_Cou--;
				if( CarBrandCol_Cou < 1 )
				{
					CarBrandCol_Cou = 5;
				}
				car_col_fun( CarBrandCol_Cou );
			}else if( col_screen == 3 )
			{
				comfirmation_flag = 1;
				lcd_fill( 0 );
				lcd_text12( 0, 0, Menu_Car_license, 8, LCD_MODE_SET );
				lcd_text12( 54, 0, Menu_VechileType, 6, LCD_MODE_SET );
				lcd_text12( 96, 0, (char*)Menu_VecLogoColor, 4, LCD_MODE_SET );

				lcd_text12( 0, 11, "VIN", 3, LCD_MODE_SET );
				lcd_text12( 19, 11, (char*)Menu_Vin_Code, 17, LCD_MODE_SET );
				lcd_text12( 24, 22, "ȷ��", 4, LCD_MODE_INVERT );
				lcd_text12( 72, 22, "ȡ��", 4, LCD_MODE_SET );
				lcd_update_all( );
			}

			break;
		case KEY_DOWN:
			if( col_screen == 1 )
			{
				CarBrandCol_Cou++;
				if( CarBrandCol_Cou > 5 )
				{
					CarBrandCol_Cou = 1;
				}
				car_col_fun( CarBrandCol_Cou );
			}else if( col_screen == 3 )
			{
				comfirmation_flag = 2;
				lcd_fill( 0 );
				lcd_text12( 0, 0, Menu_Car_license, 8, LCD_MODE_SET );
				lcd_text12( 54, 0, Menu_VechileType, 6, LCD_MODE_SET );
				lcd_text12( 96, 0, (char*)Menu_VecLogoColor, 4, LCD_MODE_SET );

				lcd_text12( 0, 11, "VIN", 3, LCD_MODE_SET );
				lcd_text12( 19, 11, (char*)Menu_Vin_Code, 17, LCD_MODE_SET );
				lcd_text12( 24, 22, "ȷ��", 4, LCD_MODE_SET );
				lcd_text12( 72, 22, "ȡ��", 4, LCD_MODE_INVERT );
				lcd_update_all( );
			}

			break;
	}
}

/**/
static void timetick( unsigned int systick )
{
/*
   CounterBack++;
   if(CounterBack!=MaxBankIdleTime*5)
   return;
   CounterBack=0;
   pMenuItem=&Menu_0_loggingin;
   pMenuItem->show();


   col_screen=0;
   CarBrandCol_Cou=1;
   comfirmation_flag=0;*/
}

MENUITEM Menu_0_4_Colour =
{
	"������ɫ����",
	12,			   0,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};
#endif
/************************************** The End Of File **************************************/

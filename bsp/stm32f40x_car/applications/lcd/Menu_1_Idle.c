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
#include  <stdlib.h>
#include  <stdio.h>
#include  <string.h>

#include "menu_include.h"
#include "sed1520.h"

static uint32_t			lasttick;
unsigned char			dispstat		= 0;
unsigned int			reset_firstset	= 0;

unsigned char			gsm_g[] = {
	0x1c,                                                                       /*[   ***  ]*/
	0x22,                                                                       /*[  *   * ]*/
	0x40,                                                                       /*[ *      ]*/
	0x40,                                                                       /*[ *      ]*/
	0x4e,                                                                       /*[ *  *** ]*/
	0x42,                                                                       /*[ *    * ]*/
	0x22,                                                                       /*[  *   * ]*/
	0x1e,                                                                       /*[   **** ]*/
};

unsigned char			gsm_0[] = {
	0x00,                                                                       /*[        ]*/
	0x00,                                                                       /*[        ]*/
	0x00,                                                                       /*[        ]*/
	0x00,                                                                       /*[        ]*/
	0x00,                                                                       /*[        ]*/
	0x00,                                                                       /*[        ]*/
	0x80,                                                                       /*[*       ]*/
	0x80,                                                                       /*[*       ]*/
};

unsigned char			gsm_1[] = {
	0x00,                                                                       /*[        ]*/
	0x00,                                                                       /*[        ]*/
	0x00,                                                                       /*[        ]*/
	0x00,                                                                       /*[        ]*/
	0x20,                                                                       /*[  *     ]*/
	0x20,                                                                       /*[  *     ]*/
	0xa0,                                                                       /*[* *     ]*/
	0xa0,                                                                       /*[* *     ]*/
};

unsigned char			gsm_2[] = {
	0x00,                                                                       /*[        ]*/
	0x00,                                                                       /*[        ]*/
	0x08,                                                                       /*[    *   ]*/
	0x08,                                                                       /*[    *   ]*/
	0x28,                                                                       /*[  * *   ]*/
	0x28,                                                                       /*[  * *   ]*/
	0xa8,                                                                       /*[* * *   ]*/
	0xa8,                                                                       /*[* * *   ]*/
};

unsigned char			gsm_3[] = {
	0x02,                                                                       /*[      * ]*/
	0x02,                                                                       /*[      * ]*/
	0x0a,                                                                       /*[    * * ]*/
	0x0a,                                                                       /*[    * * ]*/
	0x2a,                                                                       /*[  * * * ]*/
	0x2a,                                                                       /*[  * * * ]*/
	0xaa,                                                                       /*[* * * * ]*/
	0xaa,                                                                       /*[* * * * ]*/
};

unsigned char			link_on[] = {
	0x08,                                                                       /*[    *   ]*/
	0x04,                                                                       /*[     *  ]*/
	0xfe,                                                                       /*[******* ]*/
	0x00,                                                                       /*[        ]*/
	0xfe,                                                                       /*[******* ]*/
	0x40,                                                                       /*[ *      ]*/
	0x20,                                                                       /*[  *     ]*/
	0x00,                                                                       /*[        ]*/
};

unsigned char			link_off[] = {
	0x10,                                                                       /*[   *    ]*/
	0x08,                                                                       /*[    *   ]*/
	0xc6,                                                                       /*[**   ** ]*/
	0x00,                                                                       /*[        ]*/
	0xe6,                                                                       /*[***  ** ]*/
	0x10,                                                                       /*[   *    ]*/
	0x08,                                                                       /*[    *   ]*/
	0x00,                                                                       /*[        ]*/
};
static unsigned char	Battery[] = { 0x00, 0xFC, 0xFF, 0xFF, 0xFC, 0x00 };     //8*6
static unsigned char	NOBattery[] = { 0x04, 0x0C, 0x98, 0xB0, 0xE0, 0xF8 };   //6*6
static unsigned char	TriangleS[] = { 0x30, 0x78, 0xFC, 0xFC, 0x78, 0x30 };   //6*6
static unsigned char	TriangleK[] = { 0x30, 0x48, 0x84, 0x84, 0x48, 0x30 };   //6*6

static unsigned char	empty[] = { 0x84, 0x84, 0x84, 0x84, 0x84, 0xFC };       /*�ճ�*/
static unsigned char	full_0[] = { 0x84, 0x84, 0x84, 0xFC, 0xFC, 0xFC };      /*����*/
static unsigned char	full_1[] = { 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC };      /*�س�*/

//��� �Ƿ�У������ϵ���ı�־
DECL_BMP( 8, 6, Battery );  DECL_BMP( 6, 6, NOBattery );
DECL_BMP( 6, 6, TriangleS ); DECL_BMP( 6, 6, TriangleK );
//�ź�ǿ�ȱ�־
DECL_BMP( 7, 8, gsm_g );    DECL_BMP( 7, 8, gsm_0 );
DECL_BMP( 7, 8, gsm_1 );    DECL_BMP( 7, 8, gsm_2 );
DECL_BMP( 7, 8, gsm_3 );
//���ӻ������߱�־
DECL_BMP( 7, 8, link_on );  DECL_BMP( 7, 8, link_off );
//�ճ� ���� �س�
DECL_BMP( 6, 6, empty );    DECL_BMP( 6, 6, full_0 );  DECL_BMP( 6, 6, full_1 );


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void GPSGPRS_Status( void )
{
	char *mode[] = { "   ", "BD ", "GPS", "G/B" };
	lcd_text12( 19, 0, mode[gps_status.Position_Moule_Status], 3, LCD_MODE_SET );

	if( jt808_status & BIT_STATUS_GPS ) /*gps״̬��λ���*/
	{
		lcd_bitmap( 37, 2, &BMP_link_on, LCD_MODE_SET );
	}else
	{
		lcd_bitmap( 37, 2, &BMP_link_off, LCD_MODE_SET );
	}

	lcd_text12( 48, 0, "GPRS", 4, LCD_MODE_SET );

	if( connect_state.server_state == CONNECTED ) /*gprs����״̬*/
	{
		lcd_bitmap( 72, 2, &BMP_link_on, LCD_MODE_SET );
	}else
	{
		lcd_bitmap( 72, 2, &BMP_link_off, LCD_MODE_SET );
	}

#if 0
	if( JT808Conf_struct.LOAD_STATE == 1 ) //�������ر�־
	{
		lcd_bitmap( 95, 2, &BMP_empty, LCD_MODE_SET );
	}else if( JT808Conf_struct.LOAD_STATE == 2 )
	{
		lcd_bitmap( 95, 2, &BMP_full_0, LCD_MODE_SET );
	}else if( JT808Conf_struct.LOAD_STATE == 3 )
	{
		lcd_bitmap( 95, 2, &BMP_full_1, LCD_MODE_SET );
	}

	if( ModuleStatus & 0x04 ) //��Դ��־
	{
		lcd_bitmap( 105, 2, &BMP_Battery, LCD_MODE_SET );
	}else
	{
		lcd_bitmap( 105, 2, &BMP_NOBattery, LCD_MODE_SET );
	}

	if( DF_K_adjustState ) //�Ƿ�У������ϵ��
	{
		lcd_bitmap( 115, 2, &BMP_TriangleS, LCD_MODE_SET );
	}else
	{
		lcd_bitmap( 115, 2, &BMP_TriangleK, LCD_MODE_SET );
	}
#endif
}

/*
������ʾgpsʱ�䣬��gpsδ��λ��ʱ��ͣ��
��ѡ�� 
rtcʱ�䣬��ҪƵ����ȡ(1��1��)������ж�Уʱ
ʹ���Լ���mytime����θ���(ʹ��ϵͳ��1s��ʱ���Ƿ�׼ȷ)
*/
void  Disp_Idle( void )
{
	char	buf_datetime[22];
	char	buf_speed[20];

	sprintf( buf_datetime, "20%02d/%02d/%02d %02d:%02d:%02d",
	         YEAR(mytime_now),
	         MONTH(mytime_now),
	         DAY(mytime_now),
	         HOUR(mytime_now),
	         MINUTE(mytime_now),
	         SEC(mytime_now) );
	sprintf( buf_speed, "%3dkm/h   %3d�� ", gps_speed, gps_cog );

	lcd_fill( 0 );
	lcd_text12( 0, 10, (char*)buf_datetime, strlen( buf_datetime ), LCD_MODE_SET );
	lcd_text12( 0, 20, (char*)buf_speed, strlen( buf_speed ), LCD_MODE_SET );
	lcd_bitmap( 0, 3, &BMP_gsm_g, LCD_MODE_SET );
	lcd_bitmap( 8, 3, &BMP_gsm_3, LCD_MODE_SET );
	GPSGPRS_Status( );
	lcd_update_all( );
}

/**/
static void msg( void *p )
{
}

/**/
static void show( void )
{
	Disp_Idle( );
	reset_firstset	= 0;
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
			SetVIN_NUM	= 1;
			OK_Counter	= 0;

			CounterBack = 0;
			UpAndDown	= 1; 

			pMenuItem = &Menu_2_InforCheck;
			pMenuItem->show( );
			reset_firstset = 0;
			break;
		case KEY_OK:
			if( reset_firstset == 0 )
			{
				reset_firstset = 1;
			} else if( reset_firstset == 3 )
			{
				reset_firstset = 4;
			} else if( reset_firstset == 4 )
			{
				reset_firstset = 5;
			}
			break;
		case KEY_UP:
			if( reset_firstset == 1 )
			{
				reset_firstset = 2;
			} else if( reset_firstset == 2 )
			{
				reset_firstset = 3;
			} else if( reset_firstset == 5 )
			{
				reset_firstset = 6;
			}
			break;
		case KEY_DOWN:
			reset_firstset = 0;
			//��ӡ����
			GPIO_SetBits( GPIOB, GPIO_Pin_7 );
			if( print_rec_flag == 0 )
			{
				print_rec_flag = 1; //��ӡ��־
			}
			break;
		case KEY_MENU_REPEAT:		/*������������*/
			pMenuItem = &Menu_0_0_password;
			pMenuItem->show( );
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
static void timetick( unsigned int systick )
{
#if NEED_TODO

	if( reset_firstset == 6 )
	{
		reset_firstset++;
		//----------------------------------------------------------------------------------
		JT808Conf_struct.password_flag = 0; // clear  first flag
		Api_Config_Recwrite_Large( jt808, 0, (u8*)&JT808Conf_struct, sizeof( JT808Conf_struct ) );
		//----------------------------------------------------------------------------------
	}else if( reset_firstset >= 7 )         //50msһ��,,60s
	{
		reset_firstset++;
		lcd_fill( 0 );
		lcd_text12( 0, 3, "���������ó��ƺź�ID", 20, LCD_MODE_SET );
		lcd_text12( 24, 18, "���¼ӵ�鿴", 12, LCD_MODE_SET );
		lcd_update_all( );
	}else
	{
		//����Դ����
		if( Warn_Status[1] & 0x01 )
		{
			BuzzerFlag = 11;
			lcd_fill( 0 );
			lcd_text12( 30, 10, "����Դ����", 10, LCD_MODE_SET );
			lcd_update_all( );
		}
		//ѭ����ʾ��������
		tickcount++;
		if( tickcount >= 16 )
		{
			tickcount = 0;
			Disp_Idle( );
		}
	}

	Cent_To_Disp( );
#endif

	if( systick - lasttick >= RT_TICK_PER_SECOND )
	{
		Disp_Idle( );
	}
}

MENUITEM Menu_1_Idle =
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

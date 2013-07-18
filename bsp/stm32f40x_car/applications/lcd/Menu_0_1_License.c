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

/*����λ�� 0 ���� 1-6 ��ĸ������*/
static uint8_t	input_pos;
static int8_t	pos;
/*��ǰ��ѡ��*/

/*���ƺ���*/
static char	chepai[9];

static char		* hz = { "���򼽽����ɼ��ڻ�����������³ԥ�������¹����崨���Ʋ��¸������¸۰�" };
static char		* asc = { "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" };


/**/
static uint8_t min( uint8_t a, uint8_t b )
{
	if( a > b )
	{
		return b;
	} else
	{
		return a;
	}
}

/**/
void display( void )
{
	uint8_t page, index;
	lcd_fill( 0 );

	lcd_text12( 0, 4, "���ƺ�:", 7, LCD_MODE_SET );
	lcd_text12( 50, 4, chepai, strlen( chepai ), LCD_MODE_SET );
	if( input_pos == 0 ) /*��������*/
	{
		if( pos < 0 )
		{
			pos = (strlen( hz ) >> 1)-1;
		}else
		{
			pos %= ( strlen( hz ) >> 1 );
		}
		page	= pos / 10;
		index	= pos % 10;
		lcd_text12( 0, 20, hz + page * 20, min( 20, strlen( hz ) - page * 20 ), LCD_MODE_SET );
		lcd_text12( index * 12, 20, hz + page * 20 + index * 2, 2, LCD_MODE_INVERT );
	}else if( input_pos > 8 )
	{
		pMenuItem = &Menu_0_2_CarType;
		pMenuItem->show( );
		
	}else
	{
		if( pos < 0 )
		{
			pos = strlen( asc )-1;
		}else
		{
			pos %= strlen( asc );
		}
		page	= pos / 20;
		index	= pos % 20;
		lcd_text12( 0, 20, asc + page * 20, min( 20, strlen( asc ) - page * 20 ), LCD_MODE_SET );
		lcd_text12( index * 6, 20, asc + page * 20 + index, 1, LCD_MODE_INVERT );
	}
	lcd_update_all( );
}

/**/
static void msg( void *p )
{
}

/**/
static void show( void )
{
	input_pos	= 0;
	pos			= 0;
	memset( chepai, 0, 9 );
	display( );
}

/**/
static void keypress( unsigned int key )
{
	switch( key )
	{
		case KEY_MENU:
			pMenuItem = &Menu_0_loggingin;
			pMenuItem->show( );

			break;
		case KEY_OK:
			if( input_pos == 0 )
			{
				memcpy( chepai, hz+pos*2, 2 );
				input_pos += 2;
			}else
			{
				//memcpy( &chepai[input_pos], asc + pos, 1 );
				chepai[input_pos]=*(asc+pos);
				input_pos++;
			}
			pos=0;
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

/**/
static void timetick( unsigned int systick )
{
}

MENUITEM Menu_0_1_license =
{
	"���ƺ�",
	6,
	0,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

#else

#define width_hz	12
#define width_zf	6
#define top_line	14

unsigned char	screen_flag		= 0; //==1��ʼ���뺺�ֽ���
unsigned char	screen_in_sel	= 1, screen_in_sel2 = 1;
unsigned char	zifu_counter	= 0;

//���򼽻���ԥ���ɺ���  ��³������Ӷ���ʽ�  ���¼���������ش���  ��
unsigned char Car_HZ_code[31][2] = { "��", "��", "��", "��", "��", "ԥ", "��", "��", "��", "��", \
	                                 "��", "³", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��", "��" };
//


/*unsigned char Car_num_code[36]={'0','1','2','3','4','5','6','7','8','9',\
   'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R',\
   'S','T','U','V','W','X','Y','Z'};*/
unsigned char	Car_num_code[36][1] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", \
	                                    "A",   "B", "C", "D", "E", "F", "G", "H", "I", "J", "K","L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z" };

unsigned char	select[] = { 0x0C, 0x06, 0xFF, 0x06, 0x0C };
unsigned char	select_kong[] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
DECL_BMP( 8, 5, select );
DECL_BMP( 8, 5, select_kong );


/*
   ���򼽽����ɼ��ڻ�����������³ԥ�������¹����崨���Ʋ��¸������¸۰�
 */
void license_input( unsigned char par )
{
	lcd_fill( 0 );
	switch( par )
	{
		case 1:
			lcd_text12( 0, 20, "���򼽽�����ԥ���ɺ�", 20, LCD_MODE_SET );
			break;
		case 2:
			lcd_text12( 0, 20, "��³������Ӷ������", 20, LCD_MODE_SET );
			break;
		case 3:
			lcd_text12( 0, 20, "���¼���������ش���", 20, LCD_MODE_SET );
			break;
		case 4:
			lcd_text12( 0, 20, "��", 2, LCD_MODE_SET );
			break;
		case 5:
			lcd_text12( 0, 20, "0123456789ABCDEFGHIJ", 20, LCD_MODE_SET );
			break;
		case 6:
			lcd_text12( 0, 20, "KLMNOPQRSTUVWXYZ", 16, LCD_MODE_SET );
			break;
		default:
			break;
	}
	lcd_update_all( );
}

/*
   ���򼽻���ԥ����
   ������³�������
   ����ʽ����¼���������ش�����

   offset:  type==2ʱ����    ==1ʱ����
 */
void license_input_az09( unsigned char type, unsigned char offset, unsigned char par )
{
	if( ( type == 1 ) && ( par >= 1 ) && ( par <= 31 ) )
	{
		memcpy( Menu_Car_license, (char*)Car_HZ_code[par - 1], 2 );
		lcd_text12( 0, 0, (char*)Menu_Car_license, 2, LCD_MODE_SET );
	}else if( ( type == 2 ) && ( par >= 1 ) && ( par <= 36 ) )
	{
		memcpy( Menu_Car_license + offset - 1, (char*)Car_num_code[par - 1], 1 );
		lcd_text12( 0, 0, (char*)Menu_Car_license, offset, LCD_MODE_SET ); //Car_license+2+(offset-3)=Car_license+offset-1
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
	memset( Menu_Car_license, 0, sizeof( Menu_Car_license ) );
	CounterBack = 0;
	license_input( 1 );
	lcd_bitmap( ( screen_in_sel - 1 ) * 12, top_line, &BMP_select, LCD_MODE_SET );
	lcd_update_all( );

	screen_flag		= 1;
	screen_in_sel	= 1;
	screen_in_sel2	= 1;
	zifu_counter	= 0;
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
			pMenuItem = &Menu_0_loggingin;
			pMenuItem->show( );

			screen_in_sel	= 1;
			screen_in_sel2	= 1;
			screen_flag		= 0;
			zifu_counter	= 0;

			break;
		case KEY_OK:
			if( screen_flag == 1 )
			{
				if( zifu_counter == 0 )
				{
					zifu_counter = 2;                       //1������=2���ַ�
				}
				screen_flag = 5;                            //��һ������ѡ�ã�ѡ�ַ�
				lcd_fill( 0 );
				//lcd_text12(0,0,(char *)Car_license,zifu_counter,LCD_MODE_SET);

				license_input( 5 );                         //��ĸѡ��
				license_input_az09( 1, 0, screen_in_sel );  //д��ѡ���ĺ���
				lcd_bitmap( 0, top_line, &BMP_select, LCD_MODE_SET );
				lcd_update_all( );

				screen_in_sel2 = 1;
			}else if( ( screen_flag == 5 ) && ( zifu_counter < 8 ) )
			{
				zifu_counter++;

				lcd_fill( 0 );
				license_input( 5 );                                     //��ĸѡ��
				license_input_az09( 2, zifu_counter, screen_in_sel2 );  //
				//license_input_az09(1,0,screen_in_sel);//д��ѡ���ĺ���
				screen_in_sel2 = 1;
				lcd_bitmap( 0, top_line, &BMP_select, LCD_MODE_SET );
				lcd_text12( 0, 0, (char*)Menu_Car_license, zifu_counter, LCD_MODE_SET );
				lcd_update_all( );
				screen_in_sel2 = 1;
			}else if( ( screen_flag == 5 ) && ( zifu_counter == 8 ) )
			{
				screen_flag = 10;                           //���ƺ��������

				zifu_counter = 0;
				lcd_fill( 0 );
				lcd_text12( 0, 0, (char*)Menu_Car_license, 8, LCD_MODE_SET );
				//lcd_text12(15,1,(char *)Car_license+2,sizeof(Car_license)-2,LCD_MODE_SET);
				lcd_text12( 18, 20, "���ƺ��������", 14, LCD_MODE_INVERT );
				license_input_az09( 1, 0, screen_in_sel );  //д��ѡ���ĺ���
				lcd_update_all( );

				screen_in_sel2 = 1;
			}else if( screen_flag == 10 )
			{
				CarSet_0_counter	= 2;                    //   ���õ�2��
				pMenuItem			= &Menu_0_loggingin;
				pMenuItem->show( );

				screen_in_sel	= 1;
				screen_in_sel2	= 1;
				screen_flag		= 0;
				zifu_counter	= 0;
			}
			break;
		case KEY_UP:
			if( screen_flag == 1 ) //����ѡ��
			{
				if( screen_in_sel >= 2 )
				{
					screen_in_sel--;
				} else if( screen_in_sel == 1 )
				{
					screen_in_sel = 31;
				}
				if( screen_in_sel <= 10 )
				{
					license_input( 1 );
					lcd_bitmap( ( screen_in_sel - 1 ) * width_hz, top_line, &BMP_select, LCD_MODE_SET );
				}else if( ( screen_in_sel > 10 ) && ( screen_in_sel <= 20 ) )
				{
					license_input( 2 );
					lcd_bitmap( ( screen_in_sel - 11 ) * width_hz, top_line, &BMP_select, LCD_MODE_SET );
				}else if( ( screen_in_sel > 20 ) && ( screen_in_sel <= 30 ) )
				{
					license_input( 3 );
					lcd_bitmap( ( screen_in_sel - 21 ) * width_hz, top_line, &BMP_select, LCD_MODE_SET );
				}          else if( screen_in_sel == 31 )
				{
					license_input( 4 );
					lcd_bitmap( ( screen_in_sel - 31 ) * width_hz, top_line, &BMP_select, LCD_MODE_SET );
				}
				lcd_update_all( );
			}else if( screen_flag == 5 ) //��ĸ/���� ѡ��
			{
				if( screen_in_sel2 >= 2 )
				{
					screen_in_sel2--;
				} else if( screen_in_sel2 == 1 )
				{
					screen_in_sel2 = 36;
				}
				if( screen_in_sel2 <= 20 )
				{
					license_input( 5 );
					lcd_bitmap( ( screen_in_sel2 - 1 ) * width_zf, top_line, &BMP_select, LCD_MODE_SET );
				}else if( ( screen_in_sel2 > 20 ) && ( screen_in_sel2 <= 36 ) )
				{
					license_input( 6 );
					lcd_bitmap( ( screen_in_sel2 - 21 ) * width_zf, top_line, &BMP_select, LCD_MODE_SET );
				}
				//license_input_az09(1,0,screen_in_sel);//д��ѡ������ĸ������
				lcd_text12( 0, 0, (char*)Menu_Car_license, zifu_counter, LCD_MODE_SET );
				lcd_update_all( );
			}
			break;
		case KEY_DOWN:
			if( screen_flag == 1 ) //����ѡ��
			{
				if( screen_in_sel < 31 )
				{
					screen_in_sel++;
				} else if( screen_in_sel == 31 )
				{
					screen_in_sel = 1;
				}
				if( screen_in_sel <= 10 )
				{
					license_input( 1 );
					lcd_bitmap( ( screen_in_sel - 1 ) * width_hz, top_line, &BMP_select, LCD_MODE_SET );
				}else if( ( screen_in_sel > 10 ) && ( screen_in_sel <= 20 ) )
				{
					license_input( 2 );
					lcd_bitmap( ( screen_in_sel - 11 ) * width_hz, top_line, &BMP_select, LCD_MODE_SET );
				}else if( ( screen_in_sel > 20 ) && ( screen_in_sel <= 30 ) )
				{
					license_input( 3 );
					lcd_bitmap( ( screen_in_sel - 21 ) * width_hz, top_line, &BMP_select, LCD_MODE_SET );
				}          else if( screen_in_sel == 31 )
				{
					license_input( 4 );
					lcd_bitmap( ( screen_in_sel - 31 ) * width_hz, top_line, &BMP_select, LCD_MODE_SET );
				}
				lcd_update_all( );
			}else if( screen_flag == 5 ) //��ĸ/���� ѡ��
			{
				if( screen_in_sel2 < 36 )
				{
					screen_in_sel2++;
				} else if( screen_in_sel2 == 36 )
				{
					screen_in_sel2 = 1;
				}
				if( screen_in_sel2 <= 20 )
				{
					license_input( 5 );
					lcd_bitmap( ( screen_in_sel2 - 1 ) * width_zf, top_line, &BMP_select, LCD_MODE_SET );
				}else if( ( screen_in_sel2 > 20 ) && ( screen_in_sel2 <= 36 ) )
				{
					license_input( 6 );
					lcd_bitmap( ( screen_in_sel2 - 21 ) * width_zf, top_line, &BMP_select, LCD_MODE_SET );
				}
				//license_input_az09(1,0,screen_in_sel);//д��ѡ�������֡���ĸ
				lcd_text12( 0, 0, (char*)Menu_Car_license, zifu_counter, LCD_MODE_SET );
				lcd_update_all( );
			}

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
}

MENUITEM Menu_0_1_license =
{
	"���ƺ�",
	6,
	0,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

#endif

/************************************** The End Of File **************************************/

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
#include "scr.h"




static SCR_ITEM scr_item[] =
{
	{ "1.�ź�",	6, &scr_1_signal},
	{ "2.��Ա",	6, &scr_2_driver},
	{ "3.λ��",	6, &scr_3_geoinfo},
	{ "4.��Ϣ", 6, &scr_4_sms},
	{ "5.�绰", 6, &scr_5_phonecall},
	{ "6.�߼�",	6, &scr_6_advance},
	{ "7.���",	6, &scr_7_mileage},
	{ "8.��¼",	6, &scr_8_record},
	{ "9.ƣ��",	6, &scr_9_tired},
	{ "A.����",	6, &scr_a_vehicle},
	{ "B.����",	6, &scr_b_network},
	{ "C.ý��",	6, &scr_c_multimedia},
};


static uint8_t	selectpos = 0;  /*ѡ����λ��*/
static uint32_t tick=0;

/*��ʾ�˵�*/
static void display( void )
{
	lcd_fill( 0 );
	if(selectpos<6)
	{
		lcd_bitmap(0,4,&BMP_res_arrow_dn,LCD_MODE_SET);
		lcd_text12( 2, 8, scr_item[0].text, scr_item[0].len, (selectpos==0)?LCD_MODE_INVERT:LCD_MODE_SET );
		lcd_text12( 43, 8, scr_item[1].text, scr_item[1].len,(selectpos==1)?LCD_MODE_INVERT:LCD_MODE_SET );
		lcd_text12( 84, 8, scr_item[2].text, scr_item[2].len, (selectpos==2)?LCD_MODE_INVERT:LCD_MODE_SET );
		lcd_text12( 2, 20, scr_item[3].text, scr_item[3].len, (selectpos==3)?LCD_MODE_INVERT:LCD_MODE_SET );
		lcd_text12( 43, 20, scr_item[4].text, scr_item[4].len, (selectpos==4)?LCD_MODE_INVERT:LCD_MODE_SET );
		lcd_text12( 84, 20, scr_item[5].text, scr_item[5].len, (selectpos==5)?LCD_MODE_INVERT:LCD_MODE_SET );
		
	}
	else
	{
		lcd_bitmap(0,0,&BMP_res_arrow_up,LCD_MODE_SET);
		lcd_text12( 2, 8, scr_item[6].text, scr_item[6].len, (selectpos==6)?LCD_MODE_INVERT:LCD_MODE_SET );
		lcd_text12( 43, 8, scr_item[7].text, scr_item[7].len,(selectpos==7)?LCD_MODE_INVERT:LCD_MODE_SET );
		lcd_text12( 84, 8, scr_item[8].text, scr_item[8].len, (selectpos==8)?LCD_MODE_INVERT:LCD_MODE_SET );
		lcd_text12( 2, 20, scr_item[9].text, scr_item[9].len, (selectpos==9)?LCD_MODE_INVERT:LCD_MODE_SET );
		lcd_text12( 43, 20, scr_item[10].text, scr_item[10].len, (selectpos==10)?LCD_MODE_INVERT:LCD_MODE_SET );
		lcd_text12( 84, 20, scr_item[11].text, scr_item[11].len, (selectpos==11)?LCD_MODE_INVERT:LCD_MODE_SET );
	}

	lcd_update_all( );
	
}

/***/
static void show( void* parent )
{
	scr_main.parent=(PSCR)parent;
	display( );
}

/*��������*/
static void keypress(unsigned int key )
{
	switch( key )
	{
		case KEY_MENU_PRESS:
			if((void*)0==scr_main.parent) break;
			pscr=scr_main.parent;
			pscr->show(&scr_main);
			break;
		case KEY_OK_PRESS:
			pscr=scr_item[selectpos].scr;
			pscr->show(&scr_main);
			break;		
		case KEY_UP_PRESS:
			if( selectpos == 0 ) selectpos = 12;
			selectpos--;
			display();
			break;
		case KEY_DOWN_PRESS:
			selectpos++;
			if( selectpos == 12 )selectpos = 0;
			display();
			break;

	}
}

/*ϵͳʱ��*/
static void timetick(unsigned int systick )
{
	tick++;
	if(selectpos<6)
	{
		if((tick&0xf)==0x00)		/*0b0000*/
		{
			lcd_bitmap(0,4,&BMP_res_arrow_none,LCD_MODE_SET);
			lcd_update(4,8);
		}
		if((tick&0x0f)==0x08)		/*0b1000*/
		{
			lcd_bitmap(0,4,&BMP_res_arrow_dn,LCD_MODE_SET);
			lcd_update(4,8);
		}
	}
	else
	{
		if((tick&0xf)==0x00)		/*0b0000*/
		{
			lcd_bitmap(0,0,&BMP_res_arrow_none,LCD_MODE_SET);
			lcd_update(0,4);
		}
		if((tick&0x0f)==0x08)		/*0b1000*/
		{
			lcd_bitmap(0,0,&BMP_res_arrow_up,LCD_MODE_SET);
			lcd_update(0,4);
		}
	}
	


}

/**/
static void msg(  void *p )
{
}

SCR scr_main =
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};

/************************************** The End Of File **************************************/

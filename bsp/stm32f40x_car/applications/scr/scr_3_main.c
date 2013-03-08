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


extern SCR scr_3_1_recorderdata;
extern SCR  scr_3_2_signal;

static SCR_ITEM scr_item[] =
{
	{ "���˵�",			6,	0				 },
	{ "1.��¼������",	12, &scr_3_1_recorderdata },
	{ "2.�źŲɼ�",		10, &scr_3_2_signal },
	{ "3.��λ��Ϣ",		10, &scr_3_1_recorderdata },
	{ "4.������������", 14, &scr_3_1_recorderdata },
	{ "5.������������", 14, &scr_3_1_recorderdata },
	{ "6.�߼�����",		10, &scr_3_1_recorderdata },
	{ "7.����Ϣ",		8, &scr_3_1_recorderdata },
	{ "8.�绰����",		10, &scr_3_1_recorderdata },
	{ "9.����Ƶ����",	12, &scr_3_1_recorderdata },
};


static uint8_t	selectpos = 1;  /*ѡ����λ��*/


/*��ʾ�˵�*/
static void menudisplay( void )
{
	lcd_fill( 0 );
	if( selectpos & 0x01 )      /*�ǵ���*/
	{
		lcd_text12( 0, 0, scr_item[selectpos-1].text, scr_item[selectpos-1].len, LCD_MODE_SET );
		lcd_fill_rect(0,16,121,28,LCD_MODE_SET);
		lcd_text12( 0, 16, scr_item[selectpos].text, scr_item[selectpos].len, LCD_MODE_INVERT);
	}
	else
	{
		lcd_fill_rect(0,0,121,12,LCD_MODE_SET);
		lcd_text12( 0, 0, scr_item[selectpos].text, scr_item[selectpos].len, LCD_MODE_INVERT);
		lcd_text12( 0, 16, scr_item[selectpos+1].text, scr_item[selectpos+1].len, LCD_MODE_SET );
	}
	lcd_update_all( );
	
}

/***/
static void show( void )
{
	selectpos = 1;
	menudisplay( );
}

/*��������*/
static void keypress(void *thiz,unsigned int key )
{
	switch( key )
	{
		case KEY_MENU_PRESS:
			*((PSCR)thiz)=*((PSCR)thiz->parent);
			((PSCR)thiz)->show();
			break;
		case KEY_UP_PRESS:
			selectpos--;
			if( selectpos == 0 ) selectpos = 9;
			menudisplay();
			break;
		case KEY_DOWN_PRESS:
			selectpos++;
			if( selectpos == 10 )selectpos = 1;
			menudisplay();
			break;
		case KEY_OK_PRESS:
			scr_item[selectpos].scr->parent=(PSCR)thiz;
			*((PSCR)thiz)=scr_item[selectpos].scr;
			((PSCR)thiz)->show();
			break;
	}
}

/*ϵͳʱ��*/
static void timetick(void *thiz, unsigned int systick )
{
}

/**/
static void msg(  void *p )
{
}

SCR scr_3_main =
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};

/************************************** The End Of File **************************************/

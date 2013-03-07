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


/*
   static PMENUITEM psubmenu[6]=
   {
   &Menu_2_2_1_license,//���ƺ�����
   &Menu_2_2_2_CarType,//��������ѡ��
   &Menu_2_2_3_SpeedSet,//�ٶ�����
   &Menu_2_2_4_VINset,//VIN����
   &Menu_2_2_5_Cancel,//���Υ�¼�¼
   &Menu_1_Idle,//�����궨
   };



   static void menuswitch(void)
   {
   int i,index;
   lcd_fill(0);
   DisAddRead_ZK(0,3,"����",2,&test_1_MeunSet,0,0);
   DisAddRead_ZK(0,17,"����",2,&test_1_MeunSet,0,0);
   for(i=0;i<5;i++) lcd_bitmap(35+index*12, 5, &BMP_noselect_set, LCD_MODE_SET);
   lcd_bitmap(35+index*12, 5, &BMP_select_set, LCD_MODE_SET);
   DisAddRead_ZK(35,19,(char *)(psubmenu[menu_pos]->caption),4,&test_1_MeunSet,1,0);
   lcd_update_all();
   }
 */

extern SCR scr_3_1_recorderdata;

static SCR_ITEM scr_item[] =
{
	{ "���˵�",			6,	0				 },
	{ "1.��¼������",	12, &scr_3_1_recorderdata },
	{ "2.�źŲɼ�",		10, &scr_3_1_recorderdata },
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
		lcd_text12( 0, 16, scr_item[selectpos].text, scr_item[selectpos].len, LCD_MODE_INVERT);
	}
	else
	{
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
			//thiz = (void*)&scr_3_1_recorderdata;
			//thiz->show( thiz, 0 );
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

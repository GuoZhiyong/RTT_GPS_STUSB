
#include "scr.h"




static SCR_ITEM scr_item[] =
{
	{ "�绰",		6,	0				 },
	{ "1.����绰",	12, &scr_3_1_recorderdata },
	{ "2.�绰��",	8, &scr_3_2_signal },
	{ "3.δ������",	10, &scr_3_1_recorderdata },
	{ "4.�ѽ�����", 10, &scr_3_1_recorderdata },
	{ "5.�Ѳ��绰", 10, &scr_3_1_recorderdata },
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
static void show( void* parent )
{
	if(parent!=0) scr_3_5_5_phonecall.parent=(PSCR)parent;
	selectpos = 1;
	menudisplay( );
}

/*��������*/
static void keypress(unsigned int key )
{
	switch( key )
	{
		case KEY_MENU_PRESS: /*���ظ��˵�*/
			pscr=scr_3_5_5_phonecall.parent;
			pscr->show((void*)0);
			break;
		case KEY_OK_PRESS:
			pscr=scr_item[selectpos].scr;
			pscr->show(&scr_3_main);
			break;		
		case KEY_UP_PRESS:
			selectpos--;
			if( selectpos == 0 ) selectpos = 5;
			menudisplay();
			break;
		case KEY_DOWN_PRESS:
			selectpos++;
			if( selectpos == 6 )selectpos = 1;
			menudisplay();
			break;

	}
}

/*ϵͳʱ��*/
static void timetick(unsigned int systick )
{
}

/**/
static void msg(  void *p )
{
}

SCR scr_3_5_5_phonecall =
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};

/************************************** The End Of File **************************************/


#include "scr.h"


static SCR_ITEM scr_item[] =
{
	{ "�ռ���",		6,	0 },
	{ "1.�¼���Ϣ",	10, 0 },
	{ "2.������Ϣ",	10, 0 },
	{ "3.�ֻ���Ϣ",	10, 0 },
	{ "4.������Ϣ", 10, 0 },
	{ "5.��Ϣ�㲥", 10, 0 },
};

static unsigned char pos=1;

static void display(void)
{
	lcd_fill(0);
	if( pos & 0x01 )      /*�ǵ���*/
	{
		lcd_text12( 0, 0, scr_item[pos-1].text, scr_item[pos-1].len, LCD_MODE_SET );
		lcd_fill_rect(0,16,121,28,LCD_MODE_SET);
		lcd_text12( 0, 16, scr_item[pos].text, scr_item[pos].len, LCD_MODE_INVERT);
	}
	else
	{
		lcd_fill_rect(0,0,121,12,LCD_MODE_SET);
		lcd_text12( 0, 0, scr_item[pos].text, scr_item[pos].len, LCD_MODE_INVERT);
		lcd_text12( 0, 16, scr_item[pos+1].text, scr_item[pos+1].len, LCD_MODE_SET );
	}

	lcd_update_all();
}

	
/**/
static void show(void *parent)
{
	scr_4_2_sms_inbox.parent=(PSCR)parent;
	display();
}


/*��������*/
static void keypress(unsigned int key)
{
	switch(key)
	{
		case KEY_MENU_PRESS:		/*�����ϼ��˵�*/
			break;
		case KEY_OK_PRESS:				
			break;
		case KEY_UP_PRESS:
			break;	
		case KEY_DOWN_PRESS:
			break;
	}
}

/*ϵͳʱ��*/
static void timetick(unsigned int systick)
{

}

/*�����Լ�״̬����Ϣ*/
static void msg(void *thiz,void *p)
{


}



SCR scr_4_2_sms_inbox=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};












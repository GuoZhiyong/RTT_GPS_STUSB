#include "scr.h"

/*
Ϊ�˼��ٽ�����
���в��ֽ��棬�������ٵĽ��漯��
*/

static SCR_ITEM scr_item[] =
{
	{ "�ռ���",		6,	0 },
	{ "1.�¼���Ϣ",	10, 0 },
	{ "2.������Ϣ",	10, 0 },
	{ "3.�ֻ���Ϣ",	10, 0 },
	{ "4.������Ϣ", 10, 0 },
	{ "5.��Ϣ�㲥", 10, 0 },
};




static unsigned char pos=0;

static void display(void)
{
	lcd_fill(0);
	lcd_text12(0,0,"��Ϣ",4,LCD_MODE_INVERT);
	lcd_text12(121-6*6,0,"0 δ��",6,LCD_MODE_SET);

	/*��� (122-36*3)/2=7 */
	lcd_text12(0,16,"����Ϣ",6,(pos==0)?LCD_MODE_INVERT:LCD_MODE_SET);
	lcd_text12(43,16,"�ռ���",6,(pos==1)?LCD_MODE_INVERT:LCD_MODE_SET);
	lcd_text12(86,16,"������",6,(pos==2)?LCD_MODE_INVERT:LCD_MODE_SET);
	lcd_update_all();
}

	
/**/
static void show(void *parent)
{
	scr_4_sms.parent=(PSCR)parent;
	display();
}


/*��������*/
static void keypress(unsigned int key)
{
	switch(key)
	{
		case KEY_MENU_PRESS:		/*�����ϼ��˵�*/
			if((void*)0==scr_3_geoinfo.parent) break;
			pscr=scr_4_sms.parent;
			pscr->show(0);
			break;
		case KEY_OK_PRESS:				
			break;
		case KEY_UP_PRESS:
			pos++;
			pos%=3;
			display();
			break;	
		case KEY_DOWN_PRESS:
			if(pos==0) pos=3;
			pos--;
			display();
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



SCR scr_4_sms=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};










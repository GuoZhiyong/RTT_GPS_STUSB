#include "scr.h"




static uint8_t pageindex=0;

/*
ֻ����ʾ��Ϣ����û���Ӳ˵�
��ʾ3ҳ����Ϣ ��γ�� ��λ ʱ��
*/
static void showpage(void)
{
	lcd_fill(0);
	switch(pageindex){
		case 0:
			lcd_text12(0,0,"�������",8,LCD_MODE_INVERT);
			lcd_text12(121-6*3,0,"0KM",3,LCD_MODE_INVERT);
			lcd_text12(0,16,"����� ",8,LCD_MODE_INVERT);
			lcd_text12(121-6*3,16,"0KM",3,LCD_MODE_INVERT);
			break;
		case 1:
			lcd_text12(0,0,"�������",6,LCD_MODE_INVERT);
			lcd_text12(121-6*3,0,"0KM",3,LCD_MODE_INVERT);
			lcd_text12(0,16,"�������",6,LCD_MODE_INVERT);
			lcd_text12(121-6*3,16,"0KM",3,LCD_MODE_INVERT);
			break;
	}		
	lcd_update_all();
}

static void show(void *parent)
{
	scr_7_mileage.parent=(PSCR)parent;
	showpage();

}


/*��������*/
static void keypress(unsigned int key)
{
	switch(key)
	{
		case KEY_MENU_PRESS:
		case KEY_OK_PRESS:				/*�����ϼ��˵�*/
			break;
		case KEY_UP_PRESS:
			if(pageindex==0) pageindex=2;
			pageindex--;
			showpage();
			break;	
		case KEY_DOWN_PRESS:
			pageindex++;
			pageindex%=2;
			showpage();
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



SCR scr_7_mileage=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};









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
			lcd_text12(0,0,"IO����",6,LCD_MODE_INVERT);
			
			
			break;
		case 1:
			lcd_text12(0,0,"ģ����1",7,LCD_MODE_INVERT);
			lcd_text12(60,0,"0.000V",6,LCD_MODE_SET);
			lcd_text12(0,16,"ģ����2",7,LCD_MODE_INVERT);
			lcd_text12(60,16,"0.000V",6,LCD_MODE_SET);
			break;
		case 2:
			lcd_text12(0,0,"��Դ",4,LCD_MODE_INVERT);
			break;
	}		
	lcd_update_all();
}

static void show(void *parent)
{
	scr_1_signal.parent=(PSCR)parent;
	pageindex=0;
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
			pageindex--;
			if(pageindex==0) pageindex=2;
			showpage();
			break;	
		case KEY_DOWN_PRESS:
			pageindex++;
			pageindex%=3;
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



SCR scr_1_signal=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};








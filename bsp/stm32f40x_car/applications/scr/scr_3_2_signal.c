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
			lcd_text12(0,0,"ģ����",6,LCD_MODE_INVERT);

			break;
		case 2:
			lcd_text12(0,0,"��Դ",4,LCD_MODE_INVERT);
			break;
	}		
	lcd_update_all();
}

static void show(void)
{
	pageindex=0;
	showpage();

}


/*��������*/
static void keypress(void *thiz,unsigned int key)
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
static void timetick(void *thiz,unsigned int systick)
{

}

/*�����Լ�״̬����Ϣ*/
static void msg(void *thiz,void *p)
{


}



SCR scr_3_2_signal=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};








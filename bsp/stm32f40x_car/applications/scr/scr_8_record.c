#include "scr.h"





static void display(void)
{
	lcd_fill(0);
	lcd_text12(0,0,"��¼",4,LCD_MODE_INVERT);
	lcd_text12((121-6*11)/2,16,"�޼�¼��Ϣ!",11,LCD_MODE_SET);
	lcd_update_all();
}

static void show(void *parent)
{
	scr_8_record.parent=(PSCR)parent;
	display();

}


/*��������*/
static void keypress(unsigned int key)
{
	switch(key)
	{
		case KEY_MENU_PRESS:/*�����ϼ��˵�*/
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



SCR scr_8_record=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};









#include "scr.h"



static void show(void *parent)
{
	scr_6_advance.parent=(PSCR)parent;
	lcd_fill(0);
	lcd_text12((122-6*12)/2,0,"�����û�����",12,LCD_MODE_INVERT);
	lcd_asc0608((122-4*12)/2,0,"_",1,LCD_MODE_INVERT);
	lcd_update_all();

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



SCR scr_6_advance=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};









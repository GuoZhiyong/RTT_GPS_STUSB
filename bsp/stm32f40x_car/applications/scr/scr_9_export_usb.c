#include "scr.h"

static void show(void *parent)
{
	lcd_fill(0);
	lcd_fill_rect(0,0,121,12,LCD_MODE_SET);
	lcd_text12(16,0,"���ڵ�������...",15,LCD_MODE_INVERT);
	
	lcd_rect(10,18,80,10,LCD_MODE_SET);
	lcd_fill_rect(10,18,70,16+10-1,LCD_MODE_SET);


	lcd_update_all();
}


/*��������*/
static void keypress(unsigned int key)
{

}

/*ϵͳʱ��*/
static void timetick(unsigned int systick)
{

}

/*todo ����USB���ݵ�������Ϣ*/
static void msg(void *thiz,void *p)
{


}



SCR scr_9_export_usb=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};






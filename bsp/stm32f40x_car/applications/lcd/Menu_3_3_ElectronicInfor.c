#include "Menu_Include.h"
#include "sed1520.h"
static void msg( void *p)
{
}
static void show(void)
{
	pMenuItem->tick=rt_tick_get();

	lcd_fill(0);
	lcd_text12(0,10,"��ȷ�ϼ����͵����˵�",20,LCD_MODE_SET);
	lcd_update_all();
}


static void keypress(unsigned int key)
{

switch(key)
	{
	case KEY_MENU:
		pMenuItem=&Menu_3_InforInteract;
		pMenuItem->show();
		CounterBack=0;
		break;
	case KEY_OK:
		jt808_tx(0x0701,"0123456789",10);
		lcd_fill(0);
		lcd_text12(10,10,"�����˵��ϱ����",16,LCD_MODE_SET);
		lcd_update_all();
		break;
	case KEY_UP:
		break;
	case KEY_DOWN:
		break;
	}
}



MENUITEM	Menu_3_3_ElectronicInfor=
{
    "�����˵�����",
	12,0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};


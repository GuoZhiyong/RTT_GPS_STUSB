#include "menu_include.h"
#include "Lcd_init.h"


static void show(void)
{
	
	lcd_fill(0);
	lcd_text12(0,10,"Ver:MG323_1.1",13,LCD_MODE_SET);
	lcd_update_all();
	
}


static void keypress(unsigned int key)
{
	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_1_Idle;
			pMenuItem->show();
			CounterBack=0;
			break;
		case KeyValueOk:
			break;
		case KeyValueUP:  
			break;
		case KeyValueDown:
			break;
		}
 KeyValue=0;
}


static void timetick(unsigned int systick)
{
	CounterBack++;
	if(CounterBack!=MaxBankIdleTime)
		return;
	CounterBack=0;
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();

}


MENUITEM	Menu_2_6_1_Ver=
{
	"",
	&show,
	&keypress,
	&timetick,
	(void*)0
};


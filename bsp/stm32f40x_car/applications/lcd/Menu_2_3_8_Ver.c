#include "Menu_Include.h"
#include "Lcd.h"


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

ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_2_3_8_Ver=
{
    "∞Ê±æ≤È—Ø",
	8,
	&show,
	&keypress,
	&timetick,
	(void*)0
};


#include "Menu_Include.h"




u8 version_screen=0;
static void msg( void *p)
{

}
static void show(void)
{
	version_disp();
}


static void keypress(unsigned int key)
{

	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_5_other;
			pMenuItem->show();
			CounterBack=0;

			version_screen=0;
			break;
		case KeyValueOk:
			version_disp();
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
       Cent_To_Disp();
	CounterBack++;
	if(CounterBack!=MaxBankIdleTime*5)
		return;
	CounterBack=0;
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();



}


MENUITEM	Menu_5_7_Version=
{
"�汾��ʾ",
	8,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};


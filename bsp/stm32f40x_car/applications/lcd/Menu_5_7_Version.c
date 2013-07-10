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

	switch(key)
		{
		case KEY_MENU:
			pMenuItem=&Menu_5_other;
			pMenuItem->show();
			CounterBack=0;

			version_screen=0;
			break;
		case KEY_OK:
			version_disp();
			break;
		case KEY_UP:
			break;
		case KEY_DOWN:
			break;
		}
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
"∞Ê±æœ‘ æ",
	8,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};


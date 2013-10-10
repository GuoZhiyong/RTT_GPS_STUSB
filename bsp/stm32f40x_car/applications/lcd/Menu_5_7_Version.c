#include "Menu_Include.h"




u8 version_screen=0;
static void msg( void *p)
{

}
static void show(void)
{
	pMenuItem->tick=rt_tick_get();

	//version_disp();
}


static void keypress(unsigned int key)
{

	switch(key)
		{
		case KEY_MENU:
			pMenuItem=&Menu_5_other;
			pMenuItem->show();

			version_screen=0;
			break;
		case KEY_OK:
			//version_disp();
			break;
		case KEY_UP:
			break;
		case KEY_DOWN:
			break;
		}
}



MENUITEM	Menu_5_7_Version=
{
"∞Ê±æœ‘ æ",
	8,0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};


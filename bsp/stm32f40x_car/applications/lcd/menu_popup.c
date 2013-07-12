#include "Menu_Include.h"
#include "sed1520.h"



static void msg( void *p)
{
}

static void show(void)
{

}


static void keypress(unsigned int key)
{
switch(key)
	{
	case KEY_MENU:
		break;
	case KEY_OK:
		break;
	case KEY_UP:
		break;
	case KEY_DOWN:
		break;
	}

}

	

static void timetick(unsigned int systick)
{

}

MENUITEM	Menu_3_6_Record=
{
	"ÏûÏ¢",
	4,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};



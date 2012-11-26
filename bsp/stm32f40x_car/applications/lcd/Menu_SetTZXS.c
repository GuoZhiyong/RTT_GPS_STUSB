#include "menu.h"
#include "Lcd_init.h"

struct IMG_DEF test_dis_tzxs={12,12,test_00};

static void show(void)
	{
	}


static void keypress(unsigned int key)
{
switch(KeyValue)
	{
	case KeyValueMenu:
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
else
	{
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();
	CounterBack=0;
	}
}


MENUITEM	Menu_SetTZXS=
{
	"",
	&show,
	&keypress,
	&timetick,
	(void*)0
};


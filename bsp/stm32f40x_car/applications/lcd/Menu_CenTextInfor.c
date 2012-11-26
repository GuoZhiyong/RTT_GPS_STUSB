#include "menu.h"
	 #include <string.h>
struct IMG_DEF test_dis_text={12,12,test_00};


static void show(void)
	{
	memset(test_idle,0,sizeof(test_idle));
	lcd_fill(0);
	DisAddRead_ZK(20,3,"您有一条新消息",7,&test_dis_text,0,0);
	DisAddRead_ZK(26,19,"按确认键查看",6,&test_dis_text,0,0);
	lcd_update_all();
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


MENUITEM	Menu_CenterTextInfor=
{
	"",
	&show,
	&keypress,
	&timetick,
	(void*)0
};


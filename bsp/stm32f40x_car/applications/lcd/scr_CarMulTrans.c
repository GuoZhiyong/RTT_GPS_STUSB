#include "menu.h"


struct IMG_DEF test_dis_CarMulTrans={12,12,test_00};

static void show(void)
{
	lcd_fill(0);
	DisAddRead_ZK(20,3,"多媒体事件上传",7,&test_dis_CarMulTrans,0,0);
    lcd_text(50,19,FONT_NINE_DOT,"OK ?");
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
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();
	CounterBack=0;					


}


MENUITEM	scr_CarMulTrans=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};


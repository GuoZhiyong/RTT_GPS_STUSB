#include "menu.h"

struct IMG_DEF test_dis_CarEleInfor={12,12,test_00};

static void show(void)
{
	lcd_fill(0);
	DisAddRead_ZK(25,3,"电子运单发送",6,&test_dis_CarEleInfor,0,0);
    lcd_text(50,19,FONT_NINE_DOT,"OK ?");
	lcd_update_all();
}


static void keypress(unsigned int key)
{
switch(KeyValue)
	{
	case KeyValueMenu:
		pMenuItem=&Menu_1_InforInteract;//scr_CarMulTrans;
		pMenuItem->show();
		CounterBack=0;

		break;
	case KeyValueOk:
		lcd_fill(0);
		DisAddRead_ZK(10,10,"电子运单发送成功",8,&test_dis_CarEleInfor,1,0);
		lcd_update_all();
		//SD_ACKflag.f_Worklist_SD_0701H=1;//按键选择发送电子运单标志
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


MENUITEM	Menu_2_4_3_CarEleInfor=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};


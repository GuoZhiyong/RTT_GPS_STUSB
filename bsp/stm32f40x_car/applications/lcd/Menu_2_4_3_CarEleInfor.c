#include "Menu_Include.h"


static void show(void)
{
	lcd_fill(0);
	lcd_text12(0,10,"��ȷ�ϼ����͵����˵�",20,LCD_MODE_SET);
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
		lcd_text12(10,10,"�����˵����ͳɹ�",16,LCD_MODE_SET);
		lcd_update_all();
		//SD_ACKflag.f_Worklist_SD_0701H=1;//����ѡ���͵����˵���־
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

ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_2_4_3_CarEleInfor=
{
    "�����˵�����",
	12,
	&show,
	&keypress,
	&timetick,
	(void*)0
};


#include "menu.h"
#include "Lcd_init.h"

struct IMG_DEF test_dis_single={12,12,test_00};

unsigned char UpdateSingleCounter=0;

void Single_8(void)
{
	lcd_fill(0);
	DisAddRead_ZK(0,10,"信号线",3,&test_dis_single,0,0);
	lcd_text(36,10,FONT_TEN_DOT,(char *)XinhaoStatus);
	lcd_update_all();
}
static void show(void)
{
	Single_8();
}


static void keypress(unsigned int key)
{
	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_1_InforCheck;
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
	UpdateSingleCounter++;
	if(UpdateSingleCounter>=20)
		{
		UpdateSingleCounter=0;
		Single_8();
		}

	CounterBack++;
	if(CounterBack!=MaxBankIdleTime)
		return;
	CounterBack=0;
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();

}


MENUITEM	Menu_2_3_1_Sing8=
{
	"信号线状态",
	&show,
	&keypress,
	&timetick,
	(void*)0
};



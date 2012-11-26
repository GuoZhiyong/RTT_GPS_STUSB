#include  <string.h>
#include "menu.h"
#include "Lcd_init.h"

struct IMG_DEF test_dis_ExpSpeed={12,12,test_00};

static void show(void)
{
lcd_fill(0);
DisAddRead_ZK(12,0,"查看超速驾驶记录",8,&test_dis_ExpSpeed,0,0);
lcd_text(40,16,FONT_NINE_DOT,"OK ?");
lcd_update_all();


}
static void keypress(unsigned int key)
{
	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_1_Idle;
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
	CounterBack++;
	if(CounterBack!=MaxBankIdleTime)
		return;
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();
	CounterBack=0;
}




MENUITEM	Menu_2_5_2_chaosu=
{
	"",
	&show,
	&keypress,
	&timetick,
	(void*)0
};



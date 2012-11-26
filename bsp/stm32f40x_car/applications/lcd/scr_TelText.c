#include "menu.h"
//#include "Lcd_init.h"
#include <string.h>

struct IMG_DEF test_dis_TelText={12,12,test_00};

unsigned char Menu_TelText=0;
unsigned char TelText_scree=0;
static void show(void)
	{
    memset(test_idle,0,sizeof(test_idle));
	lcd_fill(0);
	DisAddRead_ZK(20,0,"电话本记录查看",7,&test_dis_TelText,0,0);
	lcd_text(40,16,FONT_NINE_DOT,"OK ?");
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


MENUITEM	scr_TelText=
{
	"",
	&show,
	&keypress,
	&timetick,
	(void*)0
};


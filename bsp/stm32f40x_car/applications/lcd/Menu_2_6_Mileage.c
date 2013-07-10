#include "Menu_Include.h"
#include <string.h>
#include <stdlib.h>
#include "sed1520.h"


unsigned char licheng_sum[18]={"总里程:000000 KM"};

static void msg( void *p)
{
}
static void show(void)
{
	unsigned long DisKm=0;

	DisKm=jt808_param.id_0xF020/1000;//单位:m

	licheng_sum[7] =DisKm%1000000/100000+0x30;
	licheng_sum[8] =DisKm%100000/10000+0x30;
	licheng_sum[9] =DisKm%10000/1000+0x30;
	licheng_sum[10]=DisKm%1000/100+0x30;
	licheng_sum[11]=DisKm%100/10+0x30;
	licheng_sum[12]=DisKm%10+0x30; 
	
    //ultoa(DisKm,licheng_sum,10);

	lcd_fill(0);
	lcd_text12(0, 3,(char *)Dis_date,20,LCD_MODE_SET);
	lcd_text12(0,18,(char *)licheng_sum,16,LCD_MODE_SET);
	lcd_update_all();
}


static void keypress(unsigned int key)
{
switch(key)
	{
	case KEY_MENU:
		pMenuItem=&Menu_2_InforCheck;
		pMenuItem->show();
		CounterBack=0;
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
    CounterBack++;
	if(CounterBack!=MaxBankIdleTime)  // 界面显示超时时退回到待机界面
		return;
	
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();
	
	CounterBack=0;

}


MENUITEM	Menu_2_6_Mileage=
{
	"里程信息查看",
	12,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

#include "menu.h"
#include "Lcd_init.h"

struct IMG_DEF test_dis_Mileage={12,12,test_00};


unsigned char licheng_sum[12]={":000000 km"};

static void show(void)
{
	unsigned long DisKm=0;

	licheng_sum[1]=DisKm%1000000/100000+0x30;
	licheng_sum[2]=DisKm%100000/10000+0x30;
	licheng_sum[3]=DisKm%10000/1000+0x30;
	licheng_sum[4]=DisKm%1000/100+0x30;
	licheng_sum[5]=DisKm%100/10+0x30;
	licheng_sum[6]=DisKm%10+0x30; 


	lcd_fill(0);
	lcd_text(0,5,FONT_SEVEN_DOT,"20");
	lcd_text(12,5,FONT_SEVEN_DOT,(char *)Dis_date);
	lcd_text(70,5,FONT_SEVEN_DOT,(char *)Dis_time);
	DisAddRead_ZK(0,17,"总里程",3,&test_dis_Mileage,0,0);
	lcd_text(36,18,FONT_NINE_DOT,(char *)licheng_sum);
	lcd_update_all();
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
    CounterBack++;
	if(CounterBack!=MaxBankIdleTime)  // 界面显示超时时退回到待机界面
		return;
	
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();
	
	CounterBack=0;

}


MENUITEM	Menu_2_3_6_Mileage=
{
	"里程信息查看",
	&show,
	&keypress,
	&timetick,
	(void*)0
};

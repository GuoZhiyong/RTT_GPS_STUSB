#include "Menu_Include.h"



u8 RertartGps_screen=0;

static void msg( void *p)
{
}
static void show(void)
{
	lcd_fill(0);
	lcd_text12(24,10,"按确认键冷启动",12,LCD_MODE_SET);
	lcd_update_all();
	
	RertartGps_screen=0;
}


static void keypress(unsigned int key)
{

	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_5_other;
			pMenuItem->show();
			CounterBack=0;

			RertartGps_screen=0;
			break;
		case KeyValueOk:
			if(RertartGps_screen==0)
				{
				RertartGps_screen=1;
				lcd_fill(0);
				lcd_text12(6,10,"北斗模块冷启动成功",18,LCD_MODE_INVERT);
				lcd_update_all();
				
				 //---- 全冷启动 ------
				 /*
                                  $CCSIR,1,1*48
                                  $CCSIR,2,1*48 
                                  $CCSIR,3,1*4A 
				 */		    
				if(GpsStatus.Position_Moule_Status==1)
					{
					gps_mode("1");
					rt_kprintf("\r\n北斗模式下冷启动");
					}
				else if(GpsStatus.Position_Moule_Status==2)
					{
					gps_mode("2");
					rt_kprintf("\r\nGPS模式下冷启动");
					}
				else if(GpsStatus.Position_Moule_Status==3)
					{
					gps_mode("3");
					rt_kprintf("\r\n双模下冷启动");
					}
				}
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
	Cent_To_Disp(); 
	CounterBack++;
	if(CounterBack!=MaxBankIdleTime*5)
		return;
	CounterBack=0;
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();
	
      RertartGps_screen=0;
}

MYTIME
MENUITEM	Menu_5_4_bdColdBoot=
{
"北斗模块冷启动",
	14,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};


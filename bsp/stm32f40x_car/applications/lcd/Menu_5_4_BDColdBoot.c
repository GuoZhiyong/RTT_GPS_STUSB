#include "Menu_Include.h"
#include "sed1520.h"


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

	switch(key)
		{
		case KEY_MENU:
			pMenuItem=&Menu_5_other;
			pMenuItem->show();
			CounterBack=0;

			RertartGps_screen=0;
			break;
		case KEY_OK:
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
					gps_mode(gps_status.Position_Moule_Status);
				}
			break;
		case KEY_UP:
			break;
		case KEY_DOWN:
			break;
		}
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


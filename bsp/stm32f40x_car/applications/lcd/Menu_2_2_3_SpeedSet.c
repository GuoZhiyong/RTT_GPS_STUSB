#include  <string.h>
#include "menu.h"
#include "Lcd_init.h"

struct IMG_DEF test_dis_SpeedSet={12,12,test_00};

unsigned char speedsel_flag=0;

void SpeedSet(unsigned char sel)
{
if(sel==1)
	{
	lcd_fill(0);
	DisAddRead_ZK(36,3,"速度设置",4,&test_dis_SpeedSet,0,0);
	
	lcd_text(3,19,FONT_NINE_DOT,"GPS");
	DisAddRead_ZK(30,19,"速度",2,&test_dis_SpeedSet,1,0);
	DisAddRead_ZK(60,19,"传感器速度",5,&test_dis_SpeedSet,0,0);
	lcd_update_all();
	}
else if(sel==2)
	{
	lcd_fill(0);
	DisAddRead_ZK(36,3,"速度设置",4,&test_dis_SpeedSet,0,0);
	lcd_text(3,19,FONT_NINE_DOT,"GPS");
	DisAddRead_ZK(30,19,"速度",2,&test_dis_SpeedSet,0,0);
	DisAddRead_ZK(60,19,"传感器速度",5,&test_dis_SpeedSet,1,0);
	lcd_update_all();
	}
else if(sel==3)
	{
	lcd_fill(0);
	DisAddRead_ZK(20,10,"设备取",3,&test_dis_SpeedSet,0,0);
	lcd_text(57,10,FONT_NINE_DOT,"GPS");
	DisAddRead_ZK(83,10,"速度",2,&test_dis_SpeedSet,0,0);
	lcd_update_all();
	}
else if(sel==4)
	{
	lcd_fill(0);
	DisAddRead_ZK(10,10,"设备取传感器速度",8,&test_dis_SpeedSet,0,0);
	lcd_update_all();
	}
}

static void show(void)
{
	SpeedSet(1);
	speedsel_flag=1;
}


static void keypress(unsigned int key)
{
	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_1_CarSet;
			pMenuItem->show();
			CounterBack=0;
			speedsel_flag=0;
			break;
		case KeyValueOk:
			if(speedsel_flag==1)
				{
				speedsel_flag=10;
				SpeedSet(3);
				
				//设备取GPS速度
				//Speed_GetType=0; 
				//DF_WriteFlashSector(DF_Speed_GetType_Page,0,(u8*)&Speed_GetType,1);
				}
			else if(speedsel_flag==2)
				{
				speedsel_flag=10;
				SpeedSet(4);

                //设备取速度线
				//Speed_GetType=1; 
				//DF_WriteFlashSector(DF_Speed_GetType_Page,0,(unsigned char*)&Speed_GetType,1);
	
				}
			else if(speedsel_flag==10)
				{
				speedsel_flag=0;
				pMenuItem=&Menu_1_CarSet;
				pMenuItem->show();
				}
			break;
		case KeyValueUP:
			speedsel_flag=1;
			SpeedSet(1);
			break;
		case KeyValueDown:
			speedsel_flag=2;
			SpeedSet(2);
			break;
		}
	KeyValue=0;
}


static void timetick(unsigned int systick)
{
	CounterBack++;
	if(CounterBack!=MaxBankIdleTime)
		return;
	CounterBack=0;
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();
	speedsel_flag=0;
}


MENUITEM	Menu_2_2_3_SpeedSet=
{
	"速度设置",
	&show,
	&keypress,
	&timetick,
	(void*)0
};



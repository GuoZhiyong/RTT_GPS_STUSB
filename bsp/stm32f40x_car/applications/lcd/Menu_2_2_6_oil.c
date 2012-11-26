#include  <string.h>
#include "menu.h"
#include "Lcd_init.h" 




struct IMG_DEF test_scr_oil={12,12,test_00};
unsigned char oil_fetch_cmd=0;
unsigned char oil_fetch_counter=0;


static void show(void)
{
	lcd_fill(0);
	DisAddRead_ZK(36,3,"油量标定",4,&test_scr_oil,0,0);
	DisAddRead_ZK(30,17,"请按确认键",5,&test_scr_oil,0,0);
	lcd_update_all();
	oil_fetch_cmd=1;

}


static void keypress(unsigned int key)
{

	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_1_CarSet;
			pMenuItem->show();
			break;
		case KeyValueOk:
			if(oil_fetch_cmd==1)
				{
				oil_fetch_cmd=2;
				//OilDevice_HBTQ_Tx();
				//---------------------------------------
				OilEnable=1;//查油耗值标志
				OilonCounter=0;
			    Oil_Query_timer=0;
				//---------------------------------------
				oil_fetch_counter=0;
				lcd_fill(0);
				DisAddRead_ZK(30,3,"命令已发送",5,&test_scr_oil,0,0);
				DisAddRead_ZK(42,18,"等待中",3,&test_scr_oil,0,0);
				lcd_update_all();
                OilonCounter=0; 
				OilFetchError=0;
				}
			else if(oil_fetch_cmd==3)
				{
				oil_fetch_cmd=1;
				lcd_fill(0);
				DisAddRead_ZK(36,3,"油量标定",4,&test_scr_oil,0,0);
				DisAddRead_ZK(30,17,"请按确认键",5,&test_scr_oil,0,0);
				lcd_update_all();
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
	u8 i=0;

	if(oil_fetch_cmd==2)
		{
		oil_fetch_counter++;
		if(oil_fetch_counter>=120)
			{
			oil_fetch_counter=0;
			oil_fetch_cmd=3;
            if(OilFetchError==1)
            	{
            	OilFetchError=0;
            	lcd_fill(0);
				DisAddRead_ZK(12,3,"油量标定接收超时",8,&test_scr_oil,0,0);
				DisAddRead_ZK(42,18,"请重取",3,&test_scr_oil,0,0);
				lcd_update_all();
				OilEnable=0;// clear 
				OilonCounter=0; 
				OilFetchError=0;
            	}
			}
		if(OilEnableDisplay==2)
			{
			OilEnableDisplay=0;
			lcd_fill(0);
			
			DisAddRead_ZK(0,0,"油耗值",3,&test_scr_oil,0,0);
			lcd_text(36,3,FONT_SIX_DOT,oil_ADValue);
			DisAddRead_ZK(66,0,"容积",2,&test_scr_oil,0,0);
			lcd_text(90,3,FONT_SIX_DOT,oil_VolumeValue);

			DisAddRead_ZK(0,12,"油杆有效长度",6,&test_scr_oil,0,0);
			lcd_text(72,16,FONT_SIX_DOT,oil_DipRodValue);
				
			lcd_text(0,24,FONT_SIX_DOT,Oil_info);
			lcd_update_all();
			
			OilEnable=0;// clear 
			OilonCounter=0; 
			OilFetchError=0;
			
			oil_fetch_cmd=3;
			}
		}
}


MENUITEM	Menu_2_2_6_oil=
{
	"",
	&show,
	&keypress,
	&timetick,
	(void*)0
};



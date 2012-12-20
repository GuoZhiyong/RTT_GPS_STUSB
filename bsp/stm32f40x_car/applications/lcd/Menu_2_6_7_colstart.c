#include "Menu_Include.h"



u8 RertartGps_screen=0;

static void show(void)
{
	lcd_fill(0);
	lcd_text12(24,10,"��������ģ��",12,LCD_MODE_SET);
	lcd_update_all();
	
	RertartGps_screen=0;
}


static void keypress(unsigned int key)
{

	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_DisTelText;
			pMenuItem->show();
			CounterBack=0;

			RertartGps_screen=0;
			break;
		case KeyValueOk:
			if(RertartGps_screen==0)
				{
				RertartGps_screen=1;
				lcd_fill(0);
				lcd_text12(18,10,"����ģ��������",14,LCD_MODE_INVERT);
				lcd_update_all();

				 //---- ȫ������ ------
				 /*
                                  $CCSIR,1,1*48
                                  $CCSIR,2,1*48 
                                  $CCSIR,3,1*4A 
				 */		    
				if(gps_bd_modeFLAG==1)
					{
					//GPS_PutStr("$CCSIR,1,1*48\r\n");//���
					//delay_ms(20);
					rt_kprintf("\r\n����ģʽ��������");
					}
				else if(gps_bd_modeFLAG==2)
					{
					//GPS_PutStr("$CCSIR,2,1*48\r\n");//gps
					//delay_ms(20);
					rt_kprintf("\r\nGPSģʽ��������");
					}
				else if(gps_bd_modeFLAG==3)
					{
					//GPS_PutStr("$CCSIR,3,1*48\r\n");   //˫ģ
				    //delay_ms(20);
					rt_kprintf("\r\n˫ģ��������");
					}
				}
			else
				{
				lcd_fill(0);
				lcd_text12(6,10,"����ģ���������ɹ�",18,LCD_MODE_INVERT);
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
	Cent_To_Disp(); 
	CounterBack++;
	if(CounterBack!=MaxBankIdleTime*5)
		return;
	CounterBack=0;
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();
	
      RertartGps_screen=0;
}


MENUITEM	Menu_2_6_7_colstart=
{
"����ģ��������",
	14,
	&show,
	&keypress,
	&timetick,
	(void*)0
};


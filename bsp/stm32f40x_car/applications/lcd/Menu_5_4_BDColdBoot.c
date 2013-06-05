#include "Menu_Include.h"



u8 RertartGps_screen=0;
u8 sele_flag=0;  //  1:������ģʽ   2:���ö�λģʽ
u8 bd_mode_flag=0;
static void set_select(u8 type)
	{
	lcd_fill(0);
	if(type==1)
		{
		lcd_text12(24,3,"����ģ��������",14,LCD_MODE_INVERT);
		lcd_text12(24,18,"��λģʽ����",12,LCD_MODE_SET);
		}
	else if(type==2)
		{
		lcd_text12(24,3,"����ģ��������",14,LCD_MODE_SET);
		lcd_text12(24,18,"��λģʽ����",12,LCD_MODE_INVERT);
		}
	lcd_update_all();
	}
static void BD_mode(u8 type)
	{
	lcd_fill(0);
	if(type==1)
		{
		lcd_text12(0,10,"����",4,LCD_MODE_INVERT);
		lcd_text12(24,10,"    GPS    ˫ģ",15,LCD_MODE_SET);
		}
	else if(type==2)
		{
		lcd_text12(0,10,"����    ",8,LCD_MODE_SET);
		lcd_text12(48,10,"GPS",3,LCD_MODE_INVERT);
		lcd_text12(72,10,"    ˫ģ",8,LCD_MODE_SET);
		}
	else if(type==3)
		{
		lcd_text12(0,10,"����    GPS    ",15,LCD_MODE_SET);
		lcd_text12(96,10,"˫ģ",4,LCD_MODE_INVERT);
		}
	lcd_update_all();
	}
static void msg( void *p)
{
}
static void show(void)
{
    set_select(1);
	RertartGps_screen=1;
	sele_flag=1;
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
			sele_flag=0;  //  1:������ģʽ   2:���ö�λģʽ
			bd_mode_flag=0;
			break;
		case KeyValueOk:
			if(RertartGps_screen==1)
				{
				if(sele_flag==1)  
					{
					sele_flag=0;
					RertartGps_screen=2;//  ����ģ��������
					lcd_fill(0);
					lcd_text12(6,10,"����ģ���������ɹ�",18,LCD_MODE_INVERT);
					lcd_update_all();
									 //---- ȫ������ ------
					 /*
	                                  $CCSIR,1,1*48
	                                  $CCSIR,2,1*48 
	                                  $CCSIR,3,1*4A 
					 */		    
					if(GpsStatus.Position_Moule_Status==1)
						{
						gps_mode("1");
						rt_kprintf("\r\n����ģʽ��������");
						}
					else if(GpsStatus.Position_Moule_Status==2)
						{
						gps_mode("2");
						rt_kprintf("\r\nGPSģʽ��������");
						}
					else if(GpsStatus.Position_Moule_Status==3)
						{
						gps_mode("3");
						rt_kprintf("\r\n˫ģ��������");
						}
					}
				else if(sele_flag==2)  //��λģʽ����
					{
					sele_flag=0;
					RertartGps_screen=3;//  ����ģ��������
					//BD
					bd_mode_flag=1;
					BD_mode(bd_mode_flag);
					}
				}
			else if(RertartGps_screen==3)
				{
				RertartGps_screen=4;
				if(bd_mode_flag==1)
					{
					gps_mode("1");
					lcd_fill(0);
					lcd_text12(12,10,"����ģʽ���óɹ�",16,LCD_MODE_INVERT);
					lcd_update_all();
					}
				else if(bd_mode_flag==2)
					{
					gps_mode("2");
					lcd_fill(0);
					lcd_text12(15,10,"GPSģʽ���óɹ�",15,LCD_MODE_INVERT);
					lcd_update_all();
					}
				else if(bd_mode_flag==3)
					{
					gps_mode("3");
					lcd_fill(0);
					lcd_text12(12,10,"˫ģģʽ���óɹ�",16,LCD_MODE_INVERT);
					lcd_update_all();
					}
				}
			break;
		case KeyValueUP:
			if(RertartGps_screen==1)
				{
				sele_flag=1;
				set_select(1);
				}
			else if(RertartGps_screen==3)
				{
				if(bd_mode_flag>1)
					bd_mode_flag--;
				else
					bd_mode_flag=3;
				BD_mode(bd_mode_flag);
				}
			break;
		case KeyValueDown:
			if(RertartGps_screen==1)
				{
				sele_flag=2;
				set_select(2);
				}
			else if(RertartGps_screen==3)
				{
				if(bd_mode_flag<3)
					bd_mode_flag++;
				else
					bd_mode_flag=1;
				BD_mode(bd_mode_flag);
				}
			break;
		}
 KeyValue=0;
}


static void timetick(unsigned int systick)
{
	CounterBack++;
	if(CounterBack!=MaxBankIdleTime*5)
		return;
	CounterBack=0;
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();
	
	RertartGps_screen=0;
	sele_flag=0;  //  1:������ģʽ   2:���ö�λģʽ
	bd_mode_flag=0;
}

ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_5_4_bdColdBoot=
{
"����ģ�����",
	12,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};


#include "Menu_Include.h"

unsigned char car_status_str[3][4]={"�ճ�","���","�س�"};

unsigned char CarStatus_change=1;//״̬ѡ��
unsigned char CarStatus_screen=0;//�����л�ʹ��



void CarStatus(unsigned char Status)
{
unsigned char i=0;

	lcd_fill(0);
	lcd_text12(12,3,"��������״̬ѡ��",16,LCD_MODE_SET);
	for(i=0;i<3;i++)
		lcd_text12(20+i*30,19,(char *)car_status_str[i],4,LCD_MODE_SET);
	lcd_text12(20+30*Status,19,(char *)car_status_str[Status-1],4,LCD_MODE_INVERT);
	lcd_update_all();
}
static void show(void)
{
	CarStatus(1);
}


static void keypress(unsigned int key)
{
switch(KeyValue)
	{
	case KeyValueMenu:
		pMenuItem=&Menu_1_InforInteract;
		pMenuItem->show();
		CounterBack=0;
		
		CarStatus_change=1;//ѡ��
		CarStatus_screen=0;//�����л�ʹ��
		break;
	case KeyValueOk:
		
		if(CarStatus_screen==0)
			{
			CarStatus_screen=1;
			lcd_fill(0);
			lcd_text12(12,10,"���ͳ���״̬",12,LCD_MODE_SET);
			lcd_text12(88,10,(char *)car_status_str[CarStatus_change-1],4,LCD_MODE_SET);
			lcd_update_all();
			//CarLoadState_Flag=CarStatus_change;
			//CarLoadState_Write();
			}
		else if(CarStatus_screen==1)
			{
			CarStatus_screen=2;	
			lcd_fill(0);
			lcd_text12(20,10,(char *)car_status_str[CarStatus_change-1],4,LCD_MODE_SET);
			lcd_text12(48,10,"���ͳɹ�",8,LCD_MODE_SET);
			lcd_update_all();
			}
		break;
	case KeyValueUP:
		if(CarStatus_screen==0)
			{			
			CarStatus_change--;
			if(CarStatus_change<=1)
				CarStatus_change=1;
			CarStatus(CarStatus_change);
			}
		break;
	case KeyValueDown:
		if(CarStatus_screen==0)
			{		
			CarStatus_change++;
			if(CarStatus_change>=3)
				CarStatus_change=3;
			CarStatus(CarStatus_change);
			}
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

ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_2_4_2_CarStatus= {

    "����״̬����",
    12,
	&show,
	&keypress,
	&timetick,
	(void*)0
};


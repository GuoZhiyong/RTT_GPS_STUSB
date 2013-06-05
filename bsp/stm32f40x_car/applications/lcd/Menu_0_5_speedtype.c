#include  <string.h>
#include "Menu_Include.h"

static u8 sel_speedtype_flag=0;
static u8 speedtype=0;
static void msg( void *p)
{

}
static void show(void)
{
	sel_speedtype_flag=1;
	speedtype=1;
	menu_speedtype=0;//�ٶȷ�ʽ��Ҫд��    JT808Conf_struct   ,�ȱ����ȫ��������ɺ�д��
	
    lcd_fill(0);
	lcd_text12(10,3,"�ٶȻ�ȡ��ʽ",12,LCD_MODE_SET);	   
	lcd_text12(0,18,"GPS�ٶ�",7,LCD_MODE_INVERT);
	lcd_text12(60,18,"�������ٶ�",10,LCD_MODE_SET); 
	lcd_update_all();
}


static void keypress(unsigned int key)
{
	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_0_loggingin;
			pMenuItem->show();

			sel_speedtype_flag=0;
			speedtype=0;
			break;
		case KeyValueOk:
			if(sel_speedtype_flag==1)
				{
				sel_speedtype_flag=2;
				
				lcd_fill(0);
				lcd_text12(24,3,"�ٶȻ�ȡ��ʽ",12,LCD_MODE_SET);
				if(speedtype==1)		
					lcd_text12(15,18,"�豸ʹ��GPS�ٶ�",15,LCD_MODE_SET);
				else if(speedtype==2)
					lcd_text12(6,18,"�豸ʹ�ô������ٶ�",18,LCD_MODE_SET);
				lcd_update_all();
				
				}
			else if(sel_speedtype_flag==2)
				{
				CarSet_0_counter=5;
				sel_speedtype_flag=0;
				pMenuItem=&Menu_0_loggingin;
				pMenuItem->show();
				}
			break;
		case KeyValueUP:
			if(sel_speedtype_flag==1)
				{
				speedtype=1;
				menu_speedtype=0;//�ٶȷ�ʽ��Ҫд��    JT808Conf_struct   ,�ȱ����ȫ��������ɺ�д��
	
				lcd_fill(0);
				lcd_text12(10,3,"�ٶȻ�ȡ��ʽ",12,LCD_MODE_SET);	   
				lcd_text12(0,18,"GPS�ٶ�",7,LCD_MODE_INVERT);
				lcd_text12(60,18,"�������ٶ�",10,LCD_MODE_SET); 
				lcd_update_all();
				}
			break;
		case KeyValueDown:
			if(sel_speedtype_flag==1)
				{
				speedtype=2;
				menu_speedtype=1;//�ٶȷ�ʽ��Ҫд��    JT808Conf_struct   ,�ȱ����ȫ��������ɺ�д��
	
				lcd_fill(0);
				lcd_text12(10,3,"�ٶȻ�ȡ��ʽ",12,LCD_MODE_SET);	   
				lcd_text12(0,18,"GPS�ٶ�",7,LCD_MODE_SET);
				lcd_text12(60,18,"�������ٶ�",10,LCD_MODE_INVERT); 
				lcd_update_all();
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
	CounterBack=0;
	pMenuItem=&Menu_0_loggingin;
	pMenuItem->show();

	sel_speedtype_flag=0;
	speedtype=0;



}

ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_0_5_speedtype=
{
"�ٶȻ�ȡ��ʽ",
	12,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};



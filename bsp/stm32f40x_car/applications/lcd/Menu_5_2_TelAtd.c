#include "Menu_Include.h"



u8 tel_screen=0;
static void msg( void *p)
{
}
static void show(void)
{
	lcd_fill(0);
	lcd_text12(36,10,"һ���ز�",8,LCD_MODE_SET);
	lcd_update_all();
}


static void keypress(unsigned int key)
{

	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_5_other;
			pMenuItem->show();
			CounterBack=0;

			tel_screen=0;
			break;
		case KeyValueOk:
			if(tel_screen==0)
				{
				tel_screen=1;
				

				OneKeyCallFlag=1;

				lcd_fill(0);
				lcd_text12(42,10,"�ز���",6,LCD_MODE_SET);
				lcd_update_all();
				//---------  һ������------
				/*OneKeyCallFlag=1;    
				One_largeCounter=0;
				One_smallCounter=0;*/
				  if(DataLink_Status()&&(CallState==CallState_Idle))    //�绰���������������
                           {  
			                 Speak_ON;   //��������
					   rt_kprintf("\r\n  һ���ز�(��������)-->��ͨͨ��\r\n");}
                                      CallState=CallState_rdytoDialLis;  // ׼����ʼ����������� 				
				//-----------------------------
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

}

MYTIME
MENUITEM	Menu_5_2_TelAtd=
{
"һ���ز�",
	8,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};


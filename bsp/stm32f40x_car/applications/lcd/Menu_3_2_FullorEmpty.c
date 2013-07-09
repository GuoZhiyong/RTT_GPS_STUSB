#include "Menu_Include.h"
#include "sed1520.h"
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
	lcd_text12(20+30*Status,19,(char *)car_status_str[Status],4,LCD_MODE_INVERT);
	lcd_update_all();
}
static void msg( void *p)
{
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
		pMenuItem=&Menu_3_InforInteract;
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
			lcd_text12(88,10,(char *)car_status_str[CarStatus_change],4,LCD_MODE_SET);
			lcd_update_all();
			}
		else if(CarStatus_screen==1)
			{
			CarStatus_screen=2;
#if NEED_TODO
				JT808Conf_struct.LOAD_STATE=CarStatus_change;
			Api_Config_Recwrite_Large(jt808,0,(u8*)&JT808Conf_struct,sizeof(JT808Conf_struct));
#endif
		     /* Car_Status[2]&=~0x03;      //  ����
	             if(CarStatus_change==1)
				Car_Status[2]|=0x01;   //����
			else if(CarStatus_change==2)
				Car_Status[2]|=0x03;   //����*/

            //�ϱ�λ����Ϣ
				#if NEED_TODO
			PositionSD_Enable();
			Current_UDP_sd=1;
				#endif

			lcd_fill(0);
			lcd_text12(20,10,(char *)car_status_str[CarStatus_change],4,LCD_MODE_SET);
			lcd_text12(48,10,"���ͳɹ�",8,LCD_MODE_SET);
			lcd_update_all();
			
			CarStatus_change=1;//ѡ��
			CarStatus_screen=0;//�����л�ʹ��
			}
		break;
	case KeyValueUP:
		if(CarStatus_screen==0)
			{			
			if(CarStatus_change<=0)
				CarStatus_change=2;
			else
				CarStatus_change--;
			CarStatus(CarStatus_change);
			}
		break;
	case KeyValueDown:
		if(CarStatus_screen==0)
			{		
			if(CarStatus_change>=2)
				CarStatus_change=0;
			else
				CarStatus_change++;
			
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


MENUITEM	Menu_3_2_FullorEmpty= 
{

    "����״̬����",
    12,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};


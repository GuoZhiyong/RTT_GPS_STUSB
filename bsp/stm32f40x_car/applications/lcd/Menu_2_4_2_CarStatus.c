#include "menu.h"


struct IMG_DEF test_dis_CarStatus={12,12,test_00};

unsigned char CarStatus_change=1;//״̬ѡ��
unsigned char CarStatus_screen=0;//�����л�ʹ��

void CarStatus(unsigned char Status)
{
switch(Status)
	{
	case 1:
		lcd_fill(0);
		DisAddRead_ZK(12,3,"��������״̬ѡ��",8,&test_dis_CarStatus,0,0);
		DisAddRead_ZK(20,19,"�ճ�",2,&test_dis_CarStatus,1,0);
		DisAddRead_ZK(50,19,"���",2,&test_dis_CarStatus,0,0);
		DisAddRead_ZK(80,19,"�س�",2,&test_dis_CarStatus,0,0);
		lcd_update_all();
		break;
	case 2:
		lcd_fill(0);
		DisAddRead_ZK(12,3,"��������״̬ѡ��",8,&test_dis_CarStatus,0,0);
		DisAddRead_ZK(20,19,"�ճ�",2,&test_dis_CarStatus,0,0);
		DisAddRead_ZK(50,19,"���",2,&test_dis_CarStatus,1,0);
		DisAddRead_ZK(80,19,"�س�",2,&test_dis_CarStatus,0,0);
		lcd_update_all();
		break;
	case 3:
		lcd_fill(0);
		DisAddRead_ZK(12,3,"��������״̬ѡ��",8,&test_dis_CarStatus,0,0);
		DisAddRead_ZK(20,19,"�ճ�",2,&test_dis_CarStatus,0,0);
		DisAddRead_ZK(50,19,"���",2,&test_dis_CarStatus,0,0);
		DisAddRead_ZK(80,19,"�س�",2,&test_dis_CarStatus,1,0);
		lcd_update_all();
		break;
	default:
		break;
	}
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
			DisAddRead_ZK(12,10,"���ͳ���״̬",6,&test_dis_CarStatus,0,0);
			if(CarStatus_change==1)
				{
				//CarLoadState_Flag=1;
				//CarLoadState_Write();
				DisAddRead_ZK(88,10,"�ճ�",2,&test_dis_CarStatus,1,0);
				}
			else if(CarStatus_change==2)
				{
				//CarLoadState_Flag=2;
				//CarLoadState_Write();
				DisAddRead_ZK(88,10,"���",2,&test_dis_CarStatus,1,0);
				}
			else if(CarStatus_change==3)
				{
				//CarLoadState_Flag=3; 
				//CarLoadState_Write();
				DisAddRead_ZK(88,10,"�س�",2,&test_dis_CarStatus,1,0);
				}
			lcd_update_all();
			}
		else if(CarStatus_screen==1)
			{
			CarStatus_screen=2;
			lcd_fill(0);
			if(CarStatus_change==1)
				DisAddRead_ZK(20,10,"�ճ�",2,&test_dis_CarStatus,1,0);
			else if(CarStatus_change==2)
				DisAddRead_ZK(20,10,"���",2,&test_dis_CarStatus,1,0);
			else if(CarStatus_change==3)
				DisAddRead_ZK(20,10,"�س�",2,&test_dis_CarStatus,1,0);
			DisAddRead_ZK(48,10,"���ͳɹ�",4,&test_dis_CarStatus,0,0);
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


MENUITEM	Menu_2_4_2_CarStatus=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};


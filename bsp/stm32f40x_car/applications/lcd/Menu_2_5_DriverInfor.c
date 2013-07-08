#include  <string.h>
#include "Menu_Include.h"
#include "Lcd.h"


/*
static unsigned char Jiayuan_screen=0;  //  ��ʾ�鿴/���ͽ���   =1ʱѡ���ǲ鿴���Ƿ��ͽ���
static unsigned char CheckJiayuanFlag=0;//  1:������ʾ��ʻԱ��Ϣ   2:���뷢�ͼ�ʻԱ��Ϣ
static unsigned char Jiayuan_1_2=0;     // 0:��ʾ�ڲ鿴����   1:��ʾ�ڷ��ͽ���
*/
typedef struct _DIS_DIRVER_INFOR
{
unsigned char DIS_SELECT_check_send;
unsigned char DIS_ENTER_check_send;
unsigned char DIS_SHOW_check_send;
}DIS_DIRVER_INFOR;

DIS_DIRVER_INFOR DIS_DRIVER_inform_temp;



//��ʻԱ���� 
void Display_jiayuan(unsigned char NameCode)
{
//unsigned char i=0;
lcd_fill(0);
if(NameCode==1)
	{
	lcd_text12(30,3,"��ʻԱ����",10,LCD_MODE_SET);
	lcd_text12(42,19,(char *)JT808Conf_struct.Driver_Info.DriveName,strlen((char *)JT808Conf_struct.Driver_Info.DriveName),LCD_MODE_SET);
	//lcd_text12(48,19,(char *)Driver_Info.DriveName,strlen(Driver_Info.DriveName),LCD_MODE_SET);
	}
else
	{
	lcd_text12(30,3,"��ʻ֤����",10,LCD_MODE_SET);
	lcd_text12(6,19,(char *)JT808Conf_struct.Driver_Info.DriverCard_ID,18,LCD_MODE_SET);
	//lcd_text12(15,19,(char *)Driver_Info.DriverCard_ID,sizeof(Driver_Info.DriverCard_ID),LCD_MODE_SET);
	}
lcd_update_all();
} 

static void Dis_DriverInfor(unsigned char type,unsigned char disscreen)
{
lcd_fill(0);
if(type==1)
	{
	if(disscreen==1)
		{
		lcd_text12(0, 3,"1.��ʻԱ��Ϣ�鿴",16,LCD_MODE_INVERT); 
	    lcd_text12(0,19,"2.��ʻԱ��Ϣ����",16,LCD_MODE_SET);
		}
	else if(disscreen==2)
		{
		lcd_text12(0, 3,"1.��ʻԱ��Ϣ�鿴",16,LCD_MODE_SET); 
		lcd_text12(0,19,"2.��ʻԱ��Ϣ����",16,LCD_MODE_INVERT);
		}
	}
else if(type==2)
	{
	if(disscreen==1)
		lcd_text12(0,10,"��ȷ�Ϸ��ͼ�ʻԱ��Ϣ",20,LCD_MODE_SET);
	else if(disscreen==2)
		lcd_text12(5,10,"��ʻԱ��Ϣ���ͳɹ�",18,LCD_MODE_SET);
	}	
lcd_update_all();
}

static void msg( void *p)
{
}
static void show(void)
{
	Dis_DriverInfor(1,1);
	DIS_DRIVER_inform_temp.DIS_SELECT_check_send=1;
	DIS_DRIVER_inform_temp.DIS_ENTER_check_send=1;
}


static void keypress(unsigned int key)
{
	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_2_InforCheck;
			pMenuItem->show();
			CounterBack=0;

			memset(&DIS_DRIVER_inform_temp,0,sizeof(DIS_DRIVER_inform_temp));
			break;
		case KeyValueOk:
			if(DIS_DRIVER_inform_temp.DIS_ENTER_check_send==1)
				{
				DIS_DRIVER_inform_temp.DIS_ENTER_check_send=2;
				DIS_DRIVER_inform_temp.DIS_SELECT_check_send=0;//�鿴���߷����Ѿ�ѡ��
				
				if(DIS_DRIVER_inform_temp.DIS_SHOW_check_send==0)//����鿴��ʻԱ��Ϣ����
					Display_jiayuan(1);
				else if(DIS_DRIVER_inform_temp.DIS_SHOW_check_send==1)//���뷢�ͼ�ʻԱ��Ϣ����
					{
					Dis_DriverInfor(2,1);
					}
				}
			else if(DIS_DRIVER_inform_temp.DIS_ENTER_check_send==2)
				{
				DIS_DRIVER_inform_temp.DIS_ENTER_check_send=3;
				if(DIS_DRIVER_inform_temp.DIS_SHOW_check_send==0)//���ز鿴�ͷ��ͽ���
					{
					Dis_DriverInfor(1,1);
					DIS_DRIVER_inform_temp.DIS_SELECT_check_send=1;
					DIS_DRIVER_inform_temp.DIS_ENTER_check_send=1;
					}
				else if(DIS_DRIVER_inform_temp.DIS_SHOW_check_send==1)//��ʾ���ͳɹ�
					{
					Dis_DriverInfor(2,2);
					//SD_ACKflag.f_DriverInfoSD_0702H=1;
					DIS_DRIVER_inform_temp.DIS_ENTER_check_send=0;//    1
					DIS_DRIVER_inform_temp.DIS_SELECT_check_send=0;
					DIS_DRIVER_inform_temp.DIS_SHOW_check_send=0;
					}
				}
			break;
		case KeyValueUP:
			if(DIS_DRIVER_inform_temp.DIS_ENTER_check_send==2)
				{
				if(DIS_DRIVER_inform_temp.DIS_SHOW_check_send==0)//�鿴
					Display_jiayuan(1);
				else if(DIS_DRIVER_inform_temp.DIS_SHOW_check_send==1)//����
					{
					Dis_DriverInfor(2,1);
					}
				}
			else if(DIS_DRIVER_inform_temp.DIS_SELECT_check_send==1)//ѡ�����鿴���߷���
				{
				DIS_DRIVER_inform_temp.DIS_SHOW_check_send=0;
				Dis_DriverInfor(1,1);
				}
			break;
		case KeyValueDown:
			if(DIS_DRIVER_inform_temp.DIS_ENTER_check_send==2)
				{
				if(DIS_DRIVER_inform_temp.DIS_SHOW_check_send==0)//�鿴
					Display_jiayuan(2);
				else if(DIS_DRIVER_inform_temp.DIS_SHOW_check_send==1)//����
					Dis_DriverInfor(2,1);
				}
			else if(DIS_DRIVER_inform_temp.DIS_SELECT_check_send==1)//ѡ�����鿴���߷���
				{
				DIS_DRIVER_inform_temp.DIS_SHOW_check_send=1;
				Dis_DriverInfor(1,2);
				
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
    memset(&DIS_DRIVER_inform_temp,0,sizeof(DIS_DRIVER_inform_temp));
}


MENUITEM	Menu_2_5_DriverInfor=
{	
	"��ʻԱ��Ϣ�鿴",
    14,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};



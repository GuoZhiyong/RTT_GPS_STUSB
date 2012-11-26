#include "menu.h"

struct IMG_DEF test_dis_Record={12,12,test_00};


unsigned char Record_screen=0;//����¼������=1,׼�����Ϳ�ʼ/����ʱΪ2   ��ʼ/�����ѷ���Ϊ3    ����һ����ָ�����ֵ
unsigned char Record_StartEnd=0;//==1¼����ʼ   ==2¼������   ����һ����ָ�����ֵ


static void show(void)
{
	lcd_fill(0);
	DisAddRead_ZK(36,3,"¼��ѡ��",4,&test_dis_Record,0,0);
	DisAddRead_ZK(24,19,"��ʼ",2,&test_dis_Record,1,0);
	DisAddRead_ZK(72,19,"����",2,&test_dis_Record,0,0);				  
    lcd_update_all();
	Record_screen=1;
	Record_StartEnd=1;
}


static void keypress(unsigned int key)
{
switch(KeyValue)
	{
	case KeyValueMenu:
		pMenuItem=&Menu_1_InforInteract;
		pMenuItem->show();
		CounterBack=0;
		    
		Record_screen=0;//����¼������=1,׼�����Ϳ�ʼ/����ʱΪ2	 ��ʼ/�����ѷ���Ϊ3    ����һ����ָ�����ֵ
		Record_StartEnd=0;//==1¼����ʼ	 ==2¼������   ����һ����ָ�����ֵ
		break;
	case KeyValueOk:
		if(Record_screen==1)
			{
			Record_screen=2;
			if(Record_StartEnd==1)
				{
				lcd_fill(0);
				DisAddRead_ZK(24,3,"����¼��ѡ��",6,&test_dis_Record,0,0);
				DisAddRead_ZK(48,19,"��ʼ",2,&test_dis_Record,1,0);
				lcd_update_all();
				}
			else if(Record_StartEnd==2)
				{
				lcd_fill(0);
				DisAddRead_ZK(24,3,"����¼��ѡ��",6,&test_dis_Record,0,0);
				DisAddRead_ZK(48,19,"����",2,&test_dis_Record,1,0);
				lcd_update_all();
				}
			}
		else if(Record_screen==2)
			{
			Record_screen=3;
			if(Record_StartEnd==1)
				{
				lcd_fill(0);
				DisAddRead_ZK(18,10,"��ʼ¼��",4,&test_dis_Record,1,0);
				DisAddRead_ZK(66,10,"�ѷ���",3,&test_dis_Record,0,0);
				lcd_update_all();
				}
			else if(Record_StartEnd==2)
				{
				lcd_fill(0);
				DisAddRead_ZK(18,10,"����¼��",4,&test_dis_Record,1,0);
				DisAddRead_ZK(66,10,"�ѷ���",3,&test_dis_Record,0,0);
				lcd_update_all();
				}
			}
		else if(Record_screen==3)//�ص�¼����ʼ/¼����������
			{
			Record_screen=1;
			Record_StartEnd=1;

			lcd_fill(0);
        	DisAddRead_ZK(36,3,"¼��ѡ��",4,&test_dis_Record,0,0);
			DisAddRead_ZK(24,19,"��ʼ",2,&test_dis_Record,1,0);
			DisAddRead_ZK(72,19,"����",2,&test_dis_Record,0,0);
			lcd_update_all();
			}

		break;
	case KeyValueUP:
        if(Record_screen==1)
        	{
        	Record_StartEnd=1;
			lcd_fill(0);
        	DisAddRead_ZK(36,3,"¼��ѡ��",4,&test_dis_Record,0,0);
			DisAddRead_ZK(24,19,"��ʼ",2,&test_dis_Record,1,0);
			DisAddRead_ZK(72,19,"����",2,&test_dis_Record,0,0);
			lcd_update_all();
        	}
		break;
	case KeyValueDown:
         if(Record_screen==1)
        	{
        	Record_StartEnd=2;
			lcd_fill(0);
        	DisAddRead_ZK(36,3,"¼��ѡ��",4,&test_dis_Record,0,0);
			DisAddRead_ZK(24,19,"��ʼ",2,&test_dis_Record,0,0);
			DisAddRead_ZK(72,19,"����",2,&test_dis_Record,1,0);
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
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();
	CounterBack=0;

}


MENUITEM	Menu_2_4_5_Record=
{
	"",
	&show,
	&keypress,
	&timetick,
	(void*)0
};


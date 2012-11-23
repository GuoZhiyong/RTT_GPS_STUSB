#include "menu.h"


struct IMG_DEF test_1_MeunSet={12,12,test_00};

unsigned char noselect_set[]={0x3C,0x7E,0xC3,0xC3,0xC3,0xC3,0x7E,0x3C};//����
unsigned char select_set[]={0x3C,0x7E,0xFF,0xFF,0xFF,0xFF,0x7E,0x3C};//ʵ��


unsigned char CarSetType=0,CarSetCounter=0;

DECL_BMP(8,8,select_set); DECL_BMP(8,8,noselect_set); 

void CarSet(unsigned char set_type)
{
unsigned char i=0;
switch(set_type)
	{
	case 1:
		lcd_fill(0);
		DisAddRead_ZK(0,3,"����",2,&test_1_MeunSet,0,0);
		DisAddRead_ZK(0,17,"����",2,&test_1_MeunSet,0,0);
		lcd_bitmap(35, 5, &BMP_select_set, LCD_MODE_SET);
		for(i=0;i<5;i++)
			lcd_bitmap(47+i*12, 5, &BMP_noselect_set, LCD_MODE_SET);
		DisAddRead_ZK(35,19,"���ƺ�����",5,&test_1_MeunSet,1,0);
		lcd_update_all();
		break;
	case 2:
		lcd_fill(0);
		DisAddRead_ZK(0,3,"����",2,&test_1_MeunSet,0,0);
		DisAddRead_ZK(0,17,"����",2,&test_1_MeunSet,0,0);
		lcd_bitmap(35, 5, &BMP_noselect_set, LCD_MODE_SET);
		lcd_bitmap(47, 5, &BMP_select_set, LCD_MODE_SET);
		for(i=0;i<4;i++)
			lcd_bitmap(59+i*12, 5, &BMP_noselect_set, LCD_MODE_SET);
		DisAddRead_ZK(35,19,"��������ѡ��",6,&test_1_MeunSet,1,0);
		lcd_update_all();
		break;
	case 3:
		lcd_fill(0);
		DisAddRead_ZK(0,3,"����",2,&test_1_MeunSet,0,0);
		DisAddRead_ZK(0,17,"����",2,&test_1_MeunSet,0,0);
		for(i=0;i<2;i++)
			lcd_bitmap(35+i*12, 5, &BMP_noselect_set, LCD_MODE_SET);
		lcd_bitmap(59, 5, &BMP_select_set, LCD_MODE_SET);

		for(i=0;i<3;i++)
			lcd_bitmap(71+i*12, 5, &BMP_noselect_set, LCD_MODE_SET);
		DisAddRead_ZK(35,19,"�ٶ�����",4,&test_1_MeunSet,1,0);
		lcd_update_all();
		break;
	case 4:
		lcd_fill(0);
		DisAddRead_ZK(0,3,"����",2,&test_1_MeunSet,0,0);
		DisAddRead_ZK(0,17,"����",2,&test_1_MeunSet,0,0);
		for(i=0;i<3;i++)
			lcd_bitmap(35+i*12, 5, &BMP_noselect_set, LCD_MODE_SET);
		lcd_bitmap(71, 5, &BMP_select_set, LCD_MODE_SET);
		for(i=0;i<2;i++)
			lcd_bitmap(83+i*12, 5, &BMP_noselect_set, LCD_MODE_SET);

		DisAddRead_ZK(35,19,"����    ����",6,&test_1_MeunSet,1,0);
		lcd_text(60,21,FONT_SEVEN_DOT,"VIN");
		lcd_update_all();
		break;
	case 5:
		lcd_fill(0);
		DisAddRead_ZK(0,3,"����",2,&test_1_MeunSet,0,0);
		DisAddRead_ZK(0,17,"����",2,&test_1_MeunSet,0,0);
		for(i=0;i<4;i++)
			lcd_bitmap(35+i*12, 5, &BMP_noselect_set, LCD_MODE_SET);
		lcd_bitmap(83, 5, &BMP_select_set, LCD_MODE_SET);
        lcd_bitmap(95, 5, &BMP_noselect_set, LCD_MODE_SET);
		DisAddRead_ZK(30,19,"���Υ�¼�¼",6,&test_1_MeunSet,1,0);  
		lcd_update_all();
		break;
	case 6:
		lcd_fill(0);
		DisAddRead_ZK(0,3,"����",2,&test_1_MeunSet,0,0);
		DisAddRead_ZK(0,17,"����",2,&test_1_MeunSet,0,0);
		for(i=0;i<5;i++)
			lcd_bitmap(35+i*12, 5, &BMP_noselect_set, LCD_MODE_SET);
		lcd_bitmap(95, 5, &BMP_select_set, LCD_MODE_SET);

		DisAddRead_ZK(36,19,"�ͺı궨",4,&test_1_MeunSet,1,0);  
		lcd_update_all();
		break;
	default	 :
		break;
	}
}

static void show(void)
{
	CarSet(1);
	CarSetType=1;
	CarSetCounter=1;
}

/*
PMENUITEM[6]=
{
	&Menu_2_2_1_license;//���ƺ�����
	&Menu_2_2_2_CarType;//��������ѡ��
	&Menu_2_2_3_SpeedSet;//�ٶ�����
	&Menu_2_2_4_VINset;//VIN����
	&Menu_2_2_5_Cancel;//VIN����
	&Menu_1_Idle;//�����궨
}
*/

static void keypress(unsigned int key)
{
switch(KeyValue)
	{
	case KeyValueMenu:
		CounterBack=0;
		pMenuItem=&Menu_1_Idle;//������Ϣ�鿴����
		pMenuItem->show();
		break;
	case KeyValueOk:
		if(CarSetCounter==1)
			{
			pMenuItem=&Menu_2_2_1_license;//���ƺ�����
		    pMenuItem->show();
			}
		else if(CarSetCounter==2)
			{
			pMenuItem=&Menu_2_2_2_CarType;//��������ѡ��
		    pMenuItem->show();
			}
		else if(CarSetCounter==3)
			{
			pMenuItem=&Menu_2_2_3_SpeedSet;//�ٶ�����
		    pMenuItem->show();
			}
		else if(CarSetCounter==4)
			{
			pMenuItem=&Menu_2_2_4_VINset;//VIN����
		    pMenuItem->show();
			}
		else if(CarSetCounter==5)
			{
			pMenuItem=&Menu_2_2_5_Cancel;//VIN����
		    pMenuItem->show();
			}
		else if(CarSetCounter==6)
			{
			pMenuItem=&Menu_1_Idle;//�����궨
		    pMenuItem->show();
			}
		
		CarSetType=0;
		CarSetCounter=0;
		break;
	case KeyValueUP:
		if(CarSetType==1)
			{
			CarSetCounter--;
			if(CarSetCounter<1)
				CarSetCounter=6;
			CarSet(CarSetCounter);
			}
		break;
	case KeyValueDown:
		if(CarSetType==1)
			{
			CarSetCounter++;
			if(CarSetCounter>6)
				CarSetCounter=1;
			CarSet(CarSetCounter);
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


MENUITEM	Menu_1_CarSet=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};


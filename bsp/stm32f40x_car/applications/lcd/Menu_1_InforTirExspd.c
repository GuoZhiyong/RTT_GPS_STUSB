#include "menu.h"


struct IMG_DEF test_1_TirExspd={12,12,test_00};

unsigned char InforTirExspdFlag=0,InforTirExspdCounter=0;

unsigned char noselect_TirExspd[]={0x3C,0x7E,0xC3,0xC3,0xC3,0xC3,0x7E,0x3C};//¿ÕÐÄ
unsigned char select_TirExspd[]={0x3C,0x7E,0xFF,0xFF,0xFF,0xFF,0x7E,0x3C};//ÊµÐÄ

DECL_BMP(8,8,select_TirExspd); DECL_BMP(8,8,noselect_TirExspd); 

void InforTirExspd(unsigned char infor_type)
{
switch(infor_type)
	{
	case 1:
		lcd_fill(0);
		DisAddRead_ZK(0,3,"Î¥¹æ",2,&test_1_TirExspd,0,0);
		DisAddRead_ZK(0,17,"¼ÝÊ»",2,&test_1_TirExspd,0,0);
		lcd_bitmap(35, 5, &BMP_select_TirExspd, LCD_MODE_SET);
		lcd_bitmap(47, 5, &BMP_noselect_TirExspd, LCD_MODE_SET);
		DisAddRead_ZK(35,19,"Æ£ÀÍ¼ÝÊ»",4,&test_1_TirExspd,1,0);
		lcd_update_all();
		break;
	case 2:
		lcd_fill(0);
		DisAddRead_ZK(0,3,"Î¥¹æ",2,&test_1_TirExspd,0,0);
		DisAddRead_ZK(0,17,"¼ÝÊ»",2,&test_1_TirExspd,0,0);
		lcd_bitmap(35, 5, &BMP_noselect_TirExspd, LCD_MODE_SET);
		lcd_bitmap(47, 5, &BMP_select_TirExspd, LCD_MODE_SET);
		DisAddRead_ZK(35,19,"³¬ËÙ¼ÝÊ»",4,&test_1_TirExspd,1,0);
		lcd_update_all();
		break;
	default:
		break ;
	}
}

static void show(void)
{
	InforTirExspd(1);
	InforTirExspdFlag=1;
	InforTirExspdCounter=1;
}


static void keypress(unsigned int key)
{
switch(KeyValue)
	{
	case KeyValueMenu:
		CounterBack=0;

		pMenuItem=&Menu_1_usb;
		pMenuItem->show();
		break;
	case KeyValueOk:
		if(InforTirExspdCounter==1)
			{
			pMenuItem=&Menu_2_5_1_pilao;//Æ£ÀÍ¼ÝÊ»
		    pMenuItem->show();
			}
		else if(InforTirExspdCounter==2)
			{
			pMenuItem=&Menu_2_5_2_chaosu;//³¬ËÙ¼ÝÊ»
		    pMenuItem->show();
			}

		InforTirExspdFlag=0;
		InforTirExspdCounter=0;
		break;
	case KeyValueUP:
		if(InforTirExspdFlag==1)
			{
			InforTirExspdCounter--;
			if(InforTirExspdCounter<1)
				InforTirExspdCounter=2;
			InforTirExspd(InforTirExspdCounter);
			}
		break;
	case KeyValueDown:
		if(InforTirExspdFlag==1)
			{
			InforTirExspdCounter++;
			if(InforTirExspdCounter>2)
				InforTirExspdCounter=1;
			InforTirExspd(InforTirExspdCounter);
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


MENUITEM	Menu_1_InforTirExspd=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};


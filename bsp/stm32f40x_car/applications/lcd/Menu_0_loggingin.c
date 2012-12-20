#include "Menu_Include.h"

unsigned char noselect_log[]={0x3C,0x7E,0xC3,0xC3,0xC3,0xC3,0x7E,0x3C};//����
unsigned char select_log[]={0x3C,0x7E,0xFF,0xFF,0xFF,0xFF,0x7E,0x3C};//ʵ��

unsigned char CarSet_0=1;
unsigned char CarSet_0_Flag=1,CarSet_0_counter=1;

DECL_BMP(8,8,select_log); DECL_BMP(8,8,noselect_log); 

void Selec_123(u8 par)
{
u8 i=0;
if(par==1)
	{
	lcd_bitmap(35, 5, &BMP_select_log, LCD_MODE_SET);
	for(i=0;i<2;i++)
		lcd_bitmap(47+i*12, 5, &BMP_noselect_log ,LCD_MODE_SET);
	}
else if(par==2)
	{
	lcd_bitmap(35, 5, &BMP_noselect_log, LCD_MODE_SET);
	lcd_bitmap(47, 5, &BMP_select_log, LCD_MODE_SET);
	lcd_bitmap(59, 5, &BMP_noselect_log, LCD_MODE_SET);
	}
else if(par==3)
	{
	for(i=0;i<2;i++)
		lcd_bitmap(35+i*12, 5, &BMP_noselect_log, LCD_MODE_SET);
	lcd_bitmap(59, 5, &BMP_select_log, LCD_MODE_SET);
	}
}
void CarSet_0_fun(u8 set_type)
{
//u8 i=0;
switch(set_type)
	{
	case 1:
		lcd_fill(0);
		lcd_text12( 0, 3,"ע��",4,LCD_MODE_SET);
		lcd_text12( 0,17,"����",4,LCD_MODE_SET);
		Selec_123(CarSet_0_counter);
		lcd_text12(35,19,"���ƺ�����",10,LCD_MODE_INVERT);
		lcd_update_all();
		break;
	case 2:
		lcd_fill(0);
		lcd_text12( 0, 3,"ע��",4,LCD_MODE_SET);
		lcd_text12( 0,17,"����",4,LCD_MODE_SET);	
        Selec_123(CarSet_0_counter);
		lcd_text12(35,19,"����ID����",10,LCD_MODE_INVERT);		
		lcd_update_all();
		break;
	case 3:
		lcd_fill(0);
		lcd_text12( 0, 3,"ע��",4,LCD_MODE_SET);
		lcd_text12( 0,17,"����",4,LCD_MODE_SET);
        Selec_123(CarSet_0_counter);
		lcd_text12(35,19,"��ɫ����",8,LCD_MODE_INVERT);
		lcd_update_all();
		break;
	default	 :
		break;
	}
}

static void show(void)
{
 CarSet_0_counter=1;
 CarSet_0_fun(CarSet_0_counter);
}


static void keypress(unsigned int key)
{
switch(KeyValue)
	{
	case KeyValueMenu:
		if(Set0_Comp_Flag>=2)
			{
			Set0_Comp_Flag=0;
			pMenuItem=&Menu_1_Idle;//������Ϣ�鿴����
			pMenuItem->show();
			}
		break;
	case KeyValueOk:
		if(CarSet_0_counter==1)
          	{
			pMenuItem=&Menu_2_2_1_license;//���ƺ�����
	        pMenuItem->show();
          	}
		else if(CarSet_0_counter==2)
			{
			pMenuItem=&Menu_2_2_5_CarSim;//sim
		    pMenuItem->show();
			}
		else if(CarSet_0_counter==3)
			{
			pMenuItem=&Menu_2_2_6_Carcol;//��ɫ
		    pMenuItem->show();
			}
        if(Set0_Comp_Flag==3)
	        {
			Set0_Comp_Flag=4;
			rt_kprintf("�������\r\n");
			pMenuItem=&Menu_2_6_1_BDupdata;//���ƺ�����
		    pMenuItem->show();
	        }
		break;
	case KeyValueUP:
        if(CarSet_0_Flag==1)
          	{
          	CarSet_0_counter--;
          	if(CarSet_0_counter<1)
              CarSet_0_counter=3;
		    CarSet_0_fun(CarSet_0_counter);
          	}
		break;
	case KeyValueDown:
		if(CarSet_0_Flag==1)
          	{
          	CarSet_0_counter++;
          	if(CarSet_0_counter>3)
              CarSet_0_counter=1;
	        CarSet_0_fun(CarSet_0_counter);
          	}         
		break;
	}
KeyValue=0;
}


static void timetick(unsigned int systick)
{
}


MENUITEM	Menu_0_loggingin=
{
   "��������",
	8,
	&show,
	&keypress,
	&timetick,
	(void*)0
};


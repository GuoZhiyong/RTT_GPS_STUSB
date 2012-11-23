#include "menu.h"


struct IMG_DEF test_1_Interact={12,12,test_00};

unsigned char InforInterType=0,InforInterCounter=0;

unsigned char noselect_inter[]={0x3C,0x7E,0xC3,0xC3,0xC3,0xC3,0x7E,0x3C};//空心
unsigned char select_inter[]={0x3C,0x7E,0xFF,0xFF,0xFF,0xFF,0x7E,0x3C};//实心


DECL_BMP(8,8,select_inter); DECL_BMP(8,8,noselect_inter); 


void InforInteract(unsigned char infor_type)
{
unsigned char i=0;

	switch(infor_type)
		{
		case 1:
			lcd_fill(0);
			DisAddRead_ZK(0,3,"信息",2,&test_1_Interact,0,0);
			DisAddRead_ZK(0,17,"交互",2,&test_1_Interact,0,0);
			lcd_bitmap(35, 5, &BMP_select_inter, LCD_MODE_SET);
			for(i=0;i<6;i++)
				lcd_bitmap(47+i*12, 5, &BMP_noselect_inter, LCD_MODE_SET);
			DisAddRead_ZK(35,19,"中心提问下发",6,&test_1_Interact,1,0);
			lcd_update_all();
			break;
		case 2:
			lcd_fill(0);
			DisAddRead_ZK(0,3,"信息",2,&test_1_Interact,0,0);
			DisAddRead_ZK(0,17,"交互",2,&test_1_Interact,0,0);
			lcd_bitmap(35, 5, &BMP_noselect_inter, LCD_MODE_SET);
			lcd_bitmap(47, 5, &BMP_select_inter, LCD_MODE_SET);
			for(i=0;i<5;i++)
				lcd_bitmap(59+i*12, 5, &BMP_noselect_inter, LCD_MODE_SET);
		
			DisAddRead_ZK(35,19,"车辆负载状态",6,&test_1_Interact,1,0);
			lcd_update_all();
			break;
		case 3:
			lcd_fill(0);
			DisAddRead_ZK(0,3,"信息",2,&test_1_Interact,0,0);
			DisAddRead_ZK(0,17,"交互",2,&test_1_Interact,0,0);
			for(i=0;i<2;i++)
				lcd_bitmap(35+i*12, 5, &BMP_noselect_inter, LCD_MODE_SET);
			lcd_bitmap(59, 5, &BMP_select_inter, LCD_MODE_SET);
			for(i=0;i<4;i++)
				lcd_bitmap(71+i*12, 5, &BMP_noselect_inter, LCD_MODE_SET);
			DisAddRead_ZK(35,19,"电子运单发送",6,&test_1_Interact,1,0);
			lcd_update_all();
			break;
		case 4:
			lcd_fill(0);
			DisAddRead_ZK(0,3,"信息",2,&test_1_Interact,0,0);
			DisAddRead_ZK(0,17,"交互",2,&test_1_Interact,0,0);
			for(i=0;i<3;i++)
				lcd_bitmap(35+i*12, 5, &BMP_noselect_inter, LCD_MODE_SET);
			lcd_bitmap(71, 5, &BMP_select_inter, LCD_MODE_SET);
			for(i=0;i<3;i++)
				lcd_bitmap(83+i*12, 5, &BMP_noselect_inter, LCD_MODE_SET);
			DisAddRead_ZK(35,19,"多媒体信息选择",7,&test_1_Interact,1,0);
			lcd_update_all();
			break;
		case 5:
			lcd_fill(0);
			DisAddRead_ZK(0,3,"信息",2,&test_1_Interact,0,0);
			DisAddRead_ZK(0,17,"交互",2,&test_1_Interact,0,0);
			for(i=0;i<4;i++)
				lcd_bitmap(35+i*12, 5, &BMP_noselect_inter, LCD_MODE_SET);
			lcd_bitmap(83, 5, &BMP_select_inter, LCD_MODE_SET);
			for(i=0;i<2;i++)
				lcd_bitmap(95+i*12, 5, &BMP_noselect_inter, LCD_MODE_SET);
			DisAddRead_ZK(35,19,"录音选择",4,&test_1_Interact,1,0);
			lcd_update_all();
			break;
		case 6:
			lcd_fill(0);
			DisAddRead_ZK(0,3,"信息",2,&test_1_Interact,0,0);
			DisAddRead_ZK(0,17,"交互",2,&test_1_Interact,0,0);
			for(i=0;i<5;i++)
				lcd_bitmap(35+i*12, 5, &BMP_noselect_inter, LCD_MODE_SET);
			lcd_bitmap(95, 5, &BMP_select_inter, LCD_MODE_SET);
			lcd_bitmap(107, 5, &BMP_noselect_inter, LCD_MODE_SET);
			DisAddRead_ZK(35,19,"事件交互",4,&test_1_Interact,1,0);
			lcd_update_all();
			break;
		case 7:
			lcd_fill(0);
			DisAddRead_ZK(0,3,"信息",2,&test_1_Interact,0,0);
			DisAddRead_ZK(0,17,"交互",2,&test_1_Interact,0,0);
			for(i=0;i<6;i++)
				lcd_bitmap(35+i*12, 5, &BMP_noselect_inter, LCD_MODE_SET);
			lcd_bitmap(107, 5, &BMP_select_inter, LCD_MODE_SET);
			DisAddRead_ZK(35,19,"车台鉴权注册",6,&test_1_Interact,1,0);
			lcd_update_all();
			break;
		default:
			break ;
		}
}



static void show(void)
{
	InforInteract(1);
	InforInterCounter=1;
	InforInterType=1;
}



static void keypress(unsigned int key)
{
switch(KeyValue)
	{
	case KeyValueMenu:
		CounterBack=0;
		pMenuItem=&Menu_1_InforTirExspd;//
		pMenuItem->show();
		break;
	case KeyValueOk:
		if(InforInterCounter==1)
			{
			pMenuItem=&Menu_2_4_1_CenterQuestion;//中心提问消息
		    pMenuItem->show();
			}
		else if(InforInterCounter==2)
			{
			pMenuItem=&Menu_2_4_2_CarStatus;// 车辆负载
		    pMenuItem->show();
			}
		else if(InforInterCounter==3)
			{
			pMenuItem=&Menu_2_4_3_CarEleInfor;//电子运单
		    pMenuItem->show();
			}
		else if(InforInterCounter==4)
			{
			pMenuItem=&Menu_2_4_4_Multimedia;//多媒体信息选择
		    pMenuItem->show();
			}
		else if(InforInterCounter==5)
			{
			pMenuItem=&Menu_2_4_5_Record;//录音
		    pMenuItem->show();
			}
		else if(InforInterCounter==6)
			{
			pMenuItem=&Menu_2_4_6_CenterAffairSet;//事件交互
		    pMenuItem->show();
			}
		else if(InforInterCounter==7)
			{
			pMenuItem=&Menu_2_4_7_LogOut;//鉴权注册
		    pMenuItem->show();
			}

		InforInterType=0;
		InforInterCounter=0;
		break;
	case KeyValueUP:
		if(InforInterType==1)
			{
			InforInterCounter--;
			if(InforInterCounter<1)
				InforInterCounter=7;
			InforInteract(InforInterCounter);
			}
		break;
	case KeyValueDown:
		if(InforInterType==1)
			{
			InforInterCounter++;
			if(InforInterCounter>7)
				InforInterCounter=1;
			InforInteract(InforInterCounter);
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


MENUITEM	Menu_1_InforInteract=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};



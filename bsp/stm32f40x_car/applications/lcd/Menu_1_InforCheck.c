#include "menu.h"


struct IMG_DEF test_1_Check={12,12,test_00};

unsigned char InforCheckType=0,InforCheckCounter=0;

unsigned char noselect_check[]={0x3C,0x7E,0xC3,0xC3,0xC3,0xC3,0x7E,0x3C};//空心
unsigned char select_check[]={0x3C,0x7E,0xFF,0xFF,0xFF,0xFF,0x7E,0x3C};//实心


DECL_BMP(8,8,select_check); DECL_BMP(8,8,noselect_check); 


void InforCheck(unsigned char infor_type)
{
unsigned char i=0;
switch(infor_type)
	{
	case 1:
		lcd_fill(0);
		DisAddRead_ZK(0,3,"信息",2,&test_1_Check,0,0);
		DisAddRead_ZK(0,17,"查看",2,&test_1_Check,0,0);
		lcd_bitmap(35,5, &BMP_select_check, LCD_MODE_SET);
		for(i=0;i<6;i++)
			lcd_bitmap(47+i*12, 5, &BMP_noselect_check, LCD_MODE_SET);
		DisAddRead_ZK(35,19,"信号线状态",5,&test_1_Check,1,0);
		lcd_update_all();
		break;
	case 2:
		lcd_fill(0);
		DisAddRead_ZK(0,3,"信息",2,&test_1_Check,0,0);
		DisAddRead_ZK(0,17,"查看",2,&test_1_Check,0,0);
		lcd_bitmap(35, 5, &BMP_noselect_check, LCD_MODE_SET);
		lcd_bitmap(47, 5, &BMP_select_check, LCD_MODE_SET);
		for(i=0;i<5;i++)
			lcd_bitmap(59+i*12, 5, &BMP_noselect_check, LCD_MODE_SET);	
		DisAddRead_ZK(35,19,"最近  分钟速度",7,&test_1_Check,1,0);
		lcd_text(59,21,FONT_SEVEN_DOT,"15");
		lcd_update_all();
		break;
	case 3:
		lcd_fill(0);
		DisAddRead_ZK(0,3,"信息",2,&test_1_Check,0,0);
		DisAddRead_ZK(0,17,"查看",2,&test_1_Check,0,0);
		for(i=0;i<2;i++)
			lcd_bitmap(35+i*12, 5, &BMP_noselect_check, LCD_MODE_SET);
		lcd_bitmap(59, 5, &BMP_select_check, LCD_MODE_SET);
		for(i=0;i<4;i++)
			lcd_bitmap(71+i*12, 5, &BMP_noselect_check, LCD_MODE_SET);
		DisAddRead_ZK(35,19,"文字消息查看",6,&test_1_Check,1,0);
		lcd_update_all();
		break;
	case 4:
		lcd_fill(0);
		DisAddRead_ZK(0,3,"信息",2,&test_1_Check,0,0);
		DisAddRead_ZK(0,17,"查看",2,&test_1_Check,0,0);
		for(i=0;i<3;i++)
			lcd_bitmap(35+i*12, 5, &BMP_noselect_check, LCD_MODE_SET);
		lcd_bitmap(71, 5, &BMP_select_check, LCD_MODE_SET);
		for(i=0;i<3;i++)
			lcd_bitmap(83+i*12, 5, &BMP_noselect_check, LCD_MODE_SET);
		DisAddRead_ZK(35,19,"车辆信息查看",6,&test_1_Check,1,0);
		lcd_update_all();
		break;
	case 5:
		lcd_fill(0);
		DisAddRead_ZK(0,3,"信息",2,&test_1_Check,0,0);
		DisAddRead_ZK(0,17,"查看",2,&test_1_Check,0,0);
		for(i=0;i<4;i++)
			lcd_bitmap(35+i*12, 5, &BMP_noselect_check, LCD_MODE_SET);
		lcd_bitmap(83, 5, &BMP_select_check, LCD_MODE_SET);
		for(i=0;i<2;i++)
			lcd_bitmap(95+i*12, 5, &BMP_noselect_check, LCD_MODE_SET);
		DisAddRead_ZK(35,19,"驾驶员信息查看",7,&test_1_Check,1,0);
		lcd_update_all();
		break;
	case 6:
		lcd_fill(0);
		DisAddRead_ZK(0,3,"信息",2,&test_1_Check,0,0);
		DisAddRead_ZK(0,17,"查看",2,&test_1_Check,0,0);
		for(i=0;i<5;i++)
			lcd_bitmap(35+i*12, 5, &BMP_noselect_check, LCD_MODE_SET);
		lcd_bitmap(95, 5, &BMP_select_check, LCD_MODE_SET);
		lcd_bitmap(107, 5, &BMP_noselect_check, LCD_MODE_SET);
		DisAddRead_ZK(35,19,"里程信息查看",6,&test_1_Check,1,0);
		lcd_update_all();
		break;
	case 7:
		lcd_fill(0);
		DisAddRead_ZK(0,3,"信息",2,&test_1_Check,0,0);
		DisAddRead_ZK(0,17,"查看",2,&test_1_Check,0,0);
		for(i=0;i<6;i++)
			lcd_bitmap(35+i*12, 5, &BMP_noselect_check, LCD_MODE_SET);
		lcd_bitmap(107, 5, &BMP_select_check, LCD_MODE_SET);
		DisAddRead_ZK(35,19,"信息点播查看",6,&test_1_Check,1,0);
		lcd_update_all();
		break;
	default:
		break ;
	}
}


static void show(void)
{
	InforCheck(1);
	
	InforCheckType=1;
	InforCheckCounter=1;
}


static void keypress(unsigned int key)
{
switch(KeyValue)
	{
	case KeyValueMenu:
		CounterBack=0;

		pMenuItem=&Menu_1_InforInteract;//scr_CarMulTrans;
		pMenuItem->show();
		break;
	case KeyValueOk:
		if(InforCheckCounter==1)
			{
			pMenuItem=&Menu_2_3_1_Sing8;//信号线
		    pMenuItem->show();
			}
		else if(InforCheckCounter==2)
			{
			pMenuItem=&Menu_2_3_2_sudu;//15speed
		    pMenuItem->show();
			}
		else if(InforCheckCounter==3)
			{
			pMenuItem=&Menu_2_3_3_TextInforStor;//文本消息(消息1-消息8)
		    pMenuItem->show();
			}
		else if(InforCheckCounter==4)
			{
			pMenuItem=&Menu_2_3_4_carinfor;//车辆信息
		    pMenuItem->show();
			}
		else if(InforCheckCounter==5)
			{
			pMenuItem=&Menu_2_3_5_jiayuan;//驾驶员信息
		    pMenuItem->show();
			}
		else if(InforCheckCounter==6)
			{
			pMenuItem=&Menu_2_3_6_Mileage;//里程信息
		    pMenuItem->show();
			}
		else if(InforCheckCounter==7)
			{
			pMenuItem=&Menu_2_3_7_CenterInforMeun;//中心信息点播
		    pMenuItem->show();
			}

		InforCheckType=0;
		InforCheckCounter=0;
		break;
	case KeyValueUP:
		if(InforCheckType==1)
			{
			InforCheckCounter--;
			if(InforCheckCounter<1)
				InforCheckCounter=7;
			InforCheck(InforCheckCounter);
			}
		break;
	case KeyValueDown:
		if(InforCheckType==1)
			{
			InforCheckCounter++;
			if(InforCheckCounter>7)
				InforCheckCounter=1;
			InforCheck(InforCheckCounter);
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


MENUITEM	Menu_1_InforCheck=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};


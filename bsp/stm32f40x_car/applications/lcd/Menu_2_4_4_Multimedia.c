#include "menu.h"


struct IMG_DEF test_dis_Multimedia={12,12,test_00};

unsigned char Menu_Multimedia=0;
unsigned char Multimedia_change=1;//选择
unsigned char Multimedia_screen=0;//界面切换使用


void Multimedia(unsigned char type)
{
switch(type)
	{
	case 1:
		lcd_fill(0);
		DisAddRead_ZK(18,3,"多媒体信息选择",7,&test_dis_Multimedia,0,0);
		DisAddRead_ZK(24,19,"音频",2,&test_dis_Multimedia,1,0);
		DisAddRead_ZK(72,19,"视频",2,&test_dis_Multimedia,0,0);
		lcd_update_all();
		break;
	case 2:
		lcd_fill(0);
		DisAddRead_ZK(18,3,"多媒体信息选择",7,&test_dis_Multimedia,0,0);
		DisAddRead_ZK(24,19,"音频",2,&test_dis_Multimedia,0,0);
		DisAddRead_ZK(72,19,"视频",2,&test_dis_Multimedia,1,0);
		lcd_update_all();
		break;
	default:
		break;
	}
}
static void show(void)
{
	Multimedia(1);
}


static void keypress(unsigned int key)
{
switch(KeyValue)
	{
	case KeyValueMenu:
		pMenuItem=&Menu_1_InforInteract;
		pMenuItem->show();
		CounterBack=0;
		
		Menu_Multimedia=0;
		Multimedia_change=1;//选择
		Multimedia_screen=0;//界面切换使用
		break;
	case KeyValueOk:
		if(Multimedia_screen==0)
			{
			Multimedia_screen=1;
			lcd_fill(0);
			DisAddRead_ZK(7,3,"发送多媒体数据类型",9,&test_dis_Multimedia,0,0);  
			if(Multimedia_change==1)
				{
				//CarLoadState_Write();
				DisAddRead_ZK(48,19,"音频",2,&test_dis_Multimedia,1,0);
				}
			else if(Multimedia_change==2)
				{
				//CarLoadState_Write(); 
				DisAddRead_ZK(48,19,"视频",2,&test_dis_Multimedia,1,0);
				}
			lcd_update_all();
			}
		else if(Multimedia_screen==1)
			{
			Multimedia_screen=2;
			lcd_fill(0);
			if(Multimedia_change==1)
				{
				DisAddRead_ZK(24,10,"音频",2,&test_dis_Multimedia,1,0);
				//Multimedia_Flag=1;
				////MP3_send_start();       //   以前的 音频 1
				//Sound_send_start();   //   以前的 音频2
				}
			else if(Multimedia_change==2)
				{
				DisAddRead_ZK(24,10,"视频",2,&test_dis_Multimedia,1,0);
				//Multimedia_Flag=2;
				////Video_send_start(); 
				}
			DisAddRead_ZK(48,10,"开始发送",4,&test_dis_Multimedia,0,0);
			lcd_update_all();
			}
		break;
	case KeyValueUP:
		if(Multimedia_screen==0)
			{
			Multimedia_change=1;
			Multimedia(1);
			}
		break;
	case KeyValueDown:
		if(Multimedia_screen==0)
			{
			Multimedia_change=2;
			Multimedia(2);
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


MENUITEM	Menu_2_4_4_Multimedia=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};


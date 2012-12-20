#include "Menu_Include.h"



unsigned char Menu_Multimedia=0;
unsigned char Multimedia_change=1;//选择
unsigned char Multimedia_screen=0;//界面切换使用


void Multimedia(unsigned char type)
{
	lcd_fill(0);
	lcd_text12(18,3,"多媒体信息选择",14,LCD_MODE_SET);	
	if(type==1)
		{
		lcd_text12(24,19,"音频",4,LCD_MODE_INVERT);
		lcd_text12(72,19,"视频",4,LCD_MODE_SET);
		}
	else if(type==2)
		{
		lcd_text12(24,19,"音频",4,LCD_MODE_SET);
		lcd_text12(72,19,"视频",4,LCD_MODE_INVERT);
		}
	lcd_update_all();

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
			
			if(Multimedia_change==1)
				{
				//CarLoadState_Write();
				lcd_text12(7,10,"多媒体数据类型:音频",19,LCD_MODE_SET);  
				}
			else if(Multimedia_change==2)
				{
				//CarLoadState_Write(); 
				lcd_text12(7,10,"多媒体数据类型:视频",19,LCD_MODE_SET);  
				}
			lcd_update_all();
			}
		else if(Multimedia_screen==1)
			{
			Multimedia_screen=2;
			lcd_fill(0);
			if(Multimedia_change==1)
				{
				lcd_text12(18,10,"音频  开始发送",14,LCD_MODE_SET);
				//Multimedia_Flag=1;
				////MP3_send_start();       //   以前的 音频 1
				//Sound_send_start();   //   以前的 音频2
				}
			else if(Multimedia_change==2)
				{
				lcd_text12(18,10,"视频  开始发送",14,LCD_MODE_SET);
				//Multimedia_Flag=2;
				////Video_send_start(); 
				}
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

ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_2_4_4_Multimedia=
{
	"发送多媒体数据",
	14,
	&show,
	&keypress,
	&timetick,
	(void*)0
};


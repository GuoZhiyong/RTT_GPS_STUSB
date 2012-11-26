#include "menu.h"

struct IMG_DEF test_dis_Record={12,12,test_00};


unsigned char Record_screen=0;//进入录音界面=1,准备发送开始/结束时为2   开始/结束已发送为3    到下一界面恢复初试值
unsigned char Record_StartEnd=0;//==1录音开始   ==2录音结束   到下一界面恢复初试值


static void show(void)
{
	lcd_fill(0);
	DisAddRead_ZK(36,3,"录音选择",4,&test_dis_Record,0,0);
	DisAddRead_ZK(24,19,"开始",2,&test_dis_Record,1,0);
	DisAddRead_ZK(72,19,"结束",2,&test_dis_Record,0,0);				  
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
		    
		Record_screen=0;//进入录音界面=1,准备发送开始/结束时为2	 开始/结束已发送为3    到下一界面恢复初试值
		Record_StartEnd=0;//==1录音开始	 ==2录音结束   到下一界面恢复初试值
		break;
	case KeyValueOk:
		if(Record_screen==1)
			{
			Record_screen=2;
			if(Record_StartEnd==1)
				{
				lcd_fill(0);
				DisAddRead_ZK(24,3,"发送录音选择",6,&test_dis_Record,0,0);
				DisAddRead_ZK(48,19,"开始",2,&test_dis_Record,1,0);
				lcd_update_all();
				}
			else if(Record_StartEnd==2)
				{
				lcd_fill(0);
				DisAddRead_ZK(24,3,"发送录音选择",6,&test_dis_Record,0,0);
				DisAddRead_ZK(48,19,"结束",2,&test_dis_Record,1,0);
				lcd_update_all();
				}
			}
		else if(Record_screen==2)
			{
			Record_screen=3;
			if(Record_StartEnd==1)
				{
				lcd_fill(0);
				DisAddRead_ZK(18,10,"开始录音",4,&test_dis_Record,1,0);
				DisAddRead_ZK(66,10,"已发送",3,&test_dis_Record,0,0);
				lcd_update_all();
				}
			else if(Record_StartEnd==2)
				{
				lcd_fill(0);
				DisAddRead_ZK(18,10,"结束录音",4,&test_dis_Record,1,0);
				DisAddRead_ZK(66,10,"已发送",3,&test_dis_Record,0,0);
				lcd_update_all();
				}
			}
		else if(Record_screen==3)//回到录音开始/录音结束界面
			{
			Record_screen=1;
			Record_StartEnd=1;

			lcd_fill(0);
        	DisAddRead_ZK(36,3,"录音选择",4,&test_dis_Record,0,0);
			DisAddRead_ZK(24,19,"开始",2,&test_dis_Record,1,0);
			DisAddRead_ZK(72,19,"结束",2,&test_dis_Record,0,0);
			lcd_update_all();
			}

		break;
	case KeyValueUP:
        if(Record_screen==1)
        	{
        	Record_StartEnd=1;
			lcd_fill(0);
        	DisAddRead_ZK(36,3,"录音选择",4,&test_dis_Record,0,0);
			DisAddRead_ZK(24,19,"开始",2,&test_dis_Record,1,0);
			DisAddRead_ZK(72,19,"结束",2,&test_dis_Record,0,0);
			lcd_update_all();
        	}
		break;
	case KeyValueDown:
         if(Record_screen==1)
        	{
        	Record_StartEnd=2;
			lcd_fill(0);
        	DisAddRead_ZK(36,3,"录音选择",4,&test_dis_Record,0,0);
			DisAddRead_ZK(24,19,"开始",2,&test_dis_Record,0,0);
			DisAddRead_ZK(72,19,"结束",2,&test_dis_Record,1,0);
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


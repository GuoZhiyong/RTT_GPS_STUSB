#include  <string.h>
#include "menu.h"
#include "Lcd_init.h"

struct IMG_DEF test_dis_Driver={12,12,test_00};

/*
static unsigned char Jiayuan_screen=0;  //  显示查看/发送界面   =1时选择是查看还是发送界面
static unsigned char CheckJiayuanFlag=0;//  1:进入显示驾驶员信息   2:进入发送驾驶员信息
static unsigned char Jiayuan_1_2=0;     // 0:表示在查看界面   1:表示在发送界面
*/
typedef struct _DIS_DIRVER_INFOR
{
unsigned char DIS_SELECT_check_send;
unsigned char DIS_ENTER_check_send;
unsigned char DIS_SHOW_check_send;
}DIS_DIRVER_INFOR;

DIS_DIRVER_INFOR DIS_DRIVER_inform_temp;



//驾驶员代码 
void Display_jiayuan(unsigned char NameCode)
{
unsigned char i=0;
lcd_fill(0);
if(NameCode==1)
	{
	DisAddRead_ZK(30,3,"驾驶员姓名",5,&test_dis_Driver,0,0);
	/*for(i=0;i<20;i++)
		{
		if((Driver_Info.DriveName[i]>=0xb0)&&(Driver_Info.DriveName[i]<=0xf7))
			i++;
		else
			break;
		}
	DisAddRead_ZK(48,19,(char *)Driver_Info.DriveName,i/2,&test_dis_Driver,0,0); */
	}
else
	{
	DisAddRead_ZK(30,3,"驾驶证号码",5,&test_dis_Driver,0,0);
	//lcd_text(15,19,FONT_SIX_DOT,(char *)Driver_Info.DriverCard_ID);
	}
lcd_update_all();
} 


static void show(void)
{
	lcd_fill(0);
	lcd_text(10,3,FONT_NINE_DOT,"1."); 
	DisAddRead_ZK(30,3,"驾驶员信息查看",7,&test_dis_Driver,1,0); 
	lcd_text(10,19,FONT_NINE_DOT,"2.");
	DisAddRead_ZK(30,19,"驾驶员信息发送",7,&test_dis_Driver,0,0);
	lcd_update_all();
	DIS_DRIVER_inform_temp.DIS_SELECT_check_send=1;
	DIS_DRIVER_inform_temp.DIS_ENTER_check_send=1;
}


static void keypress(unsigned int key)
{
	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_1_InforCheck;
			pMenuItem->show();
			CounterBack=0;

			memset(&DIS_DRIVER_inform_temp,0,sizeof(DIS_DRIVER_inform_temp));
			break;
		case KeyValueOk:
			if(DIS_DRIVER_inform_temp.DIS_ENTER_check_send==1)
				{
				DIS_DRIVER_inform_temp.DIS_ENTER_check_send=2;
				DIS_DRIVER_inform_temp.DIS_SELECT_check_send=0;//查看或者发送已经选好
				
				if(DIS_DRIVER_inform_temp.DIS_SHOW_check_send==0)//进入查看驾驶员信息界面
					Display_jiayuan(1);
				else if(DIS_DRIVER_inform_temp.DIS_SHOW_check_send==1)//进入发送驾驶员信息界面
					{
					lcd_fill(0);
					DisAddRead_ZK(10,10,"发送驾驶员信息",7,&test_dis_Driver,1,0);
					lcd_text(94,10,FONT_NINE_DOT," ?");
					lcd_update_all();
					}
				}
			else if(DIS_DRIVER_inform_temp.DIS_ENTER_check_send==2)
				{
				DIS_DRIVER_inform_temp.DIS_ENTER_check_send=3;
				if(DIS_DRIVER_inform_temp.DIS_SHOW_check_send==0)//返回查看和发送界面
					{
					lcd_fill(0);
					lcd_text(10,3,FONT_NINE_DOT,"1.");
					DisAddRead_ZK(30,3,"驾驶员信息查看",7,&test_dis_Driver,1,0);
					lcd_text(10,19,FONT_NINE_DOT,"2.");
					DisAddRead_ZK(30,19,"驾驶员信息发送",7,&test_dis_Driver,0,0);
					lcd_update_all();
					DIS_DRIVER_inform_temp.DIS_SELECT_check_send=1;
					DIS_DRIVER_inform_temp.DIS_ENTER_check_send=1;
					}
				else if(DIS_DRIVER_inform_temp.DIS_SHOW_check_send==1)//提示发送成功
					{
					lcd_fill(0);
					DisAddRead_ZK(5,10,"驾驶员信息发送成功",9,&test_dis_Driver,1,0);
					lcd_update_all();
					//SD_ACKflag.f_DriverInfoSD_0702H=1;
					DIS_DRIVER_inform_temp.DIS_ENTER_check_send=1;
					DIS_DRIVER_inform_temp.DIS_SELECT_check_send=0;
					DIS_DRIVER_inform_temp.DIS_SHOW_check_send=0;
					}
				}
			break;
		case KeyValueUP:
			if(DIS_DRIVER_inform_temp.DIS_ENTER_check_send==2)
				{
				if(DIS_DRIVER_inform_temp.DIS_SHOW_check_send==0)//查看
					Display_jiayuan(1);
				else if(DIS_DRIVER_inform_temp.DIS_SHOW_check_send==1)//发送
					{
					lcd_fill(0);
					DisAddRead_ZK(30,10,"发送驾驶员信息",7,&test_dis_Driver,1,0);
					lcd_update_all();
					}
				}
			else if(DIS_DRIVER_inform_temp.DIS_SELECT_check_send==1)//选择进入查看或者发送
				{
				DIS_DRIVER_inform_temp.DIS_SHOW_check_send=0;
				lcd_fill(0);
				lcd_text(10,3,FONT_NINE_DOT,"1.");
				DisAddRead_ZK(30,3,"驾驶员信息查看",7,&test_dis_Driver,1,0);
				lcd_text(10,19,FONT_NINE_DOT,"2.");
				DisAddRead_ZK(30,19,"驾驶员信息发送",7,&test_dis_Driver,0,0);
				lcd_update_all();
				}
			break;
		case KeyValueDown:
			if(DIS_DRIVER_inform_temp.DIS_ENTER_check_send==2)
				{
				if(DIS_DRIVER_inform_temp.DIS_SHOW_check_send==0)//查看
					Display_jiayuan(2);
				else if(DIS_DRIVER_inform_temp.DIS_SHOW_check_send==1)//发送
					{
					lcd_fill(0);
					DisAddRead_ZK(30,10,"发送驾驶员信息",7,&test_dis_Driver,1,0);
					lcd_update_all();
					}
				}
			else if(DIS_DRIVER_inform_temp.DIS_SELECT_check_send==1)//选择进入查看或者发送
				{
				DIS_DRIVER_inform_temp.DIS_SHOW_check_send=1;
				lcd_fill(0);
				lcd_text(10,3,FONT_NINE_DOT,"1.");
				DisAddRead_ZK(30,3,"驾驶员信息查看",7,&test_dis_Driver,0,0);
				lcd_text(10,19,FONT_NINE_DOT,"2.");
				DisAddRead_ZK(30,19,"驾驶员信息发送",7,&test_dis_Driver,1,0);
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
    memset(&DIS_DRIVER_inform_temp,0,sizeof(DIS_DRIVER_inform_temp));
}


MENUITEM	Menu_2_3_5_jiayuan=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};



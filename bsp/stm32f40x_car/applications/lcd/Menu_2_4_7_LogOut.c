#include "menu.h"
#include <stdio.h>
#include <string.h>

struct IMG_DEF test_dis_logout={12,12,test_00};
	   
unsigned char Menu_Logout=0;  //  1:准备注册再按确认键=2   =2:发送注册
unsigned char LogIn_Flag=0;//准备鉴权让其=1，再按确认键发送鉴权清0
unsigned char LogInorOut_screen=0;//选择鉴权/注销,选择好以后按确认键清0
unsigned char LogInorOut=0;//  1:选择鉴权	2:选择注册

static void show(void)
   {
   memset(test_idle,0,sizeof(test_idle));
   lcd_fill(0);
   lcd_text(5,3,FONT_NINE_DOT,"1.");
   DisAddRead_ZK(25,3,"车台鉴权",4,&test_dis_logout,1,0);
   lcd_text(5,19,FONT_NINE_DOT,"2.");
   DisAddRead_ZK(25,19,"车台注册",4,&test_dis_logout,0,0);
   lcd_update_all();
   LogInorOut_screen=1;
   LogInorOut=1;
   }

static void keypress(unsigned int key)
{
   switch(KeyValue)
	   {
	   case KeyValueMenu:
		   pMenuItem=&Menu_1_InforInteract;
		   pMenuItem->show();
		   CounterBack=0;

		   Menu_Logout=0;
		   LogIn_Flag=0;
		   LogInorOut_screen=0;
		   LogInorOut=0;//	1:鉴权	 2:注销
		   break;
	   case KeyValueOk:
		   if(LogInorOut==1)//鉴权
			   {
			   LogInorOut_screen=0;
			   LogInorOut=0;
			   lcd_fill(0);
			   DisAddRead_ZK(36,3,"车台鉴权",4,&test_dis_logout,0,0);
			   DisAddRead_ZK(24,19,"按确认键发送",6,&test_dis_logout,0,0);
			   lcd_update_all();
			   LogIn_Flag=1;
			   }
		   else if(LogIn_Flag==1)//鉴权已发送
			   {
			   LogIn_Flag=0;
			   lcd_fill(0);
			   DisAddRead_ZK(30,10,"鉴权已发送",5,&test_dis_logout,0,0);
			   lcd_update_all();
			   
			   //DEV_Login.Operate_enable=1;
			   //DEV_Login.Enable_sd=1;
			   }
		   else if(LogInorOut==2)//注册
			   {
			   LogInorOut_screen=0;
			   LogInorOut=0;
			   lcd_fill(0);
			   DisAddRead_ZK(36,3,"车台注册",4,&test_dis_logout,0,0);
			   DisAddRead_ZK(24,19,"按确认键发送",6,&test_dis_logout,0,0);
			   lcd_update_all();
			   Menu_Logout=1;
			   }
		   else if(Menu_Logout==1)
			   {
			   Menu_Logout=2;
			   lcd_fill(0);
			   DisAddRead_ZK(30,10,"注册已发送",5,&test_dis_logout,0,0);
			   lcd_update_all();
			   //DEV_regist.Enable_sd=1; // set 发送注册标志位
			   }
		   break;
	   case KeyValueUP:
		   if(LogInorOut_screen==1)
			   {
			   LogInorOut=1;
			   lcd_fill(0);
			   lcd_text(5,3,FONT_NINE_DOT,"1.");
			   DisAddRead_ZK(25,3,"车台鉴权",4,&test_dis_logout,1,0);
			   lcd_text(5,19,FONT_NINE_DOT,"2.");
			   DisAddRead_ZK(25,19,"车台注册",4,&test_dis_logout,0,0);
			   lcd_update_all();
			   }
		   break;
		   
	   case KeyValueDown:
		   if(LogInorOut_screen==1)
			   {
			   LogInorOut=2;
			   lcd_fill(0);
			   lcd_text(5,3,FONT_NINE_DOT,"1.");
			   DisAddRead_ZK(25,3,"车台鉴权",4,&test_dis_logout,0,0);
			   lcd_text(5,19,FONT_NINE_DOT,"2.");
			   DisAddRead_ZK(25,19,"车台注册",4,&test_dis_logout,1,0);
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
   else
	   {
	   pMenuItem=&Menu_1_Idle;
	   pMenuItem->show();
	   CounterBack=0;

	   Menu_Logout=0;
	   LogIn_Flag=0;
	   LogInorOut_screen=0;
	   LogInorOut=0;//	1:鉴权	 2:注册
	   }
}


MENUITEM    Menu_2_4_7_LogOut=
{
   &show,
   &keypress,
   &timetick,
   (void*)0
};


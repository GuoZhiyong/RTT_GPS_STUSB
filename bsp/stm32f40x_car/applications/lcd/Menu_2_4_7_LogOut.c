#include "Menu_Include.h"
#include <stdio.h>
#include <string.h>

	   
unsigned char Menu_Logout=0;  //  1:准备注册再按确认键=2   =2:发送注册
unsigned char LogIn_Flag=0;//准备鉴权让其=1，再按确认键发送鉴权清0
unsigned char LogInorOut_screen=0;//选择鉴权/注销,选择好以后按确认键清0
unsigned char LogInorOut=0;//  1:选择鉴权	2:选择注册


void confirm_login(unsigned char par)
{
lcd_fill(0);
if(par==1)
	{
	lcd_text12(30, 3,"1.车台鉴权",10,LCD_MODE_INVERT);
	lcd_text12(30,19,"2.车台注册",10,LCD_MODE_SET);
	}
else if(par==2)
	{
	lcd_text12(30, 3,"1.车台鉴权",10,LCD_MODE_SET);
	lcd_text12(30,19,"2.车台注册",10,LCD_MODE_INVERT);
	}
lcd_update_all();
}
static void show(void)
   {
   memset(test_idle,0,sizeof(test_idle));
   confirm_login(1);
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
			   lcd_text12(0,10,"按确认键发送车台鉴权",20,LCD_MODE_SET);
			   lcd_update_all();
			   LogIn_Flag=1;
			   }
		   else if(LogIn_Flag==1)//鉴权已发送
			   {
			   LogIn_Flag=0;
			   lcd_fill(0);
			   lcd_text12(30,10,"鉴权已发送",10,LCD_MODE_SET);
			   lcd_update_all();
			   
			   //DEV_Login.Operate_enable=1;
			   //DEV_Login.Enable_sd=1;
			   }
		   else if(LogInorOut==2)//注册
			   {
			   LogInorOut_screen=0;
			   LogInorOut=0;
			   lcd_fill(0);
			   lcd_text12(0,10,"按确认键发送车台注册",20,LCD_MODE_SET);
			   lcd_update_all();
			   Menu_Logout=1;
			   }
		   else if(Menu_Logout==1)
			   {
			   Menu_Logout=2;
			   lcd_fill(0);
			   lcd_text12(30,10,"注册已发送",10,LCD_MODE_SET);
			   lcd_update_all();
			   //DEV_regist.Enable_sd=1; // set 发送注册标志位
			   }
		   break;
	   case KeyValueUP:
		   if(LogInorOut_screen==1)
			   {
			   LogInorOut=1;
			   confirm_login(1);
			   }
		   break;
		   
	   case KeyValueDown:
		   if(LogInorOut_screen==1)
			   {
			   LogInorOut=2;
			   confirm_login(2);
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

ALIGN(RT_ALIGN_SIZE)
MENUITEM    Menu_2_4_7_LogOut=
{
   "鉴权注册",
   8,
   &show,
   &keypress,
   &timetick,
   (void*)0
};


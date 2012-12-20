#include "Menu_Include.h"
#include <stdio.h>
#include <string.h>

	   
unsigned char Menu_Logout=0;  //  1:׼��ע���ٰ�ȷ�ϼ�=2   =2:����ע��
unsigned char LogIn_Flag=0;//׼����Ȩ����=1���ٰ�ȷ�ϼ����ͼ�Ȩ��0
unsigned char LogInorOut_screen=0;//ѡ���Ȩ/ע��,ѡ����Ժ�ȷ�ϼ���0
unsigned char LogInorOut=0;//  1:ѡ���Ȩ	2:ѡ��ע��


void confirm_login(unsigned char par)
{
lcd_fill(0);
if(par==1)
	{
	lcd_text12(30, 3,"1.��̨��Ȩ",10,LCD_MODE_INVERT);
	lcd_text12(30,19,"2.��̨ע��",10,LCD_MODE_SET);
	}
else if(par==2)
	{
	lcd_text12(30, 3,"1.��̨��Ȩ",10,LCD_MODE_SET);
	lcd_text12(30,19,"2.��̨ע��",10,LCD_MODE_INVERT);
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
		   LogInorOut=0;//	1:��Ȩ	 2:ע��
		   break;
	   case KeyValueOk:
		   if(LogInorOut==1)//��Ȩ
			   {
			   LogInorOut_screen=0;
			   LogInorOut=0;
			   lcd_fill(0);
			   lcd_text12(0,10,"��ȷ�ϼ����ͳ�̨��Ȩ",20,LCD_MODE_SET);
			   lcd_update_all();
			   LogIn_Flag=1;
			   }
		   else if(LogIn_Flag==1)//��Ȩ�ѷ���
			   {
			   LogIn_Flag=0;
			   lcd_fill(0);
			   lcd_text12(30,10,"��Ȩ�ѷ���",10,LCD_MODE_SET);
			   lcd_update_all();
			   
			   //DEV_Login.Operate_enable=1;
			   //DEV_Login.Enable_sd=1;
			   }
		   else if(LogInorOut==2)//ע��
			   {
			   LogInorOut_screen=0;
			   LogInorOut=0;
			   lcd_fill(0);
			   lcd_text12(0,10,"��ȷ�ϼ����ͳ�̨ע��",20,LCD_MODE_SET);
			   lcd_update_all();
			   Menu_Logout=1;
			   }
		   else if(Menu_Logout==1)
			   {
			   Menu_Logout=2;
			   lcd_fill(0);
			   lcd_text12(30,10,"ע���ѷ���",10,LCD_MODE_SET);
			   lcd_update_all();
			   //DEV_regist.Enable_sd=1; // set ����ע���־λ
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
	   LogInorOut=0;//	1:��Ȩ	 2:ע��
	   }
}

ALIGN(RT_ALIGN_SIZE)
MENUITEM    Menu_2_4_7_LogOut=
{
   "��Ȩע��",
   8,
   &show,
   &keypress,
   &timetick,
   (void*)0
};


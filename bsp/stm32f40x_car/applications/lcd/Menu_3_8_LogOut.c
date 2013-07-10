#include "Menu_Include.h"
#include <stdio.h>
#include <string.h>
#include "sed1520.h"
	   
unsigned char Menu_Logout=0;  //  1:׼��ע���ٰ�ȷ�ϼ�=2   =2:����ע��
unsigned char LogInorOut_screen=0;//ѡ���Ȩ/ע��,ѡ����Ժ�ȷ�ϼ���0
unsigned char LogInorOut=0;//  1:ѡ���Ȩ	2:ѡ��ע��


void confirm_login(unsigned char par)
{
lcd_fill(0);
if(par==1)
	{
	lcd_text12(0, 10,"��ռ�Ȩ��",10,LCD_MODE_INVERT);
	lcd_text12(60,10," ��Ȩ ע��",10,LCD_MODE_SET);
	}
else if(par==2)
	{
	lcd_text12(0, 10,"��ռ�Ȩ��",10,LCD_MODE_SET);
	lcd_text12(66,10,"��Ȩ",4,LCD_MODE_INVERT);
	lcd_text12(96,10,"ע��",4,LCD_MODE_SET);
	}
else if(par==3)
	{
	lcd_text12(0, 10,"��ռ�Ȩ�� ��Ȩ ",16,LCD_MODE_SET);
	lcd_text12(96,10,"ע��",4,LCD_MODE_INVERT);
	}
lcd_update_all();
}
static void msg( void *p)
{
}
static void show(void)
   {
   confirm_login(1);
   LogInorOut_screen=1;
   LogInorOut=1;
   }

static void keypress(unsigned int key)
{
     u8  Reg_buf[40];
	 
   switch(key)
	   {
	   case KEY_MENU:
		   pMenuItem=&Menu_3_InforInteract;
		   pMenuItem->show();
		   CounterBack=0;

		   Menu_Logout=0;
		   LogInorOut_screen=0;
		   LogInorOut=0;//	1:��Ȩ	 2:ע��
		   break;
	   case KEY_OK:
	   	   if(LogInorOut==1)//��ռ�Ȩ��
			   {
			   LogInorOut_screen=0;
			   LogInorOut=0;
			   lcd_fill(0);
			   lcd_text12(6,10,"��ȷ�ϼ���ռ�Ȩ��",18,LCD_MODE_SET);
			   lcd_update_all();
			   Menu_Logout=1;
			   }
		   else if(Menu_Logout==1)
			   	{
			   	Menu_Logout=0;
				
				memset(jt808_param.id_0xF003,0,32);
				lcd_fill(0); 
				lcd_text12(24,10,"��Ȩ�������",12,LCD_MODE_SET);
				lcd_update_all();
			   	}
		  else if(LogInorOut==2)//��Ȩ
			   {
			   LogInorOut_screen=0;
			   LogInorOut=0;
			   lcd_fill(0);
			   lcd_text12(0,10,"��ȷ�ϼ����ͳ�̨��Ȩ",20,LCD_MODE_SET);
			   lcd_update_all();
			   Menu_Logout=2;
			   }
		   else if(Menu_Logout==2)//��Ȩ�ѷ��� 
			   {
			   Menu_Logout=0;

			   lcd_fill(0);
			   lcd_text12(30,10,"��Ȩ�ѷ���",10,LCD_MODE_SET);
			   lcd_update_all();
			   
//			   DEV_Login.Operate_enable=1;
//			   DEV_Login.Enable_sd=1;
			   }
		   else if(LogInorOut==3)//ע��
			   {
			   LogInorOut_screen=0;
			   LogInorOut=0;
			   lcd_fill(0);
			   lcd_text12(0,10,"��ȷ�ϼ����ͳ�̨ע��",20,LCD_MODE_SET);
			   lcd_update_all();
			   Menu_Logout=3;
			   }
		   else if(Menu_Logout==3)
			   {
			   Menu_Logout=0;

//				DEV_regist.Enable_sd=1; // set ����ע���־λ

			   lcd_fill(0);
			   lcd_text12(30,10,"ע���ѷ���",10,LCD_MODE_SET);
			   lcd_update_all();
			   }
		   break;
	   case KEY_UP:
		   if(LogInorOut_screen==1)
			   {
			   LogInorOut--;
			   if(LogInorOut<=1)
				   LogInorOut=1;
			   confirm_login(LogInorOut);
			   }
		   break;
		   
	   case KEY_DOWN:
		   if(LogInorOut_screen==1)
			   {
			   LogInorOut++;
			   if(LogInorOut>=3)
				   LogInorOut=3;
			   confirm_login(LogInorOut);
			   }
		   break;
		   
	   }

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
	   LogInorOut_screen=0;
	   LogInorOut=0;//	1:��Ȩ	 2:ע��
	   }
}

MENUITEM    Menu_3_8_LogOut=
{
   "��Ȩע��",
   8,
   &show,
   &keypress,
   &timetick,
   &msg,
   (void*)0
};


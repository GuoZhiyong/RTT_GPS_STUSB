#include "menu.h"
#include <stdio.h>
#include <string.h>

struct IMG_DEF test_dis_logout={12,12,test_00};
	   
unsigned char Menu_Logout=0;  //  1:׼��ע���ٰ�ȷ�ϼ�=2   =2:����ע��
unsigned char LogIn_Flag=0;//׼����Ȩ����=1���ٰ�ȷ�ϼ����ͼ�Ȩ��0
unsigned char LogInorOut_screen=0;//ѡ���Ȩ/ע��,ѡ����Ժ�ȷ�ϼ���0
unsigned char LogInorOut=0;//  1:ѡ���Ȩ	2:ѡ��ע��

static void show(void)
   {
   memset(test_idle,0,sizeof(test_idle));
   lcd_fill(0);
   lcd_text(5,3,FONT_NINE_DOT,"1.");
   DisAddRead_ZK(25,3,"��̨��Ȩ",4,&test_dis_logout,1,0);
   lcd_text(5,19,FONT_NINE_DOT,"2.");
   DisAddRead_ZK(25,19,"��̨ע��",4,&test_dis_logout,0,0);
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
		   LogInorOut=0;//	1:��Ȩ	 2:ע��
		   break;
	   case KeyValueOk:
		   if(LogInorOut==1)//��Ȩ
			   {
			   LogInorOut_screen=0;
			   LogInorOut=0;
			   lcd_fill(0);
			   DisAddRead_ZK(36,3,"��̨��Ȩ",4,&test_dis_logout,0,0);
			   DisAddRead_ZK(24,19,"��ȷ�ϼ�����",6,&test_dis_logout,0,0);
			   lcd_update_all();
			   LogIn_Flag=1;
			   }
		   else if(LogIn_Flag==1)//��Ȩ�ѷ���
			   {
			   LogIn_Flag=0;
			   lcd_fill(0);
			   DisAddRead_ZK(30,10,"��Ȩ�ѷ���",5,&test_dis_logout,0,0);
			   lcd_update_all();
			   
			   //DEV_Login.Operate_enable=1;
			   //DEV_Login.Enable_sd=1;
			   }
		   else if(LogInorOut==2)//ע��
			   {
			   LogInorOut_screen=0;
			   LogInorOut=0;
			   lcd_fill(0);
			   DisAddRead_ZK(36,3,"��̨ע��",4,&test_dis_logout,0,0);
			   DisAddRead_ZK(24,19,"��ȷ�ϼ�����",6,&test_dis_logout,0,0);
			   lcd_update_all();
			   Menu_Logout=1;
			   }
		   else if(Menu_Logout==1)
			   {
			   Menu_Logout=2;
			   lcd_fill(0);
			   DisAddRead_ZK(30,10,"ע���ѷ���",5,&test_dis_logout,0,0);
			   lcd_update_all();
			   //DEV_regist.Enable_sd=1; // set ����ע���־λ
			   }
		   break;
	   case KeyValueUP:
		   if(LogInorOut_screen==1)
			   {
			   LogInorOut=1;
			   lcd_fill(0);
			   lcd_text(5,3,FONT_NINE_DOT,"1.");
			   DisAddRead_ZK(25,3,"��̨��Ȩ",4,&test_dis_logout,1,0);
			   lcd_text(5,19,FONT_NINE_DOT,"2.");
			   DisAddRead_ZK(25,19,"��̨ע��",4,&test_dis_logout,0,0);
			   lcd_update_all();
			   }
		   break;
		   
	   case KeyValueDown:
		   if(LogInorOut_screen==1)
			   {
			   LogInorOut=2;
			   lcd_fill(0);
			   lcd_text(5,3,FONT_NINE_DOT,"1.");
			   DisAddRead_ZK(25,3,"��̨��Ȩ",4,&test_dis_logout,0,0);
			   lcd_text(5,19,FONT_NINE_DOT,"2.");
			   DisAddRead_ZK(25,19,"��̨ע��",4,&test_dis_logout,1,0);
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
	   LogInorOut=0;//	1:��Ȩ	 2:ע��
	   }
}


MENUITEM    Menu_2_4_7_LogOut=
{
   &show,
   &keypress,
   &timetick,
   (void*)0
};


#include "Menu_Include.h"
#include "App_moduleConfig.h"


unsigned char Menu_dianbo=0;
unsigned char dianbo_scree=1;
unsigned char MSG_TypeToCenter=0;//���͸����ĵ����

/*
typedef struct _MSG_BROADCAST
{
unsigned char	 INFO_TYPE; 	//	��Ϣ����
unsigned int     INFO_LEN;		//	��Ϣ����
unsigned char	 INFO_PlyCancel; // �㲥/ȡ����־	   0 ȡ��  1  �㲥
unsigned char	 INFO_SDFlag;	 //  ���ͱ�־λ
unsigned char	 INFO_Effective; //  ��ʾ�Ƿ���Ч	1 ��ʾ��Ч	  0  ��ʾ��Ч	  
unsigned char	 INFO_STR[30];	//	��Ϣ����
}MSG_BRODCAST;

MSG_BRODCAST	 MSG_Obj;	 // ��Ϣ�㲥		 
MSG_BRODCAST	 MSG_Obj_8[8];	// ��Ϣ�㲥    

*/
void SenddianboMeun(unsigned char screen,unsigned char SendOK)
{
if(SendOK==1)
  {
  MSG_Obj_8[screen-1].INFO_TYPE=screen;
  if(MSG_Obj_8[screen-1].INFO_Effective)
	  {
	  lcd_fill(0);
	  lcd_text12(30,3,(char*)MSG_Obj_8[screen-1].INFO_STR,MSG_Obj_8[screen-1].INFO_LEN,LCD_MODE_INVERT);
	  if(MSG_Obj_8[screen-1].INFO_PlyCancel==1)
		 lcd_text12(30,19,"�㲥�ɹ�",8,LCD_MODE_INVERT);
	  else if(MSG_Obj_8[screen-1].INFO_PlyCancel==0)
		 lcd_text12(30,19,"ȡ���ɹ�",8,LCD_MODE_INVERT);
	  lcd_update_all();
	  }
  MSG_TypeToCenter=screen;//���͸����ĵ㲥��Ϣ�����
  }
else
  {
  if((screen>=1)&&(screen<=8))
	  {
	  if(MSG_Obj_8[screen-1].INFO_Effective)
		  {
		  lcd_fill(0);
		  lcd_text12(15,5,(char *)MSG_Obj_8[screen-1].INFO_STR,MSG_Obj_8[screen-1].INFO_LEN,LCD_MODE_INVERT);
		  if(MSG_Obj_8[screen-1].INFO_PlyCancel)
			  {
			  lcd_text12(15,20,"�㲥",4,LCD_MODE_INVERT);
			  lcd_text12(80,20,"ȡ��",4,LCD_MODE_SET);
			  }
		  else
			  {
			  lcd_text12(15,20,"�㲥",4,LCD_MODE_SET);
			  lcd_text12(80,20,"ȡ��",4,LCD_MODE_INVERT);
			  }
		  lcd_update_all();
		  }
	  }
  }
}
  
  void Dis_dianbo(unsigned char screen)
  {
  switch(screen)
	  {
	  case 1:
		  lcd_fill(0);
		  lcd_text12(0,0,"1.",2,LCD_MODE_SET);
		  if(MSG_Obj_8[0].INFO_Effective)
			  lcd_text12(15,0,(char *)MSG_Obj_8[0].INFO_STR,MSG_Obj_8[0].INFO_LEN,LCD_MODE_INVERT);
		  else
			  lcd_text12(15,0,"��",2,LCD_MODE_INVERT);
		  
		  lcd_text12(0,16,"2.",2,LCD_MODE_SET);
		  if(MSG_Obj_8[1].INFO_Effective)
			  lcd_text12(15,16,(char *)MSG_Obj_8[1].INFO_STR,MSG_Obj_8[1].INFO_LEN,LCD_MODE_SET);
		  else
			  lcd_text12(15,16,"��",2,LCD_MODE_SET);
  
		  if(MSG_Obj_8[0].INFO_Effective)
			  {
			  if(MSG_Obj_8[0].INFO_PlyCancel==1)
				  lcd_text12(95,0,"�㲥",4,LCD_MODE_INVERT);
			  else
				  lcd_text12(95,0,"ȡ��",4,LCD_MODE_INVERT);
  
			  if(MSG_Obj_8[1].INFO_PlyCancel==1)
				  lcd_text12(95,16,"�㲥",4,LCD_MODE_INVERT);
			  else
				  lcd_text12(95,16,"ȡ��",4,LCD_MODE_INVERT);
			  }
		  lcd_update_all();
		  break;
	  case 2:
		  lcd_fill(0);
		  lcd_text12(0,0,"1.",2,LCD_MODE_SET);
		  if(MSG_Obj_8[0].INFO_Effective)
			  lcd_text12(15,0,(char *)MSG_Obj_8[0].INFO_STR,MSG_Obj_8[0].INFO_LEN,LCD_MODE_SET);
		  else
			  lcd_text12(15,0,"��",2,LCD_MODE_SET);
		  lcd_text12(0,16,"2.",2,LCD_MODE_SET);
		  if(MSG_Obj_8[1].INFO_Effective)
			  lcd_text12(15,16,(char *)MSG_Obj_8[1].INFO_STR,MSG_Obj_8[1].INFO_LEN,LCD_MODE_INVERT);
		  else
			  lcd_text12(15,16,"��",2,LCD_MODE_INVERT);
		  if(MSG_Obj_8[1].INFO_Effective)
			  {
			  if(MSG_Obj_8[0].INFO_PlyCancel==1)
				  lcd_text12(95,0,"�㲥",4,LCD_MODE_INVERT);
			  else
				  lcd_text12(95,0,"ȡ��",4,LCD_MODE_INVERT);
  
			  if(MSG_Obj_8[1].INFO_PlyCancel==1)
				  lcd_text12(95,16,"�㲥",4,LCD_MODE_INVERT);
			  else
				  lcd_text12(95,16,"ȡ��",4,LCD_MODE_INVERT);
			  }
		  lcd_update_all();
		  break;
	  case 3:
		  lcd_fill(0);
		  lcd_text12(0,0,"3.",2,LCD_MODE_SET);
		  if(MSG_Obj_8[2].INFO_Effective)
			  lcd_text12(15,0,(char *)MSG_Obj_8[2].INFO_STR,MSG_Obj_8[2].INFO_LEN,LCD_MODE_INVERT);
		  else
			  lcd_text12(15,0,"��",2,LCD_MODE_INVERT);
		  lcd_text12(0,16,"4.",2,LCD_MODE_SET);
		  if(MSG_Obj_8[3].INFO_Effective)
			  lcd_text12(15,16,(char *)MSG_Obj_8[3].INFO_STR,MSG_Obj_8[3].INFO_LEN,LCD_MODE_SET);
		  else
			  lcd_text12(15,16,"��",2,LCD_MODE_SET);
		  if(MSG_Obj_8[2].INFO_Effective)
			  {
			  if(MSG_Obj_8[2].INFO_PlyCancel==1)
				  lcd_text12(95,0,"�㲥",4,LCD_MODE_INVERT);
			  else
				  lcd_text12(95,0,"ȡ��",4,LCD_MODE_INVERT);
  
			  if(MSG_Obj_8[3].INFO_PlyCancel==1)
				  lcd_text12(95,16,"�㲥",4,LCD_MODE_INVERT);
			  else
				  lcd_text12(95,16,"ȡ��",4,LCD_MODE_INVERT);
			  }
		  lcd_update_all();
		  break;
	  case 4: 
		  lcd_fill(0);
		  lcd_text12(0,0,"3.",2,LCD_MODE_SET);
		  if(MSG_Obj_8[2].INFO_Effective)
			  lcd_text12(15,0,(char *)MSG_Obj_8[2].INFO_STR,MSG_Obj_8[2].INFO_LEN,LCD_MODE_SET);
		  else
			  lcd_text12(15,0,"��",2,LCD_MODE_SET);
		  lcd_text12(0,16,"4.",2,LCD_MODE_SET);
		  if(MSG_Obj_8[3].INFO_Effective)
			  lcd_text12(15,16,(char *)MSG_Obj_8[3].INFO_STR,MSG_Obj_8[3].INFO_LEN,LCD_MODE_INVERT);
		  else
			  lcd_text12(15,16,"��",2,LCD_MODE_INVERT);
		  if(MSG_Obj_8[3].INFO_Effective)
			  {
			  if(MSG_Obj_8[2].INFO_PlyCancel==1)
				  lcd_text12(95,0,"�㲥",4,LCD_MODE_INVERT);
			  else
				  lcd_text12(95,0,"ȡ��",4,LCD_MODE_INVERT);
  
			  if(MSG_Obj_8[3].INFO_PlyCancel==1)
				  lcd_text12(95,16,"�㲥",4,LCD_MODE_INVERT);
			  else
				  lcd_text12(95,16,"ȡ��",4,LCD_MODE_INVERT);
			  }
		  lcd_update_all();
		  break;
	  case 5:
		  lcd_fill(0);
		  lcd_text12(0,0,"5.",2,LCD_MODE_SET);
		  if(MSG_Obj_8[4].INFO_Effective)
			  lcd_text12(15,0,(char *)MSG_Obj_8[4].INFO_STR,MSG_Obj_8[4].INFO_LEN,LCD_MODE_INVERT);
		  else
			  lcd_text12(15,0,"��",2,LCD_MODE_INVERT);
		  lcd_text12(0,16,"6.",2,LCD_MODE_SET);
		  if(MSG_Obj_8[5].INFO_Effective)
			  lcd_text12(15,16,(char *)MSG_Obj_8[5].INFO_STR,MSG_Obj_8[5].INFO_LEN,LCD_MODE_SET);
		  else
			  lcd_text12(15,16,"��",2,LCD_MODE_SET);
		  if(MSG_Obj_8[4].INFO_Effective)
			  {
			  if(MSG_Obj_8[4].INFO_PlyCancel==1)
				  lcd_text12(95,0,"�㲥",4,LCD_MODE_INVERT);
			  else
				  lcd_text12(95,0,"ȡ��",4,LCD_MODE_INVERT);
  
			  if(MSG_Obj_8[5].INFO_PlyCancel==1)
				  lcd_text12(95,16,"�㲥",4,LCD_MODE_INVERT);
			  else
				  lcd_text12(95,16,"ȡ��",4,LCD_MODE_INVERT);
			  }
		  lcd_update_all();
		  break;
	  case 6: 
		  lcd_fill(0);
		  lcd_text12(0,0,"5.",2,LCD_MODE_SET);
		  if(MSG_Obj_8[4].INFO_Effective)
			  lcd_text12(15,0,(char *)MSG_Obj_8[4].INFO_STR,MSG_Obj_8[4].INFO_LEN,LCD_MODE_SET);
		  else
			  lcd_text12(15,0,"��",2,LCD_MODE_SET);
		  lcd_text12(0,16,"6.",2,LCD_MODE_SET);
		  if(MSG_Obj_8[5].INFO_Effective)
			  lcd_text12(15,16,(char *)MSG_Obj_8[5].INFO_STR,MSG_Obj_8[5].INFO_LEN,LCD_MODE_INVERT);
		  else
			  lcd_text12(15,16,"��",2,LCD_MODE_INVERT);
		  if(MSG_Obj_8[5].INFO_Effective)
			  {
			  if(MSG_Obj_8[4].INFO_PlyCancel==1)
				  lcd_text12(95,0,"�㲥",4,LCD_MODE_INVERT);
			  else
				  lcd_text12(95,0,"ȡ��",4,LCD_MODE_INVERT);
  
			  if(MSG_Obj_8[5].INFO_PlyCancel==1)
				  lcd_text12(95,16,"�㲥",4,LCD_MODE_INVERT);
			  else
				  lcd_text12(95,16,"ȡ��",4,LCD_MODE_INVERT);
			  }
		  lcd_update_all();
		  break;
	  case 7:
		  lcd_fill(0);
		  lcd_text12(0,0,"7.",2,LCD_MODE_SET);
		  if(MSG_Obj_8[6].INFO_Effective)
			  lcd_text12(15,0,(char *)MSG_Obj_8[6].INFO_STR,MSG_Obj_8[6].INFO_LEN,LCD_MODE_INVERT);
		  else
			  lcd_text12(15,0,"��",2,LCD_MODE_INVERT);
		  lcd_text12(0,16,"8.",2,LCD_MODE_SET);
		  if(MSG_Obj_8[7].INFO_Effective)
			  lcd_text12(15,16,(char *)MSG_Obj_8[7].INFO_STR,MSG_Obj_8[7].INFO_LEN,LCD_MODE_SET);
		  else
			  lcd_text12(15,16,"��",2,LCD_MODE_SET);
		  if(MSG_Obj_8[6].INFO_Effective)
			  {
			  if(MSG_Obj_8[6].INFO_PlyCancel==1)
				  lcd_text12(95,0,"�㲥",4,LCD_MODE_INVERT);
			  else
				  lcd_text12(95,0,"ȡ��",4,LCD_MODE_INVERT);
  
			  if(MSG_Obj_8[7].INFO_PlyCancel==1)
				  lcd_text12(95,16,"�㲥",4,LCD_MODE_INVERT);
			  else
				  lcd_text12(95,16,"ȡ��",4,LCD_MODE_INVERT);
			  }
		  lcd_update_all();
		  break;
	  case 8: 
		  lcd_fill(0);
		  lcd_text12(0,0,"7.",2,LCD_MODE_SET);
		  if(MSG_Obj_8[6].INFO_Effective)
			  lcd_text12(15,0,(char *)MSG_Obj_8[6].INFO_STR,MSG_Obj_8[6].INFO_LEN,LCD_MODE_SET);
		  else
			  lcd_text12(15,0,"��",2,LCD_MODE_SET);
		  lcd_text12(0,16,"8.",2,LCD_MODE_SET);
		  if(MSG_Obj_8[7].INFO_Effective)
			  lcd_text12(15,16,(char *)MSG_Obj_8[7].INFO_STR,MSG_Obj_8[7].INFO_LEN,LCD_MODE_INVERT);
		  else
			  lcd_text12(15,16,"��",2,LCD_MODE_INVERT);
		  if(MSG_Obj_8[7].INFO_Effective)
			  {
			  if(MSG_Obj_8[6].INFO_PlyCancel==1)
				  lcd_text12(95,0,"�㲥",4,LCD_MODE_INVERT);
			  else
				  lcd_text12(95,0,"ȡ��",4,LCD_MODE_INVERT);
  
			  if(MSG_Obj_8[7].INFO_PlyCancel==1)
				  lcd_text12(95,16,"�㲥",4,LCD_MODE_INVERT);
			  else
				  lcd_text12(95,16,"ȡ��",4,LCD_MODE_INVERT);
			  }
		  lcd_update_all();
		  break;
	  default :
		  break;
	  
	  }
  
  }
  static void msg( void *p)
{
}
  static void show(void)
	  {
	  //u8 i=0;
	  //����8����Ϣ���ж���ʾ
	  MSG_BroadCast_Read();
	  Dis_dianbo(1);
      Menu_dianbo=1;
	  }
  
  static void keypress(unsigned int key)
  {
  //u8 result=0;
	  switch(KeyValue)
		  {
		  case KeyValueMenu:
			  pMenuItem=&Menu_2_InforCheck;
			  pMenuItem->show();
			  CounterBack=0;
  
			  Menu_dianbo=0;
			  dianbo_scree=1;
              MSG_TypeToCenter=0;//���͸����ĵ����
			  break; 
		  case KeyValueOk:
			  if(Menu_dianbo==1)
				  {
				  Menu_dianbo=2;
				  //��ѡ�е����������ʾ���͵Ľ���
				  SenddianboMeun(dianbo_scree,0);
				  }
			  else if(Menu_dianbo==2)
				  {
				  //SD_ACKflag.f_MsgBroadCast_0303H=1;
				  /*MSG_Obj.INFO_TYPE=MSG_Obj_8[dianbo_scree-1].INFO_TYPE;
				  MSG_Obj.INFO_PlyCancel=MSG_Obj_8[dianbo_scree-1].INFO_PlyCancel;*/
  
				  //--- �������µ�״̬ ----------- 
				  //DF_WriteFlash(DF_Msg_Page+dianbo_scree-1, 0, (u8*)&MSG_Obj_8[dianbo_scree-1], sizeof(MSG_Obj_8[dianbo_scree-1]));
				  
				  //���ͳ�ȥ�Ľ���
				  SenddianboMeun(dianbo_scree,1);
				   
				  Menu_dianbo=3;//��ȷ�Ϸ�����Ϣ�鿴����
				  dianbo_scree=1;
				  }
			  else if(Menu_dianbo==3)
				  {
				  Menu_dianbo=0;  //   1
				  dianbo_scree=1;
				  }
			  break;
		  case KeyValueUP:
			  if(Menu_dianbo==1)
				  {
				  dianbo_scree--;
				  if(dianbo_scree<=1)
					  dianbo_scree=1;
				  Dis_dianbo(dianbo_scree);
				  }
			  else if(Menu_dianbo==2)
				  {
				  MSG_Obj_8[dianbo_scree-1].INFO_PlyCancel=1;//�㲥
				  MSG_Obj_8[dianbo_scree-1].INFO_TYPE=1;
				  SenddianboMeun(dianbo_scree,0);
				  }
			  break;
		  case KeyValueDown:
			  if(Menu_dianbo==1)
				  {
				  dianbo_scree++;
				  if(dianbo_scree>=8)
					  dianbo_scree=8;
				  Dis_dianbo(dianbo_scree);
				  }
			  else if(Menu_dianbo==2)
				  {
				  MSG_Obj_8[dianbo_scree-1].INFO_PlyCancel=0;//ȡ��
				  MSG_Obj_8[dianbo_scree-1].INFO_TYPE=1;// 0
				  SenddianboMeun(dianbo_scree,0);
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
	  }
  }
  
  
  MYTIME
  MENUITEM	  Menu_2_7_RequestProgram=
  {
	  "��Ϣ�㲥�鿴",
	  12,
	  &show,
	  &keypress,
	  &timetick,
	  &msg,
	  (void*)0
  };


#include "menu.h"
#include <stdio.h>
#include <string.h>

struct IMG_DEF test_dis_meun={12,12,test_00};

unsigned char Menu_dianbo=0;
unsigned char dianbo_scree=1;
unsigned char MSG_TypeToCenter=0;//���͸����ĵ����


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


void SenddianboMeun(unsigned char screen,unsigned char SendOK)
{
if(SendOK==1)
  {
  if(MSG_Obj_8[screen-1].INFO_Effective)
	  {
	  lcd_fill(0);
	  if(MSG_Obj_8[screen-1].INFO_PlyCancel==1)
		  {
		  DisAddRead_ZK(30,3,(char*)MSG_Obj_8[screen-1].INFO_STR,MSG_Obj_8[screen-1].INFO_LEN/2,&test_dis_meun,1,0);
		  DisAddRead_ZK(30,19,"�㲥�ɹ�",4,&test_dis_meun,1,0);
		  }
	  else if(MSG_Obj_8[screen-1].INFO_PlyCancel==0)
		  {
		  DisAddRead_ZK(30,3,(char*)MSG_Obj_8[screen-1].INFO_STR,MSG_Obj_8[screen-1].INFO_LEN/2,&test_dis_meun,1,0);
		  DisAddRead_ZK(30,19,"ȡ���ɹ�",4,&test_dis_meun,1,0);
		  }
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
		  DisAddRead_ZK(15,5,(char *)b1,MSG_Obj_8[screen-1].INFO_LEN/2,&test_dis_meun,1,0);
		  if(MSG_Obj_8[screen-1].INFO_PlyCancel)
			  {
			  DisAddRead_ZK(15,20,"�㲥",2,&test_dis_meun,1,0);
			  DisAddRead_ZK(80,20,"ȡ��",2,&test_dis_meun,0,0);
			  }
		  else
			  {
			  DisAddRead_ZK(15,20,"�㲥",2,&test_dis_meun,0,0);
			  DisAddRead_ZK(80,20,"ȡ��",2,&test_dis_meun,1,0);
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
		  lcd_text(0,0,FONT_NINE_DOT,"1.");
		  if(MSG_Obj_8[0].INFO_Effective)
			  DisAddRead_ZK(15,0,(char *)b1,MSG_Obj_8[0].INFO_LEN/2,&test_dis_meun,1,0);
		  else
			  DisAddRead_ZK(15,0,"��",1,&test_dis_meun,1,0);
		  
		  lcd_text(0,16,FONT_NINE_DOT,"2.");
		  if(MSG_Obj_8[1].INFO_Effective)
			  DisAddRead_ZK(15,16,(char *)b2,MSG_Obj_8[1].INFO_LEN/2,&test_dis_meun,0,0);
		  else
			  DisAddRead_ZK(15,16,"��",1,&test_dis_meun,0,0);
  
		  if(MSG_Obj_8[0].INFO_Effective)
			  {
			  if(MSG_Obj_8[0].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,0,"�㲥",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,0,"ȡ��",2,&test_dis_meun,1,0);
  
			  if(MSG_Obj_8[1].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,16,"�㲥",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,16,"ȡ��",2,&test_dis_meun,1,0);
			  }
		  lcd_update_all();
		  break;
	  case 2:
		  lcd_fill(0);
		  lcd_text(0,0,FONT_NINE_DOT,"1.");
		  if(MSG_Obj_8[0].INFO_Effective)
			  DisAddRead_ZK(15,0,(char *)b1,MSG_Obj_8[0].INFO_LEN/2,&test_dis_meun,0,0);
		  else
			  DisAddRead_ZK(15,0,"��",1,&test_dis_meun,0,0);
		  lcd_text(0,16,FONT_NINE_DOT,"2.");
		  if(MSG_Obj_8[1].INFO_Effective)
			  DisAddRead_ZK(15,16,(char *)b2,MSG_Obj_8[1].INFO_LEN/2,&test_dis_meun,1,0);
		  else
			  DisAddRead_ZK(15,16,"��",1,&test_dis_meun,1,0);
		  if(MSG_Obj_8[1].INFO_Effective)
			  {
			  if(MSG_Obj_8[0].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,0,"�㲥",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,0,"ȡ��",2,&test_dis_meun,1,0);
  
			  if(MSG_Obj_8[1].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,16,"�㲥",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,16,"ȡ��",2,&test_dis_meun,1,0);
			  }
		  lcd_update_all();
		  break;
	  case 3:
		  lcd_fill(0);
		  lcd_text(0,0,FONT_NINE_DOT,"3.");
		  if(MSG_Obj_8[2].INFO_Effective)
			  DisAddRead_ZK(15,0,(char *)b3,MSG_Obj_8[2].INFO_LEN/2,&test_dis_meun,1,0);
		  else
			  DisAddRead_ZK(15,0,"��",1,&test_dis_meun,1,0);
		  lcd_text(0,16,FONT_NINE_DOT,"4.");
		  if(MSG_Obj_8[3].INFO_Effective)
			  DisAddRead_ZK(15,16,(char *)b4,MSG_Obj_8[3].INFO_LEN/2,&test_dis_meun,0,0);
		  else
			  DisAddRead_ZK(15,16,"��",1,&test_dis_meun,0,0);
		  if(MSG_Obj_8[2].INFO_Effective)
			  {
			  if(MSG_Obj_8[2].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,0,"�㲥",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,0,"ȡ��",2,&test_dis_meun,1,0);
  
			  if(MSG_Obj_8[3].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,16,"�㲥",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,16,"ȡ��",2,&test_dis_meun,1,0);
			  }
		  lcd_update_all();
		  break;
	  case 4: 
		  lcd_fill(0);
		  lcd_text(0,0,FONT_NINE_DOT,"3.");
		  if(MSG_Obj_8[2].INFO_Effective)
			  DisAddRead_ZK(15,0,(char *)b3,MSG_Obj_8[2].INFO_LEN/2,&test_dis_meun,0,0);
		  else
			  DisAddRead_ZK(15,0,"��",1,&test_dis_meun,0,0);
		  lcd_text(0,16,FONT_NINE_DOT,"4.");
		  if(MSG_Obj_8[3].INFO_Effective)
			  DisAddRead_ZK(15,16,(char *)b4,MSG_Obj_8[3].INFO_LEN/2,&test_dis_meun,1,0);
		  else
			  DisAddRead_ZK(15,16,"��",1,&test_dis_meun,1,0);
		  if(MSG_Obj_8[3].INFO_Effective)
			  {
			  if(MSG_Obj_8[2].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,0,"�㲥",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,0,"ȡ��",2,&test_dis_meun,1,0);
  
			  if(MSG_Obj_8[3].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,16,"�㲥",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,16,"ȡ��",2,&test_dis_meun,1,0);
			  }
		  lcd_update_all();
		  break;
	  case 5:
		  lcd_fill(0);
		  lcd_text(0,0,FONT_NINE_DOT,"5.");
		  if(MSG_Obj_8[4].INFO_Effective)
			  DisAddRead_ZK(15,0,(char *)b5,MSG_Obj_8[4].INFO_LEN/2,&test_dis_meun,1,0);
		  else
			  DisAddRead_ZK(15,0,"��",1,&test_dis_meun,1,0);
		  lcd_text(0,16,FONT_NINE_DOT,"6.");
		  if(MSG_Obj_8[5].INFO_Effective)
			  DisAddRead_ZK(15,16,(char *)b6,MSG_Obj_8[5].INFO_LEN/2,&test_dis_meun,0,0);
		  else
			  DisAddRead_ZK(15,16,"��",1,&test_dis_meun,0,0);
		  if(MSG_Obj_8[4].INFO_Effective)
			  {
			  if(MSG_Obj_8[4].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,0,"�㲥",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,0,"ȡ��",2,&test_dis_meun,1,0);
  
			  if(MSG_Obj_8[5].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,16,"�㲥",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,16,"ȡ��",2,&test_dis_meun,1,0);
			  }
		  lcd_update_all();
		  break;
	  case 6: 
		  lcd_fill(0);
		  lcd_text(0,0,FONT_NINE_DOT,"5.");
		  if(MSG_Obj_8[4].INFO_Effective)
			  DisAddRead_ZK(15,0,(char *)b5,MSG_Obj_8[4].INFO_LEN/2,&test_dis_meun,0,0);
		  else
			  DisAddRead_ZK(15,0,"��",1,&test_dis_meun,0,0);
		  lcd_text(0,16,FONT_NINE_DOT,"6.");
		  if(MSG_Obj_8[5].INFO_Effective)
			  DisAddRead_ZK(15,16,(char *)b6,MSG_Obj_8[5].INFO_LEN/2,&test_dis_meun,1,0);
		  else
			  DisAddRead_ZK(15,16,"��",1,&test_dis_meun,1,0);
		  if(MSG_Obj_8[5].INFO_Effective)
			  {
			  if(MSG_Obj_8[4].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,0,"�㲥",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,0,"ȡ��",2,&test_dis_meun,1,0);
  
			  if(MSG_Obj_8[5].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,16,"�㲥",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,16,"ȡ��",2,&test_dis_meun,1,0);
			  }
		  lcd_update_all();
		  break;
	  case 7:
		  lcd_fill(0);
		  lcd_text(0,0,FONT_NINE_DOT,"7.");
		  if(MSG_Obj_8[6].INFO_Effective)
			  DisAddRead_ZK(15,0,(char *)b7,MSG_Obj_8[6].INFO_LEN/2,&test_dis_meun,1,0);
		  else
			  DisAddRead_ZK(15,0,"��",1,&test_dis_meun,1,0);
		  lcd_text(0,16,FONT_NINE_DOT,"8.");
		  if(MSG_Obj_8[7].INFO_Effective)
			  DisAddRead_ZK(15,16,(char *)b8,MSG_Obj_8[7].INFO_LEN/2,&test_dis_meun,0,0);
		  else
			  DisAddRead_ZK(15,16,"��",1,&test_dis_meun,0,0);
		  if(MSG_Obj_8[6].INFO_Effective)
			  {
			  if(MSG_Obj_8[6].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,0,"�㲥",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,0,"ȡ��",2,&test_dis_meun,1,0);
  
			  if(MSG_Obj_8[7].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,16,"�㲥",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,16,"ȡ��",2,&test_dis_meun,1,0);
			  }
		  lcd_update_all();
		  break;
	  case 8: 
		  lcd_fill(0);
		  lcd_text(0,0,FONT_NINE_DOT,"7.");
		  if(MSG_Obj_8[6].INFO_Effective)
			  DisAddRead_ZK(15,0,(char *)b7,MSG_Obj_8[6].INFO_LEN/2,&test_dis_meun,0,0);
		  else
			  DisAddRead_ZK(15,0,"��",1,&test_dis_meun,0,0);
		  lcd_text(0,16,FONT_NINE_DOT,"8.");
		  if(MSG_Obj_8[7].INFO_Effective)
			  DisAddRead_ZK(15,16,(char *)b8,MSG_Obj_8[7].INFO_LEN/2,&test_dis_meun,1,0);
		  else
			  DisAddRead_ZK(15,16,"��",1,&test_dis_meun,1,0);
		  if(MSG_Obj_8[7].INFO_Effective)
			  {
			  if(MSG_Obj_8[6].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,0,"�㲥",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,0,"ȡ��",2,&test_dis_meun,1,0);
  
			  if(MSG_Obj_8[7].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,16,"�㲥",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,16,"ȡ��",2,&test_dis_meun,1,0);
			  }
		  lcd_update_all();
		  break;
	  default :
		  break;
	  
	  }
  
  }
  
  static void show(void)
	  {
	  memset(test_idle,0,sizeof(test_idle));
	  lcd_fill(0);
	  DisAddRead_ZK(12,0,"��Ϣ�㲥�˵��鿴",8,&test_dis_meun,0,0);
	  lcd_text(40,16,FONT_NINE_DOT,"OK ?");
	  lcd_update_all();   
	  }
  
  static void keypress(unsigned int key)
  {
	  switch(KeyValue)
		  {
		  case KeyValueMenu:
			  pMenuItem=&Menu_1_InforCheck;
			  pMenuItem->show();
			  CounterBack=0;
  
			  Menu_dianbo=0;
			  dianbo_scree=1;
			  break; 
		  case KeyValueOk:
			  if(Menu_dianbo==0)
				  {
				  //����8����Ϣ���ж���ʾ
				  //MSG_Read(); 
				  Menu_dianbo=1;
				  Dis_dianbo(1);
				  }
			  else if(Menu_dianbo==1)
				  {
				  Menu_dianbo=2;
				  //��ѡ�е����������ʾ���͵Ľ���
				  SenddianboMeun(dianbo_scree,0);
				  }
			  else if(Menu_dianbo==2)
				  {
				  //SD_ACKflag.f_MsgBroadCast_0303H=1;
				  MSG_Obj.INFO_TYPE=MSG_Obj_8[dianbo_scree-1].INFO_TYPE;
				  MSG_Obj.INFO_PlyCancel=MSG_Obj_8[dianbo_scree-1].INFO_PlyCancel;
  
				  //--- �������µ�״̬ ----------- 
				  //DF_WriteFlash(DF_Msg_Page+dianbo_scree-1, 0, (u8*)&MSG_Obj_8[dianbo_scree-1], sizeof(MSG_Obj_8[dianbo_scree-1]));
				  
				  //���ͳ�ȥ�Ľ���
				  SenddianboMeun(dianbo_scree,1);
				   
				  Menu_dianbo=3;//��ȷ�Ϸ�����Ϣ�鿴����
				  dianbo_scree=1;
				  }
			  else if(Menu_dianbo==3)
				  {
				  Menu_dianbo=1;
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
				  MSG_Obj_8[dianbo_scree-1].INFO_TYPE=0;
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
  
  
  MENUITEM	  Menu_2_3_7_CenterInforMeun=
  {
	  "��Ϣ�㲥�鿴",
	  &show,
	  &keypress,
	  &timetick,
	  (void*)0
  };


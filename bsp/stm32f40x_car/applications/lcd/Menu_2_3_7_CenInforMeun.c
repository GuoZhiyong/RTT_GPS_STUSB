#include "menu.h"
#include <stdio.h>
#include <string.h>

struct IMG_DEF test_dis_meun={12,12,test_00};

unsigned char Menu_dianbo=0;
unsigned char dianbo_scree=1;
unsigned char MSG_TypeToCenter=0;//发送给中心的序号


typedef struct _MSG_BROADCAST
{
unsigned char	 INFO_TYPE; 	//	信息类型
unsigned int     INFO_LEN;		//	信息长度
unsigned char	 INFO_PlyCancel; // 点播/取消标志	   0 取消  1  点播
unsigned char	 INFO_SDFlag;	 //  发送标志位
unsigned char	 INFO_Effective; //  显示是否有效	1 显示有效	  0  显示无效	  
unsigned char	 INFO_STR[30];	//	信息内容
}MSG_BRODCAST;

MSG_BRODCAST	 MSG_Obj;	 // 信息点播		 
MSG_BRODCAST	 MSG_Obj_8[8];	// 信息点播    


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
		  DisAddRead_ZK(30,19,"点播成功",4,&test_dis_meun,1,0);
		  }
	  else if(MSG_Obj_8[screen-1].INFO_PlyCancel==0)
		  {
		  DisAddRead_ZK(30,3,(char*)MSG_Obj_8[screen-1].INFO_STR,MSG_Obj_8[screen-1].INFO_LEN/2,&test_dis_meun,1,0);
		  DisAddRead_ZK(30,19,"取消成功",4,&test_dis_meun,1,0);
		  }
	  lcd_update_all();
	  }
  MSG_TypeToCenter=screen;//发送给中心点播信息的序号
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
			  DisAddRead_ZK(15,20,"点播",2,&test_dis_meun,1,0);
			  DisAddRead_ZK(80,20,"取消",2,&test_dis_meun,0,0);
			  }
		  else
			  {
			  DisAddRead_ZK(15,20,"点播",2,&test_dis_meun,0,0);
			  DisAddRead_ZK(80,20,"取消",2,&test_dis_meun,1,0);
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
			  DisAddRead_ZK(15,0,"无",1,&test_dis_meun,1,0);
		  
		  lcd_text(0,16,FONT_NINE_DOT,"2.");
		  if(MSG_Obj_8[1].INFO_Effective)
			  DisAddRead_ZK(15,16,(char *)b2,MSG_Obj_8[1].INFO_LEN/2,&test_dis_meun,0,0);
		  else
			  DisAddRead_ZK(15,16,"无",1,&test_dis_meun,0,0);
  
		  if(MSG_Obj_8[0].INFO_Effective)
			  {
			  if(MSG_Obj_8[0].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,0,"点播",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,0,"取消",2,&test_dis_meun,1,0);
  
			  if(MSG_Obj_8[1].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,16,"点播",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,16,"取消",2,&test_dis_meun,1,0);
			  }
		  lcd_update_all();
		  break;
	  case 2:
		  lcd_fill(0);
		  lcd_text(0,0,FONT_NINE_DOT,"1.");
		  if(MSG_Obj_8[0].INFO_Effective)
			  DisAddRead_ZK(15,0,(char *)b1,MSG_Obj_8[0].INFO_LEN/2,&test_dis_meun,0,0);
		  else
			  DisAddRead_ZK(15,0,"无",1,&test_dis_meun,0,0);
		  lcd_text(0,16,FONT_NINE_DOT,"2.");
		  if(MSG_Obj_8[1].INFO_Effective)
			  DisAddRead_ZK(15,16,(char *)b2,MSG_Obj_8[1].INFO_LEN/2,&test_dis_meun,1,0);
		  else
			  DisAddRead_ZK(15,16,"无",1,&test_dis_meun,1,0);
		  if(MSG_Obj_8[1].INFO_Effective)
			  {
			  if(MSG_Obj_8[0].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,0,"点播",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,0,"取消",2,&test_dis_meun,1,0);
  
			  if(MSG_Obj_8[1].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,16,"点播",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,16,"取消",2,&test_dis_meun,1,0);
			  }
		  lcd_update_all();
		  break;
	  case 3:
		  lcd_fill(0);
		  lcd_text(0,0,FONT_NINE_DOT,"3.");
		  if(MSG_Obj_8[2].INFO_Effective)
			  DisAddRead_ZK(15,0,(char *)b3,MSG_Obj_8[2].INFO_LEN/2,&test_dis_meun,1,0);
		  else
			  DisAddRead_ZK(15,0,"无",1,&test_dis_meun,1,0);
		  lcd_text(0,16,FONT_NINE_DOT,"4.");
		  if(MSG_Obj_8[3].INFO_Effective)
			  DisAddRead_ZK(15,16,(char *)b4,MSG_Obj_8[3].INFO_LEN/2,&test_dis_meun,0,0);
		  else
			  DisAddRead_ZK(15,16,"无",1,&test_dis_meun,0,0);
		  if(MSG_Obj_8[2].INFO_Effective)
			  {
			  if(MSG_Obj_8[2].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,0,"点播",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,0,"取消",2,&test_dis_meun,1,0);
  
			  if(MSG_Obj_8[3].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,16,"点播",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,16,"取消",2,&test_dis_meun,1,0);
			  }
		  lcd_update_all();
		  break;
	  case 4: 
		  lcd_fill(0);
		  lcd_text(0,0,FONT_NINE_DOT,"3.");
		  if(MSG_Obj_8[2].INFO_Effective)
			  DisAddRead_ZK(15,0,(char *)b3,MSG_Obj_8[2].INFO_LEN/2,&test_dis_meun,0,0);
		  else
			  DisAddRead_ZK(15,0,"无",1,&test_dis_meun,0,0);
		  lcd_text(0,16,FONT_NINE_DOT,"4.");
		  if(MSG_Obj_8[3].INFO_Effective)
			  DisAddRead_ZK(15,16,(char *)b4,MSG_Obj_8[3].INFO_LEN/2,&test_dis_meun,1,0);
		  else
			  DisAddRead_ZK(15,16,"无",1,&test_dis_meun,1,0);
		  if(MSG_Obj_8[3].INFO_Effective)
			  {
			  if(MSG_Obj_8[2].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,0,"点播",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,0,"取消",2,&test_dis_meun,1,0);
  
			  if(MSG_Obj_8[3].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,16,"点播",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,16,"取消",2,&test_dis_meun,1,0);
			  }
		  lcd_update_all();
		  break;
	  case 5:
		  lcd_fill(0);
		  lcd_text(0,0,FONT_NINE_DOT,"5.");
		  if(MSG_Obj_8[4].INFO_Effective)
			  DisAddRead_ZK(15,0,(char *)b5,MSG_Obj_8[4].INFO_LEN/2,&test_dis_meun,1,0);
		  else
			  DisAddRead_ZK(15,0,"无",1,&test_dis_meun,1,0);
		  lcd_text(0,16,FONT_NINE_DOT,"6.");
		  if(MSG_Obj_8[5].INFO_Effective)
			  DisAddRead_ZK(15,16,(char *)b6,MSG_Obj_8[5].INFO_LEN/2,&test_dis_meun,0,0);
		  else
			  DisAddRead_ZK(15,16,"无",1,&test_dis_meun,0,0);
		  if(MSG_Obj_8[4].INFO_Effective)
			  {
			  if(MSG_Obj_8[4].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,0,"点播",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,0,"取消",2,&test_dis_meun,1,0);
  
			  if(MSG_Obj_8[5].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,16,"点播",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,16,"取消",2,&test_dis_meun,1,0);
			  }
		  lcd_update_all();
		  break;
	  case 6: 
		  lcd_fill(0);
		  lcd_text(0,0,FONT_NINE_DOT,"5.");
		  if(MSG_Obj_8[4].INFO_Effective)
			  DisAddRead_ZK(15,0,(char *)b5,MSG_Obj_8[4].INFO_LEN/2,&test_dis_meun,0,0);
		  else
			  DisAddRead_ZK(15,0,"无",1,&test_dis_meun,0,0);
		  lcd_text(0,16,FONT_NINE_DOT,"6.");
		  if(MSG_Obj_8[5].INFO_Effective)
			  DisAddRead_ZK(15,16,(char *)b6,MSG_Obj_8[5].INFO_LEN/2,&test_dis_meun,1,0);
		  else
			  DisAddRead_ZK(15,16,"无",1,&test_dis_meun,1,0);
		  if(MSG_Obj_8[5].INFO_Effective)
			  {
			  if(MSG_Obj_8[4].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,0,"点播",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,0,"取消",2,&test_dis_meun,1,0);
  
			  if(MSG_Obj_8[5].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,16,"点播",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,16,"取消",2,&test_dis_meun,1,0);
			  }
		  lcd_update_all();
		  break;
	  case 7:
		  lcd_fill(0);
		  lcd_text(0,0,FONT_NINE_DOT,"7.");
		  if(MSG_Obj_8[6].INFO_Effective)
			  DisAddRead_ZK(15,0,(char *)b7,MSG_Obj_8[6].INFO_LEN/2,&test_dis_meun,1,0);
		  else
			  DisAddRead_ZK(15,0,"无",1,&test_dis_meun,1,0);
		  lcd_text(0,16,FONT_NINE_DOT,"8.");
		  if(MSG_Obj_8[7].INFO_Effective)
			  DisAddRead_ZK(15,16,(char *)b8,MSG_Obj_8[7].INFO_LEN/2,&test_dis_meun,0,0);
		  else
			  DisAddRead_ZK(15,16,"无",1,&test_dis_meun,0,0);
		  if(MSG_Obj_8[6].INFO_Effective)
			  {
			  if(MSG_Obj_8[6].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,0,"点播",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,0,"取消",2,&test_dis_meun,1,0);
  
			  if(MSG_Obj_8[7].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,16,"点播",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,16,"取消",2,&test_dis_meun,1,0);
			  }
		  lcd_update_all();
		  break;
	  case 8: 
		  lcd_fill(0);
		  lcd_text(0,0,FONT_NINE_DOT,"7.");
		  if(MSG_Obj_8[6].INFO_Effective)
			  DisAddRead_ZK(15,0,(char *)b7,MSG_Obj_8[6].INFO_LEN/2,&test_dis_meun,0,0);
		  else
			  DisAddRead_ZK(15,0,"无",1,&test_dis_meun,0,0);
		  lcd_text(0,16,FONT_NINE_DOT,"8.");
		  if(MSG_Obj_8[7].INFO_Effective)
			  DisAddRead_ZK(15,16,(char *)b8,MSG_Obj_8[7].INFO_LEN/2,&test_dis_meun,1,0);
		  else
			  DisAddRead_ZK(15,16,"无",1,&test_dis_meun,1,0);
		  if(MSG_Obj_8[7].INFO_Effective)
			  {
			  if(MSG_Obj_8[6].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,0,"点播",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,0,"取消",2,&test_dis_meun,1,0);
  
			  if(MSG_Obj_8[7].INFO_PlyCancel==1)
				  DisAddRead_ZK(95,16,"点播",2,&test_dis_meun,1,0);
			  else
				  DisAddRead_ZK(95,16,"取消",2,&test_dis_meun,1,0);
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
	  DisAddRead_ZK(12,0,"信息点播菜单查看",8,&test_dis_meun,0,0);
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
				  //读出8条消息在判断显示
				  //MSG_Read(); 
				  Menu_dianbo=1;
				  Dis_dianbo(1);
				  }
			  else if(Menu_dianbo==1)
				  {
				  Menu_dianbo=2;
				  //将选中的序号数据显示发送的界面
				  SenddianboMeun(dianbo_scree,0);
				  }
			  else if(Menu_dianbo==2)
				  {
				  //SD_ACKflag.f_MsgBroadCast_0303H=1;
				  MSG_Obj.INFO_TYPE=MSG_Obj_8[dianbo_scree-1].INFO_TYPE;
				  MSG_Obj.INFO_PlyCancel=MSG_Obj_8[dianbo_scree-1].INFO_PlyCancel;
  
				  //--- 更新最新的状态 ----------- 
				  //DF_WriteFlash(DF_Msg_Page+dianbo_scree-1, 0, (u8*)&MSG_Obj_8[dianbo_scree-1], sizeof(MSG_Obj_8[dianbo_scree-1]));
				  
				  //发送出去的界面
				  SenddianboMeun(dianbo_scree,1);
				   
				  Menu_dianbo=3;//按确认返回信息查看界面
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
				  MSG_Obj_8[dianbo_scree-1].INFO_PlyCancel=1;//点播
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
				  MSG_Obj_8[dianbo_scree-1].INFO_PlyCancel=0;//取消
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
	  "信息点播查看",
	  &show,
	  &keypress,
	  &timetick,
	  (void*)0
  };


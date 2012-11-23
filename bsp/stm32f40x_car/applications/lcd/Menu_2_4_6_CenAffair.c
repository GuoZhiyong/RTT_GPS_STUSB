#include "menu.h"
#include <stdio.h>
#include <string.h>

struct IMG_DEF test_dis_affair={12,12,test_00};

unsigned char Menu_Affair=0;
unsigned char Affair_scree=1;


//------- �¼� ----
typedef struct _EVENT
{
  unsigned char Event_ID;   //  �¼�ID
  unsigned char Event_Len;  //  �¼�����
  unsigned char Event_Effective; //  �¼��Ƿ���Ч��   1 ΪҪ��ʾ  0     
  unsigned char Event_Str[20];  //  �¼�����
}EVENT; 

EVENT          EventObj;    // �¼�   
EVENT          EventObj_8[8]; // �¼� 



void SendAffairMeun(unsigned char screen,unsigned char SendOK)
{
if(SendOK==1)
	{
	if(EventObj_8[screen-1].Event_Effective)
		{
		lcd_fill(0);
		DisAddRead_ZK(30,5,"���ͳɹ�",4,&test_dis_affair,1,0);
		lcd_update_all();
		}
	}
else
	{
	if((screen>=1)&&(screen<=8))
		{
		if(EventObj_8[screen-1].Event_Effective)
			{
			lcd_fill(0);
			DisAddRead_ZK(15,5,(char *)a1,EventObj_8[screen-1].Event_Len/2,&test_dis_affair,1,0);
			lcd_text(50,20,FONT_NINE_DOT,"OK ?");
			lcd_update_all();
			}
		}
	}
}


void Dis_Affair(unsigned char screen)
{
switch(screen)
	{
	case 1:
		lcd_fill(0);
		lcd_text(0,0,FONT_NINE_DOT,"1.");
		if(EventObj_8[0].Event_Effective)
			DisAddRead_ZK(15,0,(char *)a1,EventObj_8[0].Event_Len/2,&test_dis_affair,1,0);
		else
			DisAddRead_ZK(15,0,"��",1,&test_dis_affair,1,0);
		lcd_text(0,16,FONT_NINE_DOT,"2.");
		if(EventObj_8[1].Event_Effective)
			DisAddRead_ZK(15,16,(char *)a2,EventObj_8[1].Event_Len/2,&test_dis_affair,0,0);
		else
			DisAddRead_ZK(15,16,"��",1,&test_dis_affair,1,0);
		lcd_update_all();
		break;
	case 2:
		lcd_fill(0);
		lcd_text(0,0,FONT_NINE_DOT,"1.");
		if(EventObj_8[0].Event_Effective)
			DisAddRead_ZK(15,0,(char *)a1,EventObj_8[0].Event_Len/2,&test_dis_affair,0,0);
		else
			DisAddRead_ZK(15,0,"��",1,&test_dis_affair,1,0);
		lcd_text(0,16,FONT_NINE_DOT,"2.");
		if(EventObj_8[1].Event_Effective)
			DisAddRead_ZK(15,16,(char *)a2,EventObj_8[1].Event_Len/2,&test_dis_affair,1,0);
		else
			DisAddRead_ZK(15,16,"��",1,&test_dis_affair,1,0);
		lcd_update_all();
		break;
	case 3:
		lcd_fill(0);
		lcd_text(0,0,FONT_NINE_DOT,"3.");
		if(EventObj_8[2].Event_Effective)
			DisAddRead_ZK(15,0,(char *)a3,EventObj_8[2].Event_Len/2,&test_dis_affair,1,0);
		else
			DisAddRead_ZK(15,0,"��",1,&test_dis_affair,1,0);
		lcd_text(0,16,FONT_NINE_DOT,"4.");
		if(EventObj_8[3].Event_Effective)
			DisAddRead_ZK(15,16,(char *)a4,EventObj_8[3].Event_Len/2,&test_dis_affair,0,0);
		else
			DisAddRead_ZK(15,16,"��",1,&test_dis_affair,1,0);
		lcd_update_all();
		break;
	case 4: 
		lcd_fill(0);
		lcd_text(0,0,FONT_NINE_DOT,"3.");
		if(EventObj_8[2].Event_Effective)
			DisAddRead_ZK(15,0,(char *)a3,EventObj_8[2].Event_Len/2,&test_dis_affair,0,0);
		else
			DisAddRead_ZK(15,0,"��",1,&test_dis_affair,1,0);
		lcd_text(0,16,FONT_NINE_DOT,"4.");
		if(EventObj_8[3].Event_Effective)
			DisAddRead_ZK(15,16,(char *)a4,EventObj_8[3].Event_Len/2,&test_dis_affair,1,0);
		else
			DisAddRead_ZK(15,16,"��",1,&test_dis_affair,1,0);
		lcd_update_all();
		break;
	case 5:
		lcd_fill(0);
		lcd_text(0,0,FONT_NINE_DOT,"5.");
		if(EventObj_8[4].Event_Effective)
			DisAddRead_ZK(15,0,(char *)a5,EventObj_8[4].Event_Len/2,&test_dis_affair,1,0);
		else
			DisAddRead_ZK(15,0,"��",1,&test_dis_affair,1,0);
		lcd_text(0,16,FONT_NINE_DOT,"6.");
		if(EventObj_8[5].Event_Effective)
			DisAddRead_ZK(15,16,(char *)a6,EventObj_8[5].Event_Len/2,&test_dis_affair,0,0);
		else
			DisAddRead_ZK(15,16,"��",1,&test_dis_affair,0,0);
		lcd_update_all();
		break;
	case 6:
		lcd_fill(0);
		lcd_text(0,0,FONT_NINE_DOT,"5.");
		if(EventObj_8[4].Event_Effective)
			DisAddRead_ZK(15,0,(char *)a5,EventObj_8[4].Event_Len/2,&test_dis_affair,0,0);
		else
			DisAddRead_ZK(15,0,"��",1,&test_dis_affair,1,0);
		lcd_text(0,16,FONT_NINE_DOT,"6.");
		if(EventObj_8[5].Event_Effective)
			DisAddRead_ZK(15,16,(char *)a6,EventObj_8[5].Event_Len/2,&test_dis_affair,0,0);
		else
			DisAddRead_ZK(15,16,"��",1,&test_dis_affair,1,0);
		lcd_update_all();
		break;
	case 7:
		lcd_fill(0);
		lcd_text(0,0,FONT_NINE_DOT,"7.");
		if(EventObj_8[6].Event_Effective)
			DisAddRead_ZK(15,0,(char *)a7,EventObj_8[6].Event_Len/2,&test_dis_affair,1,0);
		else
			DisAddRead_ZK(15,0,"��",1,&test_dis_affair,1,0);
		lcd_text(0,16,FONT_NINE_DOT,"8.");
		if(EventObj_8[7].Event_Effective)
			DisAddRead_ZK(15,16,(char *)a8,EventObj_8[7].Event_Len/2,&test_dis_affair,0,0);
		else
			DisAddRead_ZK(15,16,"��",1,&test_dis_affair,0,0);
		lcd_update_all();
		break;
	case 8:
		lcd_fill(0);
		lcd_text(0,0,FONT_NINE_DOT,"7.");
		if(EventObj_8[6].Event_Effective)
			DisAddRead_ZK(15,0,(char *)a7,EventObj_8[6].Event_Len/2,&test_dis_affair,0,0);
		else
			DisAddRead_ZK(15,0,"��",1,&test_dis_affair,0,0);
		lcd_text(0,16,FONT_NINE_DOT,"8.");
		if(EventObj_8[7].Event_Effective)
			DisAddRead_ZK(15,16,(char *)a8,EventObj_8[7].Event_Len/2,&test_dis_affair,1,0);
		else
			DisAddRead_ZK(15,16,"��",1,&test_dis_affair,1,0);
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
	DisAddRead_ZK(36,0,"�¼�����",4,&test_dis_affair,0,0);
	lcd_text(40,16,FONT_NINE_DOT,"OK ?");
	lcd_update_all();


	}

static void keypress(unsigned int key)
{
	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_1_InforInteract;
			pMenuItem->show();
			CounterBack=0;

			Menu_Affair=0;
			Affair_scree=1;
			break;
		case KeyValueOk:
			if(Menu_Affair==0)
				{
				//EventObj_Read();//�����¼�
				Menu_Affair=1;
				Dis_Affair(1);
				}
			else if(Menu_Affair==1)
				{
				if(EventObj_8[Affair_scree-1].Event_Effective)
					{
					Menu_Affair=2;
					//��ѡ�е����������ʾ���͵Ľ���
					SendAffairMeun(Affair_scree,0);
					}
				}
			else if(Menu_Affair==2)
				{
				if(EventObj_8[Affair_scree-1].Event_Effective)
					{
					//------------	�����¼�ID���	----------------
					EventObj.Event_ID=Affair_scree;
					//SD_ACKflag.f_CurrentEventACK_0301H=1; 
					//�����¼���Ϣ�˵�
					Menu_Affair=0;
					Affair_scree=1;
					
					//���ͳ�ȥ�Ľ���
					SendAffairMeun(Affair_scree,1);
					
					}
				}
			break;
		case KeyValueUP:
			if(Menu_Affair==1)
				{
				Affair_scree--;
				if(Affair_scree<=1)
					Affair_scree=1;
				Dis_Affair(Affair_scree);
				}
			break;
		case KeyValueDown:
			if(Menu_Affair==1)
				{
				Affair_scree++;
				if(Affair_scree>=8)
					Affair_scree=8;
				Dis_Affair(Affair_scree);
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


MENUITEM	Menu_2_4_6_CenterAffairSet=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};


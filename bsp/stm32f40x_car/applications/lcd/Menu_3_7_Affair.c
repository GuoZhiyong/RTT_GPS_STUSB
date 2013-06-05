#include "Menu_Include.h"
#include <stdio.h>
#include <string.h>
#include "App_moduleConfig.h"

unsigned char Menu_Affair=0;
unsigned char Affair_scree=1;


//------- �¼� ----
/*typedef struct _EVENT
{
  unsigned char Event_ID;   //  �¼�ID
  unsigned char Event_Len;  //  �¼�����
  unsigned char Event_Effective; //  �¼��Ƿ���Ч��   1 ΪҪ��ʾ  0     
  unsigned char Event_Str[20];  //  �¼�����
}EVENT; 

EVENT          EventObj;    // �¼�   
EVENT          EventObj_8[8]; // �¼� 

*/

void SendAffairMeun(unsigned char screen,unsigned char SendOK)
{
if(SendOK==1)
	{
	if(EventObj_8[screen-1].Event_Effective)
		{
		lcd_fill(0);
		lcd_text12(30,5,"���ͳɹ�",8,LCD_MODE_INVERT);
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
			lcd_text12(15,5,(char *)EventObj_8[screen-1].Event_Str,EventObj_8[screen-1].Event_Len,LCD_MODE_INVERT);
			lcd_update_all();
			}
		}
	}
}


void Dis_Affair(unsigned char screen)
{
lcd_fill(0);
switch(screen)
	{
	case 1:
		lcd_text12(0,0,"1.",2,LCD_MODE_SET);
		if(EventObj_8[0].Event_Effective)
			lcd_text12(15,0,(char *)EventObj_8[0].Event_Str,EventObj_8[0].Event_Len,LCD_MODE_INVERT);
		else
			lcd_text12(15,0,"��",2,LCD_MODE_INVERT);	
		lcd_text12(0,16,"2.",2,LCD_MODE_SET);
		if(EventObj_8[1].Event_Effective)
			lcd_text12(15,16,(char *)EventObj_8[1].Event_Str,EventObj_8[1].Event_Len,LCD_MODE_SET);
		else
			lcd_text12(15,16,"��",2,LCD_MODE_SET);
		break;
		
	case 2:
		lcd_text12(0,0,"1.",2,LCD_MODE_SET);
		if(EventObj_8[0].Event_Effective)
			lcd_text12(15,0,(char *)EventObj_8[0].Event_Str,EventObj_8[0].Event_Len,LCD_MODE_SET);
		else
			lcd_text12(15,0,"��",2,LCD_MODE_SET);
		lcd_text12(0,16,"2.",2,LCD_MODE_SET);
		if(EventObj_8[1].Event_Effective)
			lcd_text12(15,16,(char *)EventObj_8[1].Event_Str,EventObj_8[1].Event_Len,LCD_MODE_INVERT);
		else
			lcd_text12(15,16,"��",2,LCD_MODE_INVERT); 
		break;
	case 3:
		lcd_text12(0,0,"3.",2,LCD_MODE_SET);
		if(EventObj_8[2].Event_Effective)
			lcd_text12(15,0,(char *)EventObj_8[2].Event_Str,EventObj_8[2].Event_Len,LCD_MODE_INVERT);
		else
			lcd_text12(15,0,"��",2,LCD_MODE_INVERT);
		lcd_text12(0,16,"4.",2,LCD_MODE_SET);
		if(EventObj_8[3].Event_Effective)
			lcd_text12(15,16,(char *)EventObj_8[3].Event_Str,EventObj_8[3].Event_Len,LCD_MODE_SET);
		else
			lcd_text12(15,16,"��",2,LCD_MODE_SET);
		break;
	case 4: 
		lcd_text12(0,0,"3.",2,LCD_MODE_SET);
		if(EventObj_8[2].Event_Effective)
			lcd_text12(15,0,(char *)EventObj_8[2].Event_Str,EventObj_8[2].Event_Len,LCD_MODE_SET);
		else
			lcd_text12(15,0,"��",2,LCD_MODE_SET);
		lcd_text12(0,16,"4.",2,LCD_MODE_SET);
		if(EventObj_8[3].Event_Effective)
			lcd_text12(15,16,(char *)EventObj_8[3].Event_Str,EventObj_8[3].Event_Len,LCD_MODE_INVERT);
		else
			lcd_text12(15,16,"��",2,LCD_MODE_INVERT);
		break;
	case 5:
		lcd_text12(0,0,"5.",2,LCD_MODE_SET);
		if(EventObj_8[4].Event_Effective)
			lcd_text12(15,0,(char *)EventObj_8[4].Event_Str,EventObj_8[4].Event_Len,LCD_MODE_INVERT);
		else
			lcd_text12(15,0,"��",2,LCD_MODE_INVERT);
		lcd_text12(0,16,"6.",2,LCD_MODE_SET);
		if(EventObj_8[5].Event_Effective)
			lcd_text12(15,16,(char *)EventObj_8[5].Event_Str,EventObj_8[5].Event_Len,LCD_MODE_SET);
		else
			lcd_text12(15,16,"��",2,LCD_MODE_SET);
		break;
	case 6:
		lcd_text12(0,0,"5.",2,LCD_MODE_SET);
		if(EventObj_8[4].Event_Effective)
			lcd_text12(15,0,(char *)EventObj_8[4].Event_Str,EventObj_8[4].Event_Len,LCD_MODE_SET);
		else
			lcd_text12(15,0,"��",2,LCD_MODE_SET);
		lcd_text12(0,16,"6.",2,LCD_MODE_SET);
		if(EventObj_8[5].Event_Effective)
			lcd_text12(15,16,(char *)EventObj_8[5].Event_Str,EventObj_8[5].Event_Len,LCD_MODE_SET);
		else
			lcd_text12(15,16,"��",2,LCD_MODE_INVERT);
		break;
	case 7:
		lcd_text12(0,0,"7.",2,LCD_MODE_SET);
		if(EventObj_8[6].Event_Effective)
			lcd_text12(15,0,(char *)EventObj_8[6].Event_Str,EventObj_8[6].Event_Len,LCD_MODE_INVERT);
		else
			lcd_text12(15,0,"��",2,LCD_MODE_INVERT);
		lcd_text12(0,16,"8.",2,LCD_MODE_SET);
		if(EventObj_8[7].Event_Effective)
			lcd_text12(15,16,(char *)EventObj_8[7].Event_Str,EventObj_8[7].Event_Len,LCD_MODE_SET);
		else
			lcd_text12(15,16,"��",2,LCD_MODE_SET);
		break;
	case 8:
		lcd_text12(0,0,"7.",2,LCD_MODE_SET);
		if(EventObj_8[6].Event_Effective)
			lcd_text12(15,0,(char *)EventObj_8[6].Event_Str,EventObj_8[6].Event_Len,LCD_MODE_SET);
		else
			lcd_text12(15,0,"��",2,LCD_MODE_SET);
		lcd_text12(0,16,"8.",2,LCD_MODE_SET);
		if(EventObj_8[7].Event_Effective)
			lcd_text12(15,16,(char *)EventObj_8[7].Event_Str,EventObj_8[7].Event_Len,LCD_MODE_INVERT);
		else
			lcd_text12(15,16,"��",2,LCD_MODE_INVERT);
		
		break;
	default :
		break;	
	}
lcd_update_all();
}

static void msg( void *p)
{
}
static void show(void)
	{
	Menu_Affair=0;
	lcd_fill(0);
	lcd_text12(36,3,"�¼���Ϣ",8,LCD_MODE_SET);
	lcd_text12(24,18,"��ȷ�ϼ��鿴",12,LCD_MODE_SET);
	lcd_update_all();
	}

static void keypress(unsigned int key)
{
	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_3_InforInteract;
			pMenuItem->show();
			CounterBack=0;

			Menu_Affair=0;
			Affair_scree=1;
			break;
		case KeyValueOk:
			if(Menu_Affair==0)
				{
				Event_Read();//�����¼�
				Dis_Affair(1);
				Menu_Affair=1;
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
					SD_ACKflag.f_CurrentEventACK_0301H=1; 
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

ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_3_7_Affair=
{
    "�¼���Ϣ",
	8,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};


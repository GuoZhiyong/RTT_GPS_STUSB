#include "Menu_Include.h"
#include <stdio.h>
#include <string.h>
#include "app_moduleConfig.h"

/*unsigned char Question_menu=0;//=0ʱ��ʾ��������Ȼ������=1��=2��ʾ�𰸣��ٰ�ȷ�ϼ�������𰸷��ͳ���(ǰ��:������������Ϣ)
unsigned char Question_Len=0;//���ⳤ��
unsigned char Question_Counter=0;//������ʾ��n��
unsigned char Question_screen=0;//������Ҫ��ʾ������
unsigned int  Answer1=0,Answer2=0;//��1�ĳ��Ⱥʹ�2�ĳ���
unsigned char Answer1_screen=0,Answer2_screen=0;//����Ҫ��ʾ����
unsigned char Question_stor=0;//�������·�����Ϣ
*/

typedef struct _DIS_QUESTION_INFOR
{
unsigned char DIS_QUESTION_answer;//=0ʱ��ʾ��������Ȼ������=1��=2��ʾ�𰸣��ٰ�ȷ�ϼ�������𰸷��ͳ���(ǰ��:������������Ϣ)
unsigned char DIS_QUESTION_len;//���ⳤ��
unsigned char DIS_QUESTION_current;//������Ҫ��ʾ������
unsigned char DIS_QUESTION_total;//������ʾ��n��
unsigned char  DIS_ANSWER1_ID;//��1�ĳ���
unsigned char  DIS_ANSWER2_ID;//��2�ĳ���
unsigned int  DIS_ANSWER1_len;//��1�ĳ���
unsigned int  DIS_ANSWER2_len;//��2�ĳ���
unsigned char  DIS_ANSWER1[20];//��1�ĳ���
unsigned char  DIS_ANSWER2[20];//��2�ĳ���
unsigned char DIS_ANSWER1_current;//��1��Ҫ��ʾ����
unsigned char DIS_ANSWER2_current;//��2��Ҫ��ʾ����
unsigned char DIS_QUESTION_effic;//�������·�����Ϣ
}DIS_QUESTION_INFOR;

DIS_QUESTION_INFOR DIS_QUESTION_INFOR_temp;

//-----  ���� ------
/*typedef struct _CENTER_ASK
{
  unsigned char  ASK_SdFlag; //  ��־λ           ���� TTS  1  ��   TTS ����  2
  unsigned int   ASK_floatID; // ������ˮ��
  unsigned char  ASK_infolen;// ��Ϣ����  
  unsigned char  ASK_answerID;    // �ظ�ID
  unsigned char  ASK_info[30];//  ��Ϣ����
  unsigned char  ASK_answer[30];  // ��ѡ��  
}CENTRE_ASK;

CENTRE_ASK     ASK_Centre;  // ��������
*/
static void msg( void *p)
{
}
static void show(void)
	{
    Api_RecordNum_Read(ask_quesstion,1, (u8*)&ASK_Centre,sizeof(ASK_Centre)); 	
	if(ASK_Centre.ASK_SdFlag==1)
		{
		DIS_QUESTION_INFOR_temp.DIS_QUESTION_effic=1;
		if(DIS_QUESTION_INFOR_temp.DIS_QUESTION_answer==0)
			{
			DIS_QUESTION_INFOR_temp.DIS_QUESTION_answer=1;
            DIS_QUESTION_INFOR_temp.DIS_QUESTION_len=ASK_Centre.ASK_infolen;//���ⳤ��
            DIS_QUESTION_INFOR_temp.DIS_ANSWER1_ID=ASK_Centre.ASK_answer[0];
			DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len=(((u16)ASK_Centre.ASK_answer[1])<<8)+((u16)ASK_Centre.ASK_answer[2]);
			memcpy(DIS_QUESTION_INFOR_temp.DIS_ANSWER1,ASK_Centre.ASK_answer+3,DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len);
            DIS_QUESTION_INFOR_temp.DIS_ANSWER2_ID=ASK_Centre.ASK_answer[3+DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len];
			DIS_QUESTION_INFOR_temp.DIS_ANSWER2_len=(((u16)ASK_Centre.ASK_answer[4+DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len])<<8)+((u16)ASK_Centre.ASK_answer[5+DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len]);
			memcpy(DIS_QUESTION_INFOR_temp.DIS_ANSWER2,ASK_Centre.ASK_answer+6+DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len,DIS_QUESTION_INFOR_temp.DIS_ANSWER2_len);		
			DIS_QUESTION_INFOR_temp.DIS_QUESTION_current++;
			if(DIS_QUESTION_INFOR_temp.DIS_QUESTION_len%40)//������Ҫ��ʾ����?
				DIS_QUESTION_INFOR_temp.DIS_QUESTION_total=DIS_QUESTION_INFOR_temp.DIS_QUESTION_len/40+1;
			else
				DIS_QUESTION_INFOR_temp.DIS_QUESTION_total=DIS_QUESTION_INFOR_temp.DIS_QUESTION_len/40;

			if(DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len%40)//��1��Ҫ��ʾ����?
				DIS_QUESTION_INFOR_temp.DIS_ANSWER1_current=DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len/40+1;
			else
				DIS_QUESTION_INFOR_temp.DIS_ANSWER1_current=DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len/40;

			if(DIS_QUESTION_INFOR_temp.DIS_ANSWER2_len%40)//��2��Ҫ��ʾ����?
				DIS_QUESTION_INFOR_temp.DIS_ANSWER1_current=DIS_QUESTION_INFOR_temp.DIS_ANSWER2_len/40+1;
			else
				DIS_QUESTION_INFOR_temp.DIS_ANSWER1_current=DIS_QUESTION_INFOR_temp.DIS_ANSWER2_len/40; 

			lcd_fill(0);
			if(DIS_QUESTION_INFOR_temp.DIS_QUESTION_len<=40)
				{
				if(DIS_QUESTION_INFOR_temp.DIS_QUESTION_len<=20)
					lcd_text12(0,0,(char *)ASK_Centre.ASK_info,ASK_Centre.ASK_infolen,LCD_MODE_SET);
				else
					{
					lcd_text12(0, 0,(char *)ASK_Centre.ASK_info,20,LCD_MODE_SET);
					lcd_text12(0,15,(char *)ASK_Centre.ASK_info,(ASK_Centre.ASK_infolen-20),LCD_MODE_SET);
					}
				}
			lcd_update_all();
			}		
		}
	else
		{
		lcd_fill(0);
		lcd_text12(24,10,"û��������Ϣ",12,LCD_MODE_SET);
		lcd_update_all();
		}
	}

static void keypress(unsigned int key)
{
	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_3_InforInteract;
			pMenuItem->show();
			
			CounterBack=0;
			memset(&DIS_QUESTION_INFOR_temp,0,sizeof(DIS_QUESTION_INFOR_temp));
			break;
		case KeyValueOk:
			if(DIS_QUESTION_INFOR_temp.DIS_QUESTION_effic==1)//��ѡ��Ľ����������
				{
				lcd_fill(0);
				lcd_text12(12,5,"����𰸷��ͳɹ�",16,LCD_MODE_SET);
				lcd_update_all();
				if(DIS_QUESTION_INFOR_temp.DIS_QUESTION_answer==2)
					ASK_Centre.ASK_answerID=DIS_QUESTION_INFOR_temp.DIS_ANSWER1_ID; 
				if(DIS_QUESTION_INFOR_temp.DIS_QUESTION_answer==3)
					ASK_Centre.ASK_answerID=DIS_QUESTION_INFOR_temp.DIS_ANSWER2_ID;	
				rt_kprintf("\r\n�������ĵĴ�ID:%d",ASK_Centre.ASK_answerID);
				ASK_Centre.ASK_SdFlag=2;//   �ѽ�����͸�����  
				}
			break;
		case KeyValueUP:
			if(DIS_QUESTION_INFOR_temp.DIS_QUESTION_effic==1)
				{
				if(DIS_QUESTION_INFOR_temp.DIS_QUESTION_answer==3)
					{
					DIS_QUESTION_INFOR_temp.DIS_QUESTION_answer=2;
					if((DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len<20)&&(DIS_QUESTION_INFOR_temp.DIS_ANSWER2_len<20))
						{
						lcd_fill(0);
						lcd_text12(0,0,(char *)DIS_QUESTION_INFOR_temp.DIS_ANSWER1,DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len,LCD_MODE_INVERT);
						lcd_text12(0,16,(char *)DIS_QUESTION_INFOR_temp.DIS_ANSWER2,DIS_QUESTION_INFOR_temp.DIS_ANSWER2_len,LCD_MODE_SET);
						lcd_update_all();
						}
					}
				}
			break;
		case KeyValueDown:
			if(DIS_QUESTION_INFOR_temp.DIS_QUESTION_effic==1)
				{
				if(DIS_QUESTION_INFOR_temp.DIS_QUESTION_answer==1)
					{
					DIS_QUESTION_INFOR_temp.DIS_QUESTION_current++;
					if(DIS_QUESTION_INFOR_temp.DIS_QUESTION_current>=DIS_QUESTION_INFOR_temp.DIS_QUESTION_total)
						{
						DIS_QUESTION_INFOR_temp.DIS_QUESTION_answer=2;//��ʾ��
						rt_kprintf("\r\ndown:��1_len=%d,str1=%s\r\n",DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len,DIS_QUESTION_INFOR_temp.DIS_ANSWER1);
			            rt_kprintf("\r\ndown:��2_len=%d,str2=%s\r\n",DIS_QUESTION_INFOR_temp.DIS_ANSWER2_len,DIS_QUESTION_INFOR_temp.DIS_ANSWER2);		
			  
						if((DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len<20)&&(DIS_QUESTION_INFOR_temp.DIS_ANSWER2_len<20))
							{
							lcd_fill(0);
							lcd_text12(0,0,(char *)DIS_QUESTION_INFOR_temp.DIS_ANSWER1,DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len,LCD_MODE_INVERT);
							lcd_text12(0,16,(char *)DIS_QUESTION_INFOR_temp.DIS_ANSWER2,DIS_QUESTION_INFOR_temp.DIS_ANSWER2_len,LCD_MODE_SET);
							lcd_update_all();
							}
						}
					 /*else//������ʾ��������
					 	{
					 	}*/
					}
				else if(DIS_QUESTION_INFOR_temp.DIS_QUESTION_answer==2)
					{
					DIS_QUESTION_INFOR_temp.DIS_QUESTION_answer=3;
					if((DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len<20)&&(DIS_QUESTION_INFOR_temp.DIS_ANSWER2_len<20))
						{
						lcd_fill(0);
						lcd_text12(0,0,(char *)DIS_QUESTION_INFOR_temp.DIS_ANSWER1,DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len,LCD_MODE_SET);
						lcd_text12(0,16,(char *)DIS_QUESTION_INFOR_temp.DIS_ANSWER2,DIS_QUESTION_INFOR_temp.DIS_ANSWER2_len,LCD_MODE_INVERT);
						lcd_update_all();
						}
					}
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
		memset(&DIS_QUESTION_INFOR_temp,0,sizeof(DIS_QUESTION_INFOR_temp));
		}
}



MYTIME
MENUITEM	Menu_3_1_CenterQuesSend=
{
	"����������Ϣ",
	12,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};


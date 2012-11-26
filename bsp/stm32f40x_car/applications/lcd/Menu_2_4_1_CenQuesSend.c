#include "menu.h"
#include <stdio.h>
#include<string.h>

struct IMG_DEF test_dis_question={12,12,test_00};

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
unsigned int  DIS_ANSWER1_len;//��1�ĳ���
unsigned int  DIS_ANSWER2_len;//��2�ĳ���
unsigned char DIS_ANSWER1_current;//��1��Ҫ��ʾ����
unsigned char DIS_ANSWER2_current;//��2��Ҫ��ʾ����
unsigned char DIS_QUESTION_effic;//�������·�����Ϣ
}DIS_QUESTION_INFOR;

DIS_QUESTION_INFOR DIS_QUESTION_INFOR_temp;

//-----  ���� ------
typedef struct _CENTER_ASK
{
  unsigned char  ASK_SdFlag; //  ��־λ           ���� TTS  1  ��   TTS ����  2
  unsigned int   ASK_floatID; // ������ˮ��
  unsigned char  ASK_infolen;// ��Ϣ����  
  unsigned char  ASK_answerID;    // �ظ�ID
  unsigned char  ASK_info[30];//  ��Ϣ����
  unsigned char  ASK_answer[30];  // ��ѡ��  
}CENTRE_ASK;

CENTRE_ASK     ASK_Centre;  // ��������


static void show(void)
	{
	memset(test_idle,0,sizeof(test_idle));
	//DF_ReadFlash(DF_question_Page,0,(u8*)&ASK_Centre,sizeof(ASK_Centre));
	if(ASK_Centre.ASK_SdFlag==1)
		{
		DIS_QUESTION_INFOR_temp.DIS_QUESTION_effic=1;
		if(DIS_QUESTION_INFOR_temp.DIS_QUESTION_answer==0)
			{
			DIS_QUESTION_INFOR_temp.DIS_QUESTION_answer=1;
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
			if(DIS_QUESTION_INFOR_temp.DIS_QUESTION_len<40)
				DisAddRead_ZK(0,0,(char *)ASK_Centre.ASK_info,ASK_Centre.ASK_infolen/2,&test_dis_question,0,0);
			lcd_update_all();
			}		
		}
	else
		{
		lcd_fill(0);
		DisAddRead_ZK(18,19,"û��������Ϣ",6,&test_dis_question,1,0);
		lcd_update_all();
		}
	}

static void keypress(unsigned int key)
{
	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_1_InforInteract;
			pMenuItem->show();
			
			CounterBack=0;
			memset(&DIS_QUESTION_INFOR_temp,0,sizeof(DIS_QUESTION_INFOR_temp));
			break;
		case KeyValueOk:
			if((DIS_QUESTION_INFOR_temp.DIS_QUESTION_effic==1)&&(DIS_QUESTION_INFOR_temp.DIS_QUESTION_answer==2))//��ѡ��Ľ����������
				{
				lcd_fill(0);
				DisAddRead_ZK(30,5,"���ͳɹ�",4,&test_dis_question,1,0);
				lcd_update_all();
				if(DIS_QUESTION_INFOR_temp.DIS_QUESTION_answer==1)
					ASK_Centre.ASK_answerID=ASK_Centre.ASK_answer[0]; 
				if(DIS_QUESTION_INFOR_temp.DIS_QUESTION_answer>=2)
					{
					ASK_Centre.ASK_answerID=ASK_Centre.ASK_answer[3+(ASK_Centre.ASK_answer[1]<<8)+ASK_Centre.ASK_answer[2]];
					}	
				ASK_Centre.ASK_SdFlag=2;//   �ѽ�����͸�����  
				}
			break;
		case KeyValueUP:
			if(DIS_QUESTION_INFOR_temp.DIS_QUESTION_effic==1)
				{
				if(DIS_QUESTION_INFOR_temp.DIS_QUESTION_answer==2)
					{
					if(DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len<10)
						{
						lcd_fill(0);
						DisAddRead_ZK(15,0,(char *)ASK_Centre.ASK_answer+3,DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len/2,&test_dis_question,1,0);
						lcd_update_all();
						}
					if(DIS_QUESTION_INFOR_temp.DIS_ANSWER2_len<10)
						{
						lcd_fill(0);
						DisAddRead_ZK(15,16,(char *)ASK_Centre.ASK_answer+6+DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len,DIS_QUESTION_INFOR_temp.DIS_ANSWER2_len/2,&test_dis_question,0,0);
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
						if(DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len<10)
							{
							lcd_fill(0);
							DisAddRead_ZK(15,0,(char *)ASK_Centre.ASK_answer+3,DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len/2,&test_dis_question,1,0);
							lcd_update_all();
							}
						if(DIS_QUESTION_INFOR_temp.DIS_ANSWER2_len<10)
							{
							lcd_fill(0);
							DisAddRead_ZK(15,16,(char *)ASK_Centre.ASK_answer+6+DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len,DIS_QUESTION_INFOR_temp.DIS_ANSWER2_len/2,&test_dis_question,0,0);
							lcd_update_all();
							}
						}
					}
				else if(DIS_QUESTION_INFOR_temp.DIS_QUESTION_answer==2)
					{
					if(DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len<10)
						{
						lcd_fill(0);
						DisAddRead_ZK(15,0,(char *)ASK_Centre.ASK_answer+3,DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len/2,&test_dis_question,0,0);
						lcd_update_all();
						}
					if(DIS_QUESTION_INFOR_temp.DIS_ANSWER2_len<10)
						{
						lcd_fill(0);
						DisAddRead_ZK(15,16,(char *)ASK_Centre.ASK_answer+6+DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len,DIS_QUESTION_INFOR_temp.DIS_ANSWER2_len/2,&test_dis_question,1,0);
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


MENUITEM	Menu_2_4_1_CenterQuestion=
{
	"",
	&show,
	&keypress,
	&timetick,
	(void*)0
};


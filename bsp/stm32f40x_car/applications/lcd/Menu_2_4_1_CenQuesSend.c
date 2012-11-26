#include "menu.h"
#include <stdio.h>
#include<string.h>

struct IMG_DEF test_dis_question={12,12,test_00};

/*unsigned char Question_menu=0;//=0时显示提问问题然后让其=1，=2显示答案，再按确认键把问题答案发送出来(前提:有中心提问消息)
unsigned char Question_Len=0;//问题长度
unsigned char Question_Counter=0;//问题显示第n屏
unsigned char Question_screen=0;//问题需要显示总屏数
unsigned int  Answer1=0,Answer2=0;//答案1的长度和答案2的长度
unsigned char Answer1_screen=0,Answer2_screen=0;//答案需要显示几屏
unsigned char Question_stor=0;//有中心下发的消息
*/

typedef struct _DIS_QUESTION_INFOR
{
unsigned char DIS_QUESTION_answer;//=0时显示提问问题然后让其=1，=2显示答案，再按确认键把问题答案发送出来(前提:有中心提问消息)
unsigned char DIS_QUESTION_len;//问题长度
unsigned char DIS_QUESTION_current;//问题需要显示总屏数
unsigned char DIS_QUESTION_total;//问题显示第n屏
unsigned int  DIS_ANSWER1_len;//答案1的长度
unsigned int  DIS_ANSWER2_len;//答案2的长度
unsigned char DIS_ANSWER1_current;//答案1需要显示几屏
unsigned char DIS_ANSWER2_current;//答案2需要显示几屏
unsigned char DIS_QUESTION_effic;//有中心下发的消息
}DIS_QUESTION_INFOR;

DIS_QUESTION_INFOR DIS_QUESTION_INFOR_temp;

//-----  提问 ------
typedef struct _CENTER_ASK
{
  unsigned char  ASK_SdFlag; //  标志位           发给 TTS  1  ；   TTS 回来  2
  unsigned int   ASK_floatID; // 提问流水号
  unsigned char  ASK_infolen;// 信息长度  
  unsigned char  ASK_answerID;    // 回复ID
  unsigned char  ASK_info[30];//  信息内容
  unsigned char  ASK_answer[30];  // 候选答案  
}CENTRE_ASK;

CENTRE_ASK     ASK_Centre;  // 中心提问


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
			if(DIS_QUESTION_INFOR_temp.DIS_QUESTION_len%40)//问题需要显示几屏?
				DIS_QUESTION_INFOR_temp.DIS_QUESTION_total=DIS_QUESTION_INFOR_temp.DIS_QUESTION_len/40+1;
			else
				DIS_QUESTION_INFOR_temp.DIS_QUESTION_total=DIS_QUESTION_INFOR_temp.DIS_QUESTION_len/40;

			if(DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len%40)//答案1需要显示几屏?
				DIS_QUESTION_INFOR_temp.DIS_ANSWER1_current=DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len/40+1;
			else
				DIS_QUESTION_INFOR_temp.DIS_ANSWER1_current=DIS_QUESTION_INFOR_temp.DIS_ANSWER1_len/40;

			if(DIS_QUESTION_INFOR_temp.DIS_ANSWER2_len%40)//答案2需要显示几屏?
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
		DisAddRead_ZK(18,19,"没有提问消息",6,&test_dis_question,1,0);
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
			if((DIS_QUESTION_INFOR_temp.DIS_QUESTION_effic==1)&&(DIS_QUESTION_INFOR_temp.DIS_QUESTION_answer==2))//把选择的结果发给中心
				{
				lcd_fill(0);
				DisAddRead_ZK(30,5,"发送成功",4,&test_dis_question,1,0);
				lcd_update_all();
				if(DIS_QUESTION_INFOR_temp.DIS_QUESTION_answer==1)
					ASK_Centre.ASK_answerID=ASK_Centre.ASK_answer[0]; 
				if(DIS_QUESTION_INFOR_temp.DIS_QUESTION_answer>=2)
					{
					ASK_Centre.ASK_answerID=ASK_Centre.ASK_answer[3+(ASK_Centre.ASK_answer[1]<<8)+ASK_Centre.ASK_answer[2]];
					}	
				ASK_Centre.ASK_SdFlag=2;//   把结果发送给中心  
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
						DIS_QUESTION_INFOR_temp.DIS_QUESTION_answer=2;//显示答案
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


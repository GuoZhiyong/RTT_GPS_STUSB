#include "menu.h"
#include <string.h>

struct IMG_DEF test_dis_text0={12,12,test_00};

//-------�ı���Ϣ-------
typedef struct _MSG_TEXT
{
  unsigned char   TEXT_mOld;     //  ���µ�һ����Ϣ  дΪ1���������µ�һ����Ϣ
  unsigned char   TEXT_TYPE;     //  ��Ϣ����   1-8  �еڼ���
  unsigned char   TEXT_LEN;      //  ��Ϣ����    
  unsigned char   TEXT_STR[150]; //  ��Ϣ����
}MSG_TEXT;

MSG_TEXT       TEXT_Obj;
MSG_TEXT       TEXT_Obj_8[8],TEXT_Obj_8bak[8];

/*static unsigned char dis_screen=0;//�ı���Ϣ������   =1ѡ��Ҫ��ʾ�ڼ�����Ϣ(1-8)��=2��ʾ��Ϣʱ�·���ʾ
static unsigned char dis_screen_counter=0;//�鿴���ǵڼ�����Ϣ(1-8)
static unsigned char TxtScreenNum_Total_0=0;//��Ҫ��ʾ����Ϣ������
static unsigned char TxtScreen_CurrentNum_0=0;// ��ǰ��ʾ�ڼ���
static unsigned char read_temp_0[50];//Ҫ��ʾ����Ϣ���ݣ�ÿ�������ʾ40���ֽ�
static unsigned int  TxtInfo_len_0=0;//Ҫ��ʾ����Ϣ����
*/
typedef struct _DIS_TEXT
{
unsigned char DIS_TEXT_screen;//�ı���Ϣ������   =1ѡ��Ҫ��ʾ�ڼ�����Ϣ(1-8)��=2��ʾ��Ϣʱ�·���ʾ
unsigned char DIS_TEXT_screen_counter;//�鿴���ǵڼ�����Ϣ(1-8)
unsigned char DIS_TEXT_screen_total_num;//��Ҫ��ʾ����Ϣ������
unsigned char DIS_TEXT_screen_current_num;//��ǰ��ʾ�ڼ���
unsigned char DIS_TEXT_inform[40];//Ҫ��ʾ����Ϣ���ݣ�ÿ�������ʾ40���ֽ�
unsigned int  DIS_TEXT_inform_len;//Ҫ��ʾ����Ϣ����
}DIS_TEXT;

DIS_TEXT DIS_TEXT_temp;


void DIS_MEUN_1(unsigned char screen)
{
char InforNum[2]={"0."};
if((screen>=1)&&(screen<=8))
	{
	InforNum[0]='0'+screen;
	lcd_fill(0);
	DisAddRead_ZK(36, 3,"������Ϣ",4,&test_dis_text0,1,0);
	lcd_text(0, 19,FONT_NINE_DOT,(char *)InforNum);
	DisAddRead_ZK(20, 19,"��Ϣ���ݲ鿴",6,&test_dis_text0,1,0);
	lcd_update_all();
	}
}

void DIS_MEUN_2(unsigned char screen)
{
char InforNum[2]={"0."};
if((screen>=1)&&(screen<=8))
	{
	InforNum[0]='0'+screen;
	lcd_fill(0);
	lcd_text(0, 10,FONT_NINE_DOT,(char *)InforNum);
	DisAddRead_ZK(20,10,(char *)TEXT_Obj_8[screen-1].TEXT_STR,TEXT_Obj_8[screen-1].TEXT_LEN/2,&test_dis_text0,0,0);
	lcd_update_all();
	}
}

static void show(void)
	{
	DIS_TEXT_temp.DIS_TEXT_screen=1;
	DIS_TEXT_temp.DIS_TEXT_screen_counter=1;
	DIS_MEUN_1(1);
	}

static void keypress(unsigned int key)
{
unsigned char CurrentDisplen=0;//��ʾ���ֵĸ���

	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_1_InforCheck;
			pMenuItem->show();
			
			CounterBack=0;
			memset(&DIS_TEXT_temp,0,sizeof(DIS_TEXT_temp));
			break;
		case KeyValueOk:
			if(DIS_TEXT_temp.DIS_TEXT_screen==1)
				{
				DIS_TEXT_temp.DIS_TEXT_screen=2;//��� dis_screen_counter ��ʾ��n����Ϣ
				//TEXT_Read();//����Ҫ��ʵ����Ϣ
				DIS_TEXT_temp.DIS_TEXT_inform_len=TEXT_Obj_8[DIS_TEXT_temp.DIS_TEXT_screen_counter-1].TEXT_LEN;// �յ��ı���Ϣ���� 
                if(DIS_TEXT_temp.DIS_TEXT_inform_len==0)
                	{
                	lcd_fill(0);
					lcd_text(45, 10,FONT_NINE_DOT,"[");
					DisAddRead_ZK(54,10,"��",1,&test_dis_text0,0,0); 
					lcd_text(66, 10,FONT_NINE_DOT,"]");
					lcd_update_all();
                	}
				else
					{
					if(DIS_TEXT_temp.DIS_TEXT_inform_len>40)
						{
						DIS_TEXT_temp.DIS_TEXT_screen_current_num++;
						if(DIS_TEXT_temp.DIS_TEXT_inform_len%40)					
							DIS_TEXT_temp.DIS_TEXT_screen_total_num=DIS_TEXT_temp.DIS_TEXT_inform_len/40+1;
						else
							DIS_TEXT_temp.DIS_TEXT_screen_total_num=DIS_TEXT_temp.DIS_TEXT_inform_len/40;	
					    CurrentDisplen=20;
						}
					else
						{
						DIS_TEXT_temp.DIS_TEXT_screen_total_num=1;
						CurrentDisplen=DIS_TEXT_temp.DIS_TEXT_inform_len/2;
						}				
				
					lcd_fill(0);
					DisAddRead_ZK(0,0,(char *)TEXT_Obj_8[DIS_TEXT_temp.DIS_TEXT_screen_counter-1].TEXT_STR,CurrentDisplen,&test_dis_text0,0,0); 
					lcd_update_all();
					}
				//========================================================
				}
			else if(DIS_TEXT_temp.DIS_TEXT_screen==2)
				{
				DIS_TEXT_temp.DIS_TEXT_screen=1;
				DIS_TEXT_temp.DIS_TEXT_screen_current_num=0;
				DIS_MEUN_1(DIS_TEXT_temp.DIS_TEXT_screen_counter);
				//TEXT_Read();//����Ҫ��ʵ����Ϣ
				}

			break;
		case KeyValueUP:
            if(DIS_TEXT_temp.DIS_TEXT_screen==1)
            	{
            	DIS_TEXT_temp.DIS_TEXT_screen_counter--;
				if(DIS_TEXT_temp.DIS_TEXT_screen_counter<=1)
					DIS_TEXT_temp.DIS_TEXT_screen_counter=1;
				DIS_MEUN_1(DIS_TEXT_temp.DIS_TEXT_screen_counter);
            	}		
			break;
		case KeyValueDown:
			 if(DIS_TEXT_temp.DIS_TEXT_screen==1)
            	{
            	DIS_TEXT_temp.DIS_TEXT_screen_counter++;
				if(DIS_TEXT_temp.DIS_TEXT_screen_counter>=8)
					DIS_TEXT_temp.DIS_TEXT_screen_counter=8;
				DIS_MEUN_1(DIS_TEXT_temp.DIS_TEXT_screen_counter);
            	}
			 else if((DIS_TEXT_temp.DIS_TEXT_screen==2)&&(DIS_TEXT_temp.DIS_TEXT_inform_len>0))
				{
				DIS_TEXT_temp.DIS_TEXT_screen_current_num++;
				if(DIS_TEXT_temp.DIS_TEXT_screen_current_num>=DIS_TEXT_temp.DIS_TEXT_screen_total_num)
					DIS_TEXT_temp.DIS_TEXT_screen_current_num=DIS_TEXT_temp.DIS_TEXT_screen_total_num;
				memset(test_idle,0,sizeof(test_idle));
				lcd_fill(0);
				if(DIS_TEXT_temp.DIS_TEXT_screen_current_num!=DIS_TEXT_temp.DIS_TEXT_screen_total_num)
					{
					memcpy(DIS_TEXT_temp.DIS_TEXT_inform,&TEXT_Obj_8[DIS_TEXT_temp.DIS_TEXT_screen_counter-1].TEXT_STR[40*(DIS_TEXT_temp.DIS_TEXT_screen_current_num-1)],40);//20������20*2���ֽ�
					DisAddRead_ZK(0,0,(char *)DIS_TEXT_temp.DIS_TEXT_inform,20,&test_dis_text0,0,0);	
					}
				else
					{ 
					memcpy(DIS_TEXT_temp.DIS_TEXT_inform,&TEXT_Obj_8[DIS_TEXT_temp.DIS_TEXT_screen_counter-1].TEXT_STR[40*(DIS_TEXT_temp.DIS_TEXT_screen_current_num-1)],DIS_TEXT_temp.DIS_TEXT_inform_len%40);//20������20*2���ֽ�
					DisAddRead_ZK(0,0,(char *)DIS_TEXT_temp.DIS_TEXT_inform,(DIS_TEXT_temp.DIS_TEXT_inform_len%40>>1),&test_dis_text0,0,0);	// ��ʾ���Ǻ�������������1λ��2
					}
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
		memset(&DIS_TEXT_temp,0,sizeof(DIS_TEXT_temp));
		}
}


MENUITEM	Menu_2_3_3_TextInforStor=
{
	"������Ϣ�鿴",
	&show,
	&keypress,
	&timetick,
	(void*)0
};


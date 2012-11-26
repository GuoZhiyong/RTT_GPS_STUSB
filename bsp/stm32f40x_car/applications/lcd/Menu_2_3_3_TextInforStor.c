#include "menu.h"
#include <string.h>

struct IMG_DEF test_dis_text0={12,12,test_00};

//-------文本信息-------
typedef struct _MSG_TEXT
{
  unsigned char   TEXT_mOld;     //  最新的一条信息  写为1代表是最新的一条信息
  unsigned char   TEXT_TYPE;     //  信息类型   1-8  中第几条
  unsigned char   TEXT_LEN;      //  信息长度    
  unsigned char   TEXT_STR[150]; //  信息内容
}MSG_TEXT;

MSG_TEXT       TEXT_Obj;
MSG_TEXT       TEXT_Obj_8[8],TEXT_Obj_8bak[8];

/*static unsigned char dis_screen=0;//文本信息主界面   =1选择要显示第几条消息(1-8)，=2显示消息时下翻显示
static unsigned char dis_screen_counter=0;//查看的是第几条信息(1-8)
static unsigned char TxtScreenNum_Total_0=0;//需要显示的消息总屏数
static unsigned char TxtScreen_CurrentNum_0=0;// 当前显示第几屏
static unsigned char read_temp_0[50];//要显示的信息内容，每屏最多显示40个字节
static unsigned int  TxtInfo_len_0=0;//要显示的信息长度
*/
typedef struct _DIS_TEXT
{
unsigned char DIS_TEXT_screen;//文本信息主界面   =1选择要显示第几条消息(1-8)，=2显示消息时下翻显示
unsigned char DIS_TEXT_screen_counter;//查看的是第几条信息(1-8)
unsigned char DIS_TEXT_screen_total_num;//需要显示的消息总屏数
unsigned char DIS_TEXT_screen_current_num;//当前显示第几屏
unsigned char DIS_TEXT_inform[40];//要显示的信息内容，每屏最多显示40个字节
unsigned int  DIS_TEXT_inform_len;//要显示的信息长度
}DIS_TEXT;

DIS_TEXT DIS_TEXT_temp;


void DIS_MEUN_1(unsigned char screen)
{
char InforNum[2]={"0."};
if((screen>=1)&&(screen<=8))
	{
	InforNum[0]='0'+screen;
	lcd_fill(0);
	DisAddRead_ZK(36, 3,"文字消息",4,&test_dis_text0,1,0);
	lcd_text(0, 19,FONT_NINE_DOT,(char *)InforNum);
	DisAddRead_ZK(20, 19,"消息内容查看",6,&test_dis_text0,1,0);
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
unsigned char CurrentDisplen=0;//显示汉字的个数

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
				DIS_TEXT_temp.DIS_TEXT_screen=2;//结合 dis_screen_counter 显示第n条信息
				//TEXT_Read();//读出要现实的信息
				DIS_TEXT_temp.DIS_TEXT_inform_len=TEXT_Obj_8[DIS_TEXT_temp.DIS_TEXT_screen_counter-1].TEXT_LEN;// 收到文本信息长度 
                if(DIS_TEXT_temp.DIS_TEXT_inform_len==0)
                	{
                	lcd_fill(0);
					lcd_text(45, 10,FONT_NINE_DOT,"[");
					DisAddRead_ZK(54,10,"空",1,&test_dis_text0,0,0); 
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
				//TEXT_Read();//读出要现实的信息
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
					memcpy(DIS_TEXT_temp.DIS_TEXT_inform,&TEXT_Obj_8[DIS_TEXT_temp.DIS_TEXT_screen_counter-1].TEXT_STR[40*(DIS_TEXT_temp.DIS_TEXT_screen_current_num-1)],40);//20个汉字20*2个字节
					DisAddRead_ZK(0,0,(char *)DIS_TEXT_temp.DIS_TEXT_inform,20,&test_dis_text0,0,0);	
					}
				else
					{ 
					memcpy(DIS_TEXT_temp.DIS_TEXT_inform,&TEXT_Obj_8[DIS_TEXT_temp.DIS_TEXT_screen_counter-1].TEXT_STR[40*(DIS_TEXT_temp.DIS_TEXT_screen_current_num-1)],DIS_TEXT_temp.DIS_TEXT_inform_len%40);//20个汉字20*2个字节
					DisAddRead_ZK(0,0,(char *)DIS_TEXT_temp.DIS_TEXT_inform,(DIS_TEXT_temp.DIS_TEXT_inform_len%40>>1),&test_dis_text0,0,0);	// 显示的是汉字书所以右移1位除2
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
	"文字消息查看",
	&show,
	&keypress,
	&timetick,
	(void*)0
};


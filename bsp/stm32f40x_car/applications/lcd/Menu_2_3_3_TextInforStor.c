#include "Menu_Include.h"
#include <string.h>


static unsigned char dis_screen=0;//�ı���Ϣ������1
static unsigned char dis_screen_counter=0;
static unsigned char TxtScreenNum_Total_0=0;// ��Ϣ��ʾ��Ļ����
static unsigned char TxtScreen_CurrentNum_0=0;// ��ǰ����Ŀ
static unsigned char read_temp_0[50];
static unsigned int  TxtInfo_len_0=0;


void DIS_MEUN_1(uint8_t screen)
{
char InforNum[20]={"0.��Ϣ���ݲ鿴"};
if((screen>=1)&&(screen<=8))
	{
	InforNum[0]='0'+screen;
	lcd_fill(0);
	lcd_text12(36, 3,"������Ϣ",8,LCD_MODE_SET);
	lcd_text12(0, 19,(char *)InforNum,14,LCD_MODE_SET);
	lcd_update_all();
	}
}

void DIS_MEUN_2(uint8_t screen)
{
char InforNum[2]={"0."};
if((screen>=1)&&(screen<=8))
	{
	InforNum[0]='0'+screen;
	lcd_fill(0);
	lcd_text12(0, 10,(char *)InforNum,2,LCD_MODE_SET);
	lcd_text12(20,10,(char *)TEXT_Obj_8[screen-1].TEXT_STR,TEXT_Obj_8[screen-1].TEXT_LEN,LCD_MODE_SET);
	lcd_update_all();
	}
}

static void show(void)
	{
	dis_screen=1;
	dis_screen_counter=1;
	/*if(READ_ONE==1)
		{
		READ_ONE=0;
		TEXT_Read();//����Ҫ��ʵ����Ϣ
		}*/
	DIS_MEUN_1(dis_screen_counter);
	}

static void keypress(unsigned int key)
{
unsigned char CurrentDisplen=0;

	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_1_InforCheck;
			pMenuItem->show();
			CounterBack=0;
            
			dis_screen=0;//�ı���Ϣ������1
			dis_screen_counter=0;
			TxtScreenNum_Total_0=0;// ��Ϣ��ʾ��Ļ����
			TxtScreen_CurrentNum_0=0;// ��ǰ����Ŀ			
			/*READ_ONE=1;*/

			break;
		case KeyValueOk:
			if(dis_screen==1)
				{
				dis_screen=2;//��� dis_screen_counter ��ʾ��n����Ϣ
				DIS_MEUN_2(dis_screen_counter);

				//========================================================
				TxtInfo_len_0=TEXT_Obj_8[dis_screen_counter-1].TEXT_LEN;// �յ��ı���Ϣ���� 
				//printf("\r\n ���ݳ��� = %d ",TxtInfo_len_0);
                if(TxtInfo_len_0==0)
                	{
                	lcd_fill(0);
					lcd_text12(45, 10,"[��]",4,LCD_MODE_SET);
					lcd_update_all();
                	}
				else
					{
					if(TxtInfo_len_0>40)
						{
						TxtScreen_CurrentNum_0++;
						if(TxtInfo_len_0%40)					
							TxtScreenNum_Total_0=TxtInfo_len_0/40+1;
						else
							TxtScreenNum_Total_0=TxtInfo_len_0/40;	

					    CurrentDisplen=40;
						//printf("\r\n д�뺺���� : %d�� ����=%d,��Ҫ��ʾ%d��,��ǰ��%d��",CurrentDisplen,TxtInfo_len_0,TxtScreenNum_Total_0,TxtScreen_CurrentNum_0);
						}
					else
						{
						TxtScreenNum_Total_0=1;
						CurrentDisplen=TxtInfo_len_0; 
						//printf("\r\n д�뺺���� : %d�� ����=%d,��Ҫ��ʾ%d��",CurrentDisplen,TxtInfo_len_0,TxtScreenNum_Total_0);
						}				
				
					lcd_fill(0);
					lcd_text12(0,0,(char *)TEXT_Obj_8[dis_screen_counter-1].TEXT_STR,CurrentDisplen,LCD_MODE_SET); 
					lcd_update_all();
					}
				//========================================================
				}
			else if(dis_screen==2)
				{
				dis_screen=1;
				//dis_screen_counter=1;
				TxtScreen_CurrentNum_0=0;
				DIS_MEUN_1(dis_screen_counter);
				/*TEXT_Read();//����Ҫ��ʵ����Ϣ*/
				}

			break;
		case KeyValueUP:
            if(dis_screen==1)
            	{
            	dis_screen_counter--;
				if(dis_screen_counter<=1)
					dis_screen_counter=1;
				DIS_MEUN_1(dis_screen_counter);
            	}		
			break;
		case KeyValueDown:
			 if(dis_screen==1)
            	{
            	dis_screen_counter++;
				if(dis_screen_counter>=8)
					dis_screen_counter=8;
				DIS_MEUN_1(dis_screen_counter);
            	}
			 //else if(dis_screen==2)//&&(TxtScreenNum_Total_0>1))
			 else if((dis_screen==2)&&(TxtInfo_len_0>0))
				{
				
				//====================================================
				TxtScreen_CurrentNum_0++;
				//printf("\r\n  0  ��ʾ��Ϣ�� %d ��,���� %d ��",TxtScreen_CurrentNum_0,TxtScreenNum_Total_0);
				if(TxtScreen_CurrentNum_0>=TxtScreenNum_Total_0)
					TxtScreen_CurrentNum_0=TxtScreenNum_Total_0;

				//printf("\r\n  2  ��ʾ��Ϣ�� %d ��,���� %d ��",TxtScreen_CurrentNum_0,TxtScreenNum_Total_0);
				memset(test_idle,0,sizeof(test_idle));
				lcd_fill(0);
				if(TxtScreen_CurrentNum_0!=TxtScreenNum_Total_0)
					{
					memcpy(read_temp_0,&TEXT_Obj_8[dis_screen_counter-1].TEXT_STR[40*(TxtScreen_CurrentNum_0-1)],40);//20������20*2���ֽ�
					lcd_text12(0,0,(char *)read_temp_0,40,LCD_MODE_SET);	

					}
				else
					{  
					//printf("\r\n  ���һ��");
					memcpy(read_temp_0,&TEXT_Obj_8[dis_screen_counter-1].TEXT_STR[40*(TxtScreen_CurrentNum_0-1)],TxtInfo_len_0%40);//20������20*2���ֽ�
					lcd_text12(0,0,(char *)read_temp_0,(TxtInfo_len_0%40),LCD_MODE_SET);	// ��ʾ���Ǻ�������������1λ��2
					}
				lcd_update_all();
				//=====================================================
				}
			break;
		}
 KeyValue=0;
}


static void timetick(unsigned int systick)
{
       Cent_To_Disp();
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
MENUITEM	Menu_2_3_3_TextInforStor=
{
	"�ı���Ϣ�鿴",
	12,
	&show,
	&keypress,
	&timetick,
	(void*)0
};


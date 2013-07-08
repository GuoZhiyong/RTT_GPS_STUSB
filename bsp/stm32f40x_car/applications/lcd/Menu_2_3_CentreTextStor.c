#include "Menu_Include.h"
#include <string.h>
#include "sed1520.h"

static unsigned char dis_screen=0;//�ı���Ϣ������1
static unsigned char dis_screen_counter=0;
static unsigned char TxtScreenNum_Total_0=0;// ��Ϣ��ʾ��Ļ����
static unsigned char TxtScreen_CurrentNum_0=0;// ��ǰ����Ŀ
static unsigned char read_temp_0[50];
static unsigned int  TxtInfo_len_0=0;


void DIS_MEUN_1(u8 screen)
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

void DIS_MEUN_2(u8 screen)
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
static void msg( void *p)
{
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
u8 CurrentDisplen=0;
u8 i=0;

	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_2_InforCheck;
			pMenuItem->show();
			CounterBack=0;
            
			dis_screen=0;//�ı���Ϣ������1
			dis_screen_counter=0;
			TxtScreenNum_Total_0=0;// ��Ϣ��ʾ��Ļ����
			TxtScreen_CurrentNum_0=0;// ��ǰ����Ŀ	
			TxtInfo_len_0=0;
			/*READ_ONE=1;*/

			break;
		case KeyValueOk:
			if(dis_screen==1)
				{
				dis_screen=2;//��� dis_screen_counter ��ʾ��n����Ϣ
				//DIS_MEUN_2(dis_screen_counter);

				for(i=0;i<=7;i++)
					{
					Api_RecordNum_Read(text_msg,i+1, (u8*)&TEXT_Obj,sizeof(TEXT_Obj)); 			
					memcpy((u8*)&TEXT_Obj_8[i],(u8*)&TEXT_Obj,sizeof(TEXT_Obj));     
					//rt_kprintf("\r\n�ı���Ϣ ����:%d  ��ϢTYPE:%d  ����:%d",TEXT_Obj_8bak[i].TEXT_mOld,TEXT_Obj_8bak[i].TEXT_TYPE,TEXT_Obj_8bak[i].TEXT_LEN);  
					}
				//========================================================
				TxtInfo_len_0=TEXT_Obj_8[dis_screen_counter-1].TEXT_LEN;// �յ��ı���Ϣ���� 
				//rt_kprintf("\r\n ���ݳ��� = %d ",TxtInfo_len_0);
                if(TxtInfo_len_0==0)
                	{
                	lcd_fill(0);
					lcd_text12(45, 10,"[��]",4,LCD_MODE_SET);
					lcd_update_all();
                	}
				else
					{
					lcd_fill(0);
					if(TxtInfo_len_0>40)
						{
						TxtScreen_CurrentNum_0++;
						if(TxtInfo_len_0%40)					
							TxtScreenNum_Total_0=TxtInfo_len_0/40+1;
						else
							TxtScreenNum_Total_0=TxtInfo_len_0/40;	
					    lcd_text12(0, 0,(char *) TEXT_Obj_8[dis_screen_counter-1].TEXT_STR,   20,LCD_MODE_SET); 
                        lcd_text12(0,15,(char *)&TEXT_Obj_8[dis_screen_counter-1].TEXT_STR+20,20,LCD_MODE_SET);   
						//rt_kprintf("\r\n д�뺺���� : %d�� ����=%d,��Ҫ��ʾ%d��,��ǰ��%d��",CurrentDisplen,TxtInfo_len_0,TxtScreenNum_Total_0,TxtScreen_CurrentNum_0);
						}
					else
						{
						TxtScreenNum_Total_0=1;
						if(TxtInfo_len_0>20)
							{
						    lcd_text12(0, 0,(char *)TEXT_Obj_8[dis_screen_counter-1].TEXT_STR,   20,LCD_MODE_SET); 
	                        lcd_text12(0,15,(char *)TEXT_Obj_8[dis_screen_counter-1].TEXT_STR+20,TxtInfo_len_0-20,LCD_MODE_SET);   
							}
						else
						    lcd_text12(0, 0,(char *) TEXT_Obj_8[dis_screen_counter-1].TEXT_STR,TxtInfo_len_0,LCD_MODE_SET);  
						//rt_kprintf("\r\n д�뺺���� : %d�� ����=%d,��Ҫ��ʾ%d��",CurrentDisplen,TxtInfo_len_0,TxtScreenNum_Total_0);
						}				
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
				//rt_kprintf("\r\n  0  ��ʾ��Ϣ�� %d ��,���� %d ��",TxtScreen_CurrentNum_0,TxtScreenNum_Total_0);
				if(TxtScreen_CurrentNum_0>=TxtScreenNum_Total_0)
					TxtScreen_CurrentNum_0=TxtScreenNum_Total_0;

				//rt_kprintf("\r\n  2  ��ʾ��Ϣ�� %d ��,���� %d ��",TxtScreen_CurrentNum_0,TxtScreenNum_Total_0);
				lcd_fill(0);
				if(TxtScreen_CurrentNum_0!=TxtScreenNum_Total_0)
					{
					memcpy(read_temp_0,&TEXT_Obj_8[dis_screen_counter-1].TEXT_STR[40*(TxtScreen_CurrentNum_0-1)],40);//20������20*2���ֽ�
					lcd_text12(0,0,(char *)read_temp_0,20,LCD_MODE_SET);
					lcd_text12(0,15,(char *)read_temp_0+20,20,LCD_MODE_SET);

					}
				else
					{  
					//rt_kprintf("\r\n  ���һ��");
					CurrentDisplen=TxtInfo_len_0%40;
					memcpy(read_temp_0,&TEXT_Obj_8[dis_screen_counter-1].TEXT_STR[40*(TxtScreen_CurrentNum_0-1)],CurrentDisplen);//20������20*2���ֽ�
                    if(CurrentDisplen>20)
                    	{
                    	lcd_text12(0,0,(char *)read_temp_0,20,LCD_MODE_SET);
						lcd_text12(0,0,(char *)read_temp_0+20,CurrentDisplen-20,LCD_MODE_SET);
                    	}
					else
					    lcd_text12(0,0,(char *)read_temp_0,CurrentDisplen,LCD_MODE_SET);	// ��ʾ���Ǻ�������������1λ��2
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

		dis_screen=0;//�ı���Ϣ������1
		dis_screen_counter=0;
		TxtScreenNum_Total_0=0;// ��Ϣ��ʾ��Ļ����
		TxtScreen_CurrentNum_0=0;// ��ǰ����Ŀ
		TxtInfo_len_0=0;
		}
}


MENUITEM	Menu_2_3_CentreTextStor=
{
	"�ı���Ϣ�鿴",
	12,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};


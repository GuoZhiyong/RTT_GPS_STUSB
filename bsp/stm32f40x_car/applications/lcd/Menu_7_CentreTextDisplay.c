#include "Menu_Include.h"
#include <string.h>

static unsigned char Menu_Text=0;//==1ѡ��Ҫ��ʾ����Ϣ,ѡ��Ҫ��ʾ����Ϣ��Ϊ2
static int TxtInfo_len=0;//Ҫ��ʾ����Ϣ����
static unsigned char  TxtScreenNum_Total=0;// ��Ϣ��ʾ��Ļ����
static unsigned char  TxtScreen_CurrentNum=0;// ��ǰ����Ŀ
static unsigned char read_temp[50];



static void msg( void *p)
{

}
static void show(void)
	{
	lcd_fill(0);
	lcd_text12(20,3,"����һ������Ϣ",14,LCD_MODE_SET);
	lcd_text12(26,19,"��ȷ�ϼ��鿴",12,LCD_MODE_SET);
	lcd_update_all();
	}

static void keypress(unsigned int key)
{
unsigned char CurrentDisplen=0;

	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_1_Idle;
			pMenuItem->show();
			CounterBack=0;

			Menu_Text=0;
			TxtInfo_len=0;//Ҫ��ʾ����Ϣ����

			break;
		case KeyValueOk:
			if(Menu_Text==0)
				{
				rt_kprintf("\r\n��Ҫ��ʾ������:%s",TextInfo.TEXT_Content);
				TxtInfo_len=strlen((const char*)TextInfo.TEXT_Content);// �յ��ı���Ϣ���� 
				if(TxtInfo_len%40)					
				  TxtScreenNum_Total=TxtInfo_len/40+1;
				else
				  TxtScreenNum_Total=TxtInfo_len/40;	
				
                if(TxtScreenNum_Total>1)	// �ж��Ƿ����1��			 	
					{ 
				    Menu_Text=1;
					lcd_fill(0);
					lcd_text12(0, 0,(char *)TextInfo.TEXT_Content,20,LCD_MODE_SET); 
					lcd_text12(0,15,(char *)TextInfo.TEXT_Content+20,20,LCD_MODE_SET);
					lcd_update_all();
	                }	
				else
					{
					CurrentDisplen=strlen((char *)TextInfo.TEXT_Content); 
					if(CurrentDisplen>20)
						{
						lcd_fill(0);
						lcd_text12(0, 0,(char *)TextInfo.TEXT_Content,20,LCD_MODE_SET); 
						lcd_text12(0,15,(char *)TextInfo.TEXT_Content+20,CurrentDisplen-20,LCD_MODE_SET);
						lcd_update_all();
						}
					else
						{
						lcd_fill(0);
						lcd_text12(0, 0,(char *)TextInfo.TEXT_Content,CurrentDisplen,LCD_MODE_SET); 
						lcd_update_all();
						}
					}
                TxtScreen_CurrentNum=1; // ����ǰ���ǵ�һ��
				}
			break;
		case KeyValueUP:
			if(Menu_Text==1)
				{
				if(TxtScreen_CurrentNum>1)
				   TxtScreen_CurrentNum--;
				
				lcd_fill(0);
				if(TxtScreen_CurrentNum!=TxtScreenNum_Total)
					{
					memcpy(read_temp,&TextInfo.TEXT_Content[40*(TxtScreen_CurrentNum-1)],40);//20������20*2���ֽ�
					lcd_text12(0, 0,(char *)read_temp,20,LCD_MODE_SET);	
					lcd_text12(0,15,(char *)read_temp+20,20,LCD_MODE_SET);
					}
				else
					{   
				   CurrentDisplen=TxtInfo_len%40;
				   memcpy(read_temp,&TextInfo.TEXT_Content[40*(TxtScreen_CurrentNum-1)],CurrentDisplen);//20������20*2���ֽ�
                   if(CurrentDisplen>20)
	                   	{
	                   	lcd_text12(0, 0,(char *)read_temp,20,LCD_MODE_SET);	// ��ʾ���Ǻ�������������1λ��2
                        lcd_text12(0,15,(char *)read_temp,CurrentDisplen-20,LCD_MODE_SET);	// ��ʾ���Ǻ�������������1λ��2
                        }
				   else
					    lcd_text12(0,0,(char *)read_temp,CurrentDisplen,LCD_MODE_SET);	// ��ʾ���Ǻ�������������1λ��2
                  
					}
				lcd_update_all();
				}
			break;
		case KeyValueDown:
			if(Menu_Text==1)
				{
				TxtScreen_CurrentNum++;
				if(TxtScreen_CurrentNum>TxtScreenNum_Total)
					{ 
				    TxtScreen_CurrentNum=TxtScreenNum_Total;
				    break;
					}
				lcd_fill(0);
				if(TxtScreen_CurrentNum!=TxtScreenNum_Total)
					{
					memcpy(read_temp,&TextInfo.TEXT_Content[40*(TxtScreen_CurrentNum-1)],40);//20������20*2���ֽ�
					lcd_text12(0, 0,(char *)read_temp,20,LCD_MODE_SET);	
					lcd_text12(0,15,(char *)read_temp+20,20,LCD_MODE_SET);
					//���ڵĺ���ֻ��д1�У���Ҫ�жϷֿ�д2��
					}
				else
					{   
				   CurrentDisplen=TxtInfo_len%40;
				   memcpy(read_temp,&TextInfo.TEXT_Content[40*(TxtScreen_CurrentNum-1)],CurrentDisplen);//20������20*2���ֽ�
                   if(CurrentDisplen>20)
	                   	{
	                   	lcd_text12(0, 0,(char *)read_temp,20,LCD_MODE_SET);	// ��ʾ���Ǻ�������������1λ��2
                        lcd_text12(0,15,(char *)read_temp,CurrentDisplen-20,LCD_MODE_SET);	// ��ʾ���Ǻ�������������1λ��2
                        }
				   else
					    lcd_text12(0,0,(char *)read_temp,CurrentDisplen,LCD_MODE_SET);	// ��ʾ���Ǻ�������������1λ��2
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
		}
}

MYTIME
MENUITEM	Menu_7_CentreTextDisplay=
{
    "�����·���Ϣ",
	12,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};


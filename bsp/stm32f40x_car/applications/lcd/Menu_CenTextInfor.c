#include "Menu_Include.h"
#include <string.h>
#include "jt808.h"

static unsigned char Menu_Text=0;//==1ѡ��Ҫ��ʾ����Ϣ,ѡ��Ҫ��ʾ����Ϣ��Ϊ2
static int TxtInfo_len=0;//Ҫ��ʾ����Ϣ����
static unsigned char  TxtScreenNum_Total=0;// ��Ϣ��ʾ��Ļ����
static unsigned char  TxtScreen_CurrentNum=0;// ��ǰ����Ŀ
static unsigned char read_temp[50];




static void show(void)
	{
	memset(test_idle,0,sizeof(test_idle));
	lcd_fill(0);
	lcd_text12(20,3,"����һ������Ϣ",14,LCD_MODE_SET);
	lcd_text12(26,19,"��ȷ�ϼ��鿴",12,LCD_MODE_SET);
	lcd_update_all();
	}

static void keypress(unsigned int key)
{
unsigned char CurrentDisplen=0;;

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
				TxtInfo_len=strlen((const char*)TextInfo.TEXT_Content);// �յ��ı���Ϣ���� 
				if(TxtInfo_len%40)					
				  TxtScreenNum_Total=TxtInfo_len/40+1;
				else
				  TxtScreenNum_Total=TxtInfo_len/40;	
				
                if(TxtScreenNum_Total>1)	// �ж��Ƿ����1��			 	
				 { 
				    Menu_Text=1;
					CurrentDisplen=20;
                 }	
				else
					CurrentDisplen=strlen((char *)TextInfo.TEXT_Content)/2; 
                TxtScreen_CurrentNum=1; // ����ǰ���ǵ�һ��
                
				//���ڵĺ���ֻ��д1�У���Ҫ�жϷֿ�д2��
				lcd_fill(0);
				lcd_text12(0,0,(char *)TextInfo.TEXT_Content,CurrentDisplen,LCD_MODE_SET); 
				lcd_update_all();
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
					lcd_text12(0,0,(char *)read_temp,20,LCD_MODE_SET);	
					//���ڵĺ���ֻ��д1�У���Ҫ�жϷֿ�д2��
				}
				else
				{   
				   memcpy(read_temp,&TextInfo.TEXT_Content[40*(TxtScreen_CurrentNum-1)],TxtInfo_len%40);//20������20*2���ֽ�
				   lcd_text12(0,0,(char *)read_temp,(TxtInfo_len%40>>1),LCD_MODE_SET);	// ��ʾ���Ǻ�������������1λ��2
                   //���ڵĺ���ֻ��д1�У���Ҫ�жϷֿ�д2��
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
				
				memset(test_idle,0,sizeof(test_idle));
				lcd_fill(0);
				if(TxtScreen_CurrentNum!=TxtScreenNum_Total)
				{
					memcpy(read_temp,&TextInfo.TEXT_Content[40*(TxtScreen_CurrentNum-1)],40);//20������20*2���ֽ�
					lcd_text12(0,0,(char *)read_temp,20,LCD_MODE_SET);	
					//���ڵĺ���ֻ��д1�У���Ҫ�жϷֿ�д2��
				}
				else
				{   
				   memcpy(read_temp,&TextInfo.TEXT_Content[40*(TxtScreen_CurrentNum-1)],TxtInfo_len%40);//20������20*2���ֽ�
				   lcd_text12(0,0,(char *)read_temp,(TxtInfo_len%40>>1),LCD_MODE_SET);	// ��ʾ���Ǻ�������������1λ��2
				   //���ڵĺ���ֻ��д1�У���Ҫ�жϷֿ�д2��
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

ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_CenterTextInfor=
{
    "�����·���Ϣ",
	12,
	&show,
	&keypress,
	&timetick,
	(void*)0
};


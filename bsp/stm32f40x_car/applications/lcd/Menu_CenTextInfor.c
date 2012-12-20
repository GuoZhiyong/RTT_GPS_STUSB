#include "Menu_Include.h"
#include <string.h>
#include "jt808.h"

static unsigned char Menu_Text=0;//==1选择要显示的信息,选好要显示的信息后为2
static int TxtInfo_len=0;//要显示的信息长度
static unsigned char  TxtScreenNum_Total=0;// 消息显示屏幕总数
static unsigned char  TxtScreen_CurrentNum=0;// 当前屏数目
static unsigned char read_temp[50];




static void show(void)
	{
	memset(test_idle,0,sizeof(test_idle));
	lcd_fill(0);
	lcd_text12(20,3,"您有一条新消息",14,LCD_MODE_SET);
	lcd_text12(26,19,"按确认键查看",12,LCD_MODE_SET);
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
			TxtInfo_len=0;//要显示的信息长度

			break;
		case KeyValueOk:
			if(Menu_Text==0)
				{
				TxtInfo_len=strlen((const char*)TextInfo.TEXT_Content);// 收到文本信息长度 
				if(TxtInfo_len%40)					
				  TxtScreenNum_Total=TxtInfo_len/40+1;
				else
				  TxtScreenNum_Total=TxtInfo_len/40;	
				
                if(TxtScreenNum_Total>1)	// 判断是否大于1屏			 	
				 { 
				    Menu_Text=1;
					CurrentDisplen=20;
                 }	
				else
					CurrentDisplen=strlen((char *)TextInfo.TEXT_Content)/2; 
                TxtScreen_CurrentNum=1; // 代表当前屏是第一屏
                
				//现在的函数只能写1行，需要判断分开写2次
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
					memcpy(read_temp,&TextInfo.TEXT_Content[40*(TxtScreen_CurrentNum-1)],40);//20个汉字20*2个字节
					lcd_text12(0,0,(char *)read_temp,20,LCD_MODE_SET);	
					//现在的函数只能写1行，需要判断分开写2次
				}
				else
				{   
				   memcpy(read_temp,&TextInfo.TEXT_Content[40*(TxtScreen_CurrentNum-1)],TxtInfo_len%40);//20个汉字20*2个字节
				   lcd_text12(0,0,(char *)read_temp,(TxtInfo_len%40>>1),LCD_MODE_SET);	// 显示的是汉字书所以右移1位除2
                   //现在的函数只能写1行，需要判断分开写2次
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
					memcpy(read_temp,&TextInfo.TEXT_Content[40*(TxtScreen_CurrentNum-1)],40);//20个汉字20*2个字节
					lcd_text12(0,0,(char *)read_temp,20,LCD_MODE_SET);	
					//现在的函数只能写1行，需要判断分开写2次
				}
				else
				{   
				   memcpy(read_temp,&TextInfo.TEXT_Content[40*(TxtScreen_CurrentNum-1)],TxtInfo_len%40);//20个汉字20*2个字节
				   lcd_text12(0,0,(char *)read_temp,(TxtInfo_len%40>>1),LCD_MODE_SET);	// 显示的是汉字书所以右移1位除2
				   //现在的函数只能写1行，需要判断分开写2次
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
    "中心下发消息",
	12,
	&show,
	&keypress,
	&timetick,
	(void*)0
};


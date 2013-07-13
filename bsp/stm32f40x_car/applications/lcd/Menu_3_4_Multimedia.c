#include "Menu_Include.h"
#include "sed1520.h"


unsigned char Menu_Multimedia=0;
unsigned char Multimedia_change=1;//ѡ��
unsigned char Multimedia_screen=0;//�����л�ʹ��


void Multimedia(unsigned char type)
{
	lcd_fill(0);
	lcd_text12(18,3,"��ý����Ϣѡ��",14,LCD_MODE_SET);	
	if(type==1)
		{
		lcd_text12(24,19,"��Ƶ",4,LCD_MODE_INVERT);
		lcd_text12(72,19,"��Ƶ",4,LCD_MODE_SET);
		}
	else if(type==2)
		{
		lcd_text12(24,19,"��Ƶ",4,LCD_MODE_SET);
		lcd_text12(72,19,"��Ƶ",4,LCD_MODE_INVERT);
		}
	lcd_update_all();

}

static void msg( void *p)
{
}
static void show(void)
{
	Multimedia(1);
}


static void keypress(unsigned int key)
{
switch(key)
	{
	case KEY_MENU:
		pMenuItem=&Menu_3_InforInteract;
		pMenuItem->show();
		CounterBack=0;
		
		Menu_Multimedia=0;
		Multimedia_change=1;//ѡ��
		Multimedia_screen=0;//�����л�ʹ��
		break;
	case KEY_OK:
		if(Multimedia_screen==0)
			{
			Multimedia_screen=1;
			lcd_fill(0);
			
			if(Multimedia_change==1)
				{
				//CarLoadState_Write();
				lcd_text12(7,10,"��ý����������:��Ƶ",19,LCD_MODE_SET);  
				}
			else if(Multimedia_change==2)
				{
				//CarLoadState_Write(); 
				lcd_text12(7,10,"��ý����������:��Ƶ",19,LCD_MODE_SET);  
				}
			lcd_update_all();
			}
		else if(Multimedia_screen==1)
			{
			Multimedia_screen=2;

//bitter:			MediaObj.SD_Data_Flag=1;//���Ͷ�ý�����ݱ�־
			if(Multimedia_change==1)
				{
//bitter:					MediaObj.Media_Type=1;//��Ƶ1
				/*memset(send_data,0,sizeof(send_data));
				send_data[0]=0x08;
				send_data[1]=0x01;
				send_data[2]=0x00;
				send_data[3]=0x01;
				send_data[4]=0x01;//��Ƶ
				rt_mb_send(&mb_hmi, (rt_uint32_t)&send_data[0]);*/
				
				lcd_fill(0);
				lcd_text12(18,10,"��Ƶ  ��ʼ����",14,LCD_MODE_SET);
				lcd_update_all();
				}
			else if(Multimedia_change==2)
				{
//bitter:					MediaObj.Media_Type=2;//��Ƶ2
				/*memset(send_data,0,sizeof(send_data));
				send_data[0]=0x08;
				send_data[1]=0x01;
				send_data[2]=0x00;
				send_data[3]=0x01;
				send_data[4]=0x02;//��Ƶ
				rt_mb_send(&mb_hmi, (rt_uint32_t)&send_data[0]);*/
				
				lcd_fill(0);
				lcd_text12(18,10,"��Ƶ  ��ʼ����",14,LCD_MODE_SET);	
				lcd_update_all();
				}
			
			}
		break;
	case KEY_UP:
		if(Multimedia_screen==0)
			{
			Multimedia_change=1;
			Multimedia(1);
			}
		break;
	case KEY_DOWN:
		if(Multimedia_screen==0)
			{
			Multimedia_change=2;
			Multimedia(2);
			}
		break;
	}
}



MENUITEM	Menu_3_4_Multimedia=
{
	"���Ͷ�ý������",
	14,0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};


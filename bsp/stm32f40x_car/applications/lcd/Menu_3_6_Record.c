#include "Menu_Include.h"
#include "sed1520.h"


unsigned char Record_screen=0;//����¼������=1,׼�����Ϳ�ʼ/����ʱΪ2   ��ʼ/�����ѷ���Ϊ3    ����һ����ָ�����ֵ
unsigned char Record_StartEnd=0;//==1¼����ʼ   ==2¼������   ����һ����ָ�����ֵ

void record_sel(unsigned char sel)
{
	lcd_fill(0);
	lcd_text12(36,3,"¼��ѡ��",8,LCD_MODE_SET);					  
	if(sel==1)
		{
		lcd_text12(24,19,"��ʼ",4,LCD_MODE_INVERT);
		lcd_text12(72,19,"����",4,LCD_MODE_SET);
		}
    else
    	{
		lcd_text12(24,19,"��ʼ",4,LCD_MODE_SET);
		lcd_text12(72,19,"����",4,LCD_MODE_INVERT);
    	}
    lcd_update_all();
}

static void msg( void *p)
{
}
static void show(void)
{
	record_sel(1);
	Record_screen=1;
	Record_StartEnd=1;
}


static void keypress(unsigned int key)
{
switch(key)
	{
	case KEY_MENU:
		pMenuItem=&Menu_3_InforInteract;
		pMenuItem->show();
		CounterBack=0;
		    
		Record_screen=0;//����¼������=1,׼�����Ϳ�ʼ/����ʱΪ2	 ��ʼ/�����ѷ���Ϊ3    ����һ����ָ�����ֵ
		Record_StartEnd=0;//==1¼����ʼ	 ==2¼������   ����һ����ָ�����ֵ
		break;
	case KEY_OK:
		if(Record_screen==1)
			{
			Record_screen=2;
			lcd_fill(0);
			if(Record_StartEnd==1)
				lcd_text12(10,10,"����¼��ѡ��:��ʼ",17,LCD_MODE_SET);
			else if(Record_StartEnd==2)
				lcd_text12(10,10,"����¼��ѡ��:����",17,LCD_MODE_SET);
			lcd_update_all();
			}
		else if(Record_screen==2)
			{
			Record_screen=3;
			lcd_fill(0);
			if(Record_StartEnd==1)
				lcd_text12(18,10,"��ʼ¼���ѷ���",14,LCD_MODE_SET);
			else if(Record_StartEnd==2)
				lcd_text12(18,10,"����¼���ѷ���",14,LCD_MODE_SET);
			lcd_update_all();
			}
		else if(Record_screen==3)//�ص�¼����ʼ/¼����������
			{
			Record_screen=1;
			Record_StartEnd=1;

			record_sel(1);
			}

		break;
	case KEY_UP:
        if(Record_screen==1)
        	{
        	Record_StartEnd=1;
			record_sel(1);
        	}
		break;
	case KEY_DOWN:
         if(Record_screen==1)
        	{
        	Record_StartEnd=2;
			record_sel(2);
        	}
		break;
	}

}

	

MENUITEM	Menu_3_6_Record=
{
	"¼����ʼ����",
	12,0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};


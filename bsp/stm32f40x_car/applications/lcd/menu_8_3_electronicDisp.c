#include "Menu_Include.h"

u8 elecList_screen=0;
u8 elecList_NUM_MAX=0;
u8 elecList_NUM=0;
u8 list_len=0;
u8 list_str[210]={"���˵�λ: �����һ��ͨ�Ź㲥���޹�˾  �绰:022-26237216 ���˵�λ: ����������乫˾  �绰:022-86692666 ��Ʒ����:GPS�����ն�  ��װ��ʽ: ��ʽ  ÿ������: 20  ����: 30��  ����: ��ʽС����  �˴����� :  2012-1-11   "};

static void msg( void *p)
{
}
static void show(void)
	{
	if(elecList_screen==0)
		{
	    elecList_screen=1;
		elecList_NUM=0;
		list_len=strlen(list_str);
		if(list_len%40)
			elecList_NUM_MAX=list_len/40+1;
		else
			elecList_NUM_MAX=list_len/40;
		rt_kprintf("\r\n��Ϣ����=%d,��Ҫ��ʾN��=%d",list_len,elecList_NUM_MAX);
		lcd_fill(0);
		lcd_text12(24,10,"�����˵��鿴",12,LCD_MODE_SET);
		lcd_update_all();
		}
	}

static void keypress(unsigned int key)
{
u8 disp_infor[50],disp_len=0;

	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_8_bd808new;
			pMenuItem->show();
			
			elecList_screen=0;
			elecList_NUM_MAX=0;
			elecList_NUM=0;
			list_len=0;
			break;
		case KeyValueOk:
			if(elecList_screen==1)
				{
				elecList_screen=2;
				elecList_NUM=0;
				memcpy(disp_infor,list_str,40);
				lcd_fill(0);
				lcd_text12(0,3,(char *)disp_infor,20,LCD_MODE_SET);
				lcd_text12(0,18,(char *)disp_infor+20,20,LCD_MODE_SET);
				lcd_update_all();
				}
			break;
		case KeyValueUP:
			if(elecList_screen==2)
				{
				if(elecList_NUM>0)
					elecList_NUM--;
				else
					elecList_NUM=0;
				memcpy(disp_infor,list_str+elecList_NUM*40,40);
				lcd_fill(0);
				lcd_text12(0,3,(char *)disp_infor,20,LCD_MODE_SET);
				lcd_text12(0,18,(char *)disp_infor+20,20,LCD_MODE_SET);
				lcd_update_all();
				}
			break;
		case KeyValueDown:
			if(elecList_screen==2)
				{
				if(elecList_NUM>=(elecList_NUM_MAX-1))
					elecList_NUM=elecList_NUM_MAX-1;
				else
					elecList_NUM++;
				memcpy(disp_infor,list_str+elecList_NUM*40,40);
				lcd_fill(0);
				if(elecList_NUM==(elecList_NUM_MAX-1))
					{
					disp_len=list_len%40;
					if(disp_len<20)
						lcd_text12(0,3,(char *)disp_infor,disp_len,LCD_MODE_SET);
					else
						{
						lcd_text12(0,3,(char *)disp_infor,20,LCD_MODE_SET);
				        lcd_text12(0,18,(char *)disp_infor+20,disp_len-20,LCD_MODE_SET);
						}
					}
				else
					{
					lcd_text12(0,3,(char *)disp_infor,20,LCD_MODE_SET);
				    lcd_text12(0,18,(char *)disp_infor+20,20,LCD_MODE_SET);
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
		elecList_screen=0;
		elecList_NUM_MAX=0;
		elecList_NUM=0;
		list_len=0;
		}
}

ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_8_3_electronicDisp=
{
    "�����˵���ʾ",
	12,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};


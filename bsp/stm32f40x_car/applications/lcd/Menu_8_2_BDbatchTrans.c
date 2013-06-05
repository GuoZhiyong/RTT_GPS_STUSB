#include "Menu_Include.h"

u8 batch_screen=0;
static void msg( void *p)
{
}
static void show(void)
	{
	batch_screen=1;
	lcd_fill(0);
	lcd_text12(12,10,"��λ���������ϴ�",16,LCD_MODE_SET);
	lcd_update_all();
	}

static void keypress(unsigned int key)
{
	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_8_bd808new;
			pMenuItem->show();
			batch_screen=0;
			break;
		case KeyValueOk:
			if(batch_screen==1)
				{
				batch_screen=2;
				//����Ӧ��־λ�ϴ���������
				//SD_ACKflag.f_BD_BatchTrans_0704H=1;
				lcd_fill(0);
				lcd_text12(12,10,"�����ϴ��������",16,LCD_MODE_SET);
				lcd_update_all();
				}
			break;
		case KeyValueUP:
			break;
		case KeyValueDown:
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
		batch_screen=0;
		}
}

ALIGN(RT_ALIGN_SIZE)
MENUITEM	Menu_8_2_BDbatchTrans=
{
    "�����ϴ�����",
	12,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};


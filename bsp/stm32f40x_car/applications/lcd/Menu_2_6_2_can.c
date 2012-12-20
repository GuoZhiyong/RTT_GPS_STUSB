#include "Menu_Include.h"

u8 can_screen=0;
u8 can_counter=1;
u8 can_ID_counter=0;
u8 CAN_baud[13]={"波特率:009600"};
u8 CAN_ID1[13]={"CAN1:00000001"};
u8 CAN_ID2[13]={"CAN2:00000002"};
void can_select(u8  par )
{
lcd_fill(0);
if(par==1)
	{	
	lcd_text12(20, 3,"CAN ID设置",10,LCD_MODE_INVERT);
	lcd_text12(20,19,"CAN 波特率查询",14,LCD_MODE_SET);
	}
else
    {
	lcd_text12(20, 3,"CAN ID设置",10,LCD_MODE_SET);
	lcd_text12(20,19,"CAN 波特率查询",14,LCD_MODE_INVERT);
	}
lcd_update_all();
}

void can_set_check(u8  par )
{
lcd_fill(0);
if(par==1)
	{	
	lcd_text12(0, 3,CAN_ID1,13,LCD_MODE_SET);
	lcd_text12(0,19,CAN_ID2,13,LCD_MODE_SET);
	}
else
	lcd_text12(0,10,CAN_baud,13,LCD_MODE_SET);
lcd_update_all();
}

static void show(void)
{
	can_select(1);
}


static void keypress(unsigned int key)
{

	switch(KeyValue)
		{
		case KeyValueMenu:
			pMenuItem=&Menu_2_6_3_concuss;
			pMenuItem->show();
			CounterBack=0;

			can_counter=1;
			break;
		case KeyValueOk:
			if(can_screen==0)
				{
			       can_screen=1;
				can_set_check(can_counter);
				}
			else if((can_screen==1)&&((can_counter==2)))
				{
				can_screen=0;
				can_select(can_counter);
				}
			//设置can ID
			else if((can_screen==1)&&((can_counter==1)))
				{
				rt_kprintf("\r\n");
				//can_screen=0;
				//can_select(can_counter);
				}
			break;
		case KeyValueUP:
			if(can_screen==0)
				{
				can_counter=1;
				can_select(can_counter);
				}
			if((can_screen==1)&&(can_counter==1))
				{
				}
			break;
		case KeyValueDown:
			if(can_screen==0)
				{
				can_counter=2;
				can_select(can_counter);
				}
			break;
		}
 KeyValue=0;
}


static void timetick(unsigned int systick)
{
    Cent_To_Disp();
	CounterBack++;
	if(CounterBack!=MaxBankIdleTime*5)
		return;
	CounterBack=0;
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();
	
}


MENUITEM	Menu_2_6_2_can=
{
"CAN设置",
	7,
	&show,
	&keypress,
	&timetick,
	(void*)0
};


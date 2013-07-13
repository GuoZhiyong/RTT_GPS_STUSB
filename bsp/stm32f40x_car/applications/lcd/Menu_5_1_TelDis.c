#include "Menu_Include.h"
#include "sed1520.h"

unsigned char Menu_TelText=0;
unsigned char TelText_scree=0;

unsigned char Tel_num_code[10][2]={"0","1","2","3","4","5","6","7","8","9"};



void Dis_TelText(unsigned char screen)
	{
	lcd_fill(0);
	lcd_text12(0,3,(char *)Tel_num_code[screen],1,LCD_MODE_SET);
#if NEED_TODO
		if(PhoneBook_8[screen-1].Effective_Flag)
		{
		lcd_text12(15,3,(char*)PhoneBook_8[screen-1].UserStr,strlen((char*)PhoneBook_8[screen-1].UserStr),LCD_MODE_SET);
		lcd_text12(40,19,(char *)PhoneBook_8[screen-1].NumberStr,strlen((char*)PhoneBook_8[screen-1].NumberStr),LCD_MODE_SET);
		}
	else
		lcd_text12(50,19,"Null",4, LCD_MODE_SET);
#endif	
	lcd_update_all();
	}

static void msg( void *p)
{
}
static void show(void)
	{
	lcd_fill(0);
	lcd_text12(20,10,"电话本记录查看",14,LCD_MODE_SET);
	lcd_update_all();
	Menu_TelText=1;
	}

static void keypress(unsigned int key)
{
	switch(key)
		{
		case KEY_MENU:
			pMenuItem=&Menu_5_other;
			pMenuItem->show();
			CounterBack=0;

			Menu_TelText=0;
            TelText_scree=0;
			break;
		case KEY_OK:
			/*if(Menu_TelText==0)
				{
				//PhoneBook_Read();
				Menu_TelText=1;
				}
			else */
			if(Menu_TelText==1)
				{
//bitter:				PhoneBook_Read();
				Menu_TelText=2;
				TelText_scree=1;
				Dis_TelText(1);
				}
			break;
		case KEY_UP:
			if(Menu_TelText==2)
				{
				TelText_scree--;
				if(TelText_scree<=1)
					TelText_scree=1;
				Dis_TelText(TelText_scree);
				}
			break;
		case KEY_DOWN:
			if(Menu_TelText==2)
				{
				TelText_scree++;
				if(TelText_scree>=8)
					TelText_scree=8;
				Dis_TelText(TelText_scree);
				}
			break;
		}
}


MENUITEM	Menu_5_1_TelDis=
{
    "电话本查看",
	10,0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};



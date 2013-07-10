#include "Menu_Include.h"
#include <string.h>
#include "sed1520.h"

unsigned char noselect_5[]={0x3C,0x7E,0xC3,0xC3,0xC3,0xC3,0x7E,0x3C};//空心
unsigned char select_5[]={0x3C,0x7E,0xFF,0xFF,0xFF,0xFF,0x7E,0x3C};//实心

DECL_BMP(8,8,select_5); DECL_BMP(8,8,noselect_5); 

static unsigned char menu_pos=0;
static PMENUITEM psubmenu[8]=
{
	&Menu_5_1_TelDis,
	&Menu_5_2_TelAtd,
	&Menu_5_3_bdupgrade,
	&Menu_5_4_bdColdBoot,
	&Menu_5_5_can,
	&Menu_5_6_Concuss,
	&Menu_5_7_Version,
	&Menu_5_8_Usb,
};
static void menuswitch(void)
{
unsigned char i=0;
	
lcd_fill(0);
lcd_text12(0,3,"其它",4,LCD_MODE_SET);
lcd_text12(0,17,"信息",4,LCD_MODE_SET);
for(i=0;i<8;i++)
	lcd_bitmap(30+i*11, 5, &BMP_noselect_5, LCD_MODE_SET);
lcd_bitmap(30+menu_pos*11,5,&BMP_select_5,LCD_MODE_SET);
lcd_text12(30,19,(char *)(psubmenu[menu_pos]->caption),psubmenu[menu_pos]->len,LCD_MODE_SET);
lcd_update_all();
}
static void msg( void *p)
{
}
static void show(void)
{
    menu_pos=0;
	menuswitch();
}


static void keypress(unsigned int key)
{
switch(key)
	{
	case KEY_MENU:
		pMenuItem=&Menu_1_Idle;
		pMenuItem->show();
		CounterBack=0;
		break;
	case KEY_OK:
			pMenuItem=psubmenu[menu_pos];//疲劳驾驶
			pMenuItem->show();
		break;
	case KEY_UP:
		if(menu_pos==0) 
			menu_pos=7;
		else
			menu_pos--;
		menuswitch();		
		break;
	case KEY_DOWN:
		menu_pos++;
		if(menu_pos>7)
			menu_pos=0;
		menuswitch();
		break;
	}
}


static void timetick(unsigned int systick)
{

	CounterBack++;
	if(CounterBack!=MaxBankIdleTime)
		return;
	CounterBack=0;
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();
}


MENUITEM	Menu_5_other=
{
    "其他信息",
	8,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};


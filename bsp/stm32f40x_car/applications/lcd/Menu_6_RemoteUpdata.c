#include "Menu_Include.h"
#include "sed1520.h"


u16  counter=0;

u16 RxPage_Counter=0;//进度计数
u8   UpdataProc[8]={"   /   "};

void Proc_Fun(void)
{
 /*   UpdataProc[0]=ISP_current_packnum/100+'0';
	UpdataProc[1]=ISP_current_packnum%100/10+'0';
	UpdataProc[2]=ISP_current_packnum%10+'0';
	UpdataProc[4]=ISP_total_packnum/100+'0';
	UpdataProc[5]=ISP_total_packnum%100/10+'0';
	UpdataProc[6]=ISP_total_packnum%10+'0';
*/
    lcd_fill(0);
	lcd_text12(12,3,"主机程序升级进度",16,LCD_MODE_SET);
	lcd_text12(36,20,(char*)UpdataProc,7,LCD_MODE_SET);
	lcd_update_all();

}

static void msg( void *p)
{

}
static void show(void)
{
}


static void keypress(unsigned int key)
{
	switch(key)
		{
		case KEY_MENU:
			pMenuItem=&Menu_7_CentreTextDisplay;
			pMenuItem->show();
			if(ISP_Updata_Flag==2)
				{
				ISP_Updata_Flag=0;
				
				pMenuItem=&Menu_1_Idle;
				pMenuItem->show();
				}
			break;
		case KEY_OK:
				
			break;
		case KEY_UP:

			break;
		case KEY_DOWN:
		
			break;
		}
}

static void timetick(unsigned int systick)  
{
if(ISP_Updata_Flag==1)
	{
	counter++;
	if(counter>=20)
		{
		counter=0;
		Proc_Fun();
		}
	}
else if(ISP_Updata_Flag==2)
	{
	lcd_fill(0);
	lcd_text12(24,10,"远程升级完成",12,LCD_MODE_SET);
	lcd_update_all();
	}
}


MENUITEM	Menu_6_RemoteUpdata=
{
"远程升级",
	8,0,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};


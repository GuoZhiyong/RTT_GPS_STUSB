#include "menu.h"


struct IMG_DEF test_1_Check={12,12,test_00};

unsigned char noselect_check[]={0x3C,0x7E,0xC3,0xC3,0xC3,0xC3,0x7E,0x3C};//空心
unsigned char select_check[]={0x3C,0x7E,0xFF,0xFF,0xFF,0xFF,0x7E,0x3C};//实心

static unsigned char menu_pos=0;


DECL_BMP(8,8,select_check); DECL_BMP(8,8,noselect_check); 

	
static PMENUITEM psubmenu[7]=
{
	&Menu_2_3_1_Sing8,			//信号线
	&Menu_2_3_2_sudu,			//15speed
	&Menu_2_3_3_TextInforStor,	//文本消息(消息1-消息8)
	&Menu_2_3_4_carinfor,		//车辆信息
	&Menu_2_3_5_jiayuan,		//驾驶员信息
	&Menu_2_3_6_Mileage,		//里程信息
	&Menu_2_3_7_CenterInforMeun,//中心信息点播
};



static void menuswitch(void)
{
	unsigned char i;
	lcd_fill(0);
	DisAddRead_ZK(0,3,"信息",2,&test_1_Check,0,0);
	DisAddRead_ZK(0,17,"查看",2,&test_1_Check,0,0);
	for(i=0;i<6;i++) lcd_bitmap(47+i*12, 5, &BMP_noselect_check, LCD_MODE_SET);
	lcd_bitmap(35+menu_pos*12,5, &BMP_select_check, LCD_MODE_SET);
	DisAddRead_ZK(35,19,(char *)(psubmenu[menu_pos]->caption),5,&test_1_Check,1,0);
	lcd_update_all();
}

static void show(void)
{
	menu_pos=0;
	menuswitch();
}


static void keypress(unsigned int key)
{
switch(KeyValue)
	{
	case KeyValueMenu:
		CounterBack=0;

		pMenuItem=&Menu_1_InforInteract;//scr_CarMulTrans;
		pMenuItem->show();
		break;
	case KeyValueOk:
		pMenuItem=psubmenu[menu_pos];
		pMenuItem->show();
		break;
	case KeyValueUP:
		if(menu_pos==0) menu_pos=6;
		menu_pos--;
		menuswitch();
		break;
	case KeyValueDown:
		menu_pos++;
		menu_pos%=6;
		menuswitch();
		break;
	}
KeyValue=0;
}


static void timetick(unsigned int systick)
{

	CounterBack++;
	if(CounterBack!=MaxBankIdleTime)
		return;
	pMenuItem=&Menu_1_Idle;
	pMenuItem->show();
	CounterBack=0;

}


MENUITEM	Menu_1_InforCheck=
{
	"",
	&show,
	&keypress,
	&timetick,
	(void*)0
};


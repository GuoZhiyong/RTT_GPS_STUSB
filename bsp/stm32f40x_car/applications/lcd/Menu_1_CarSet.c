#include "menu.h"


struct IMG_DEF test_1_MeunSet={12,12,test_00};

unsigned char noselect_set[]={0x3C,0x7E,0xC3,0xC3,0xC3,0xC3,0x7E,0x3C};//����
unsigned char select_set[]={0x3C,0x7E,0xFF,0xFF,0xFF,0xFF,0x7E,0x3C};//ʵ��


static unsigned char menu_pos=0;

DECL_BMP(8,8,select_set); DECL_BMP(8,8,noselect_set); 

static PMENUITEM psubmenu[6]=
{
	&Menu_2_2_1_license,//���ƺ�����
	&Menu_2_2_2_CarType,//��������ѡ��
	&Menu_2_2_3_SpeedSet,//�ٶ�����
	&Menu_2_2_4_VINset,//VIN����
	&Menu_2_2_5_Cancel,//���Υ�¼�¼
	&Menu_1_Idle,//�����궨
};



static void menuswitch(void)
{
	int i,index;
	lcd_fill(0);
	DisAddRead_ZK(0,3,"����",2,&test_1_MeunSet,0,0);
	DisAddRead_ZK(0,17,"����",2,&test_1_MeunSet,0,0);
	for(i=0;i<5;i++) lcd_bitmap(35+index*12, 5, &BMP_noselect_set, LCD_MODE_SET);
	lcd_bitmap(35+index*12, 5, &BMP_select_set, LCD_MODE_SET);
	DisAddRead_ZK(35,19,(char *)(psubmenu[menu_pos]->caption),5,&test_1_MeunSet,1,0);
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
		pMenuItem=&Menu_1_Idle;//������Ϣ�鿴����
		pMenuItem->show();
		break;
	case KeyValueOk:
		pMenuItem=psubmenu[menu_pos];//���ƺ�����
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


MENUITEM	Menu_1_CarSet=
{
	"��������",
	&show,
	&keypress,
	&timetick,
	(void*)0
};


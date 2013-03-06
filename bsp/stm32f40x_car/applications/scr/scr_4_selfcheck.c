#include "scr.h"




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
	DisAddRead_ZK(35,19,(char *)(psubmenu[menu_pos]->caption),4,&test_1_MeunSet,1,0);
	lcd_update_all();
}



static void show(void)
{
	menu_pos=0;
	menuswitch();
}




/*��������*/
static void keypress(unsigned int key)
{

}

/*ϵͳʱ��*/
static void timetick(unsigned int systick)
{

}

/*�����Լ�״̬����Ϣ*/
static void msg(void *p)
{


}



SCR scr_0_selftest=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};
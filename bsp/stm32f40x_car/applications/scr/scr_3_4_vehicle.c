#include "scr.h"



static SCR_ITEM scr_item[] =
{
	{ "���ƺ���",	8, 0 },
	{ "������ɫ",	8, 0 },
	{ "���Ʒ���",	8, 0 },
	{ "����ϵ��",	8, 0 },
};



static void show(void* parent)
{
	scr_3_4_vehicle.parent=(PSCR)parent;

}


/*��������*/
static void keypress(unsigned int key)
{
	switch(key)
	{
		case KEY_MENU_PRESS:
		case KEY_OK_PRESS:				/*�����ϼ��˵�*/
			break;
		case KEY_UP_PRESS:
			break;	
		case KEY_DOWN_PRESS:
			break;
	}
}

/*ϵͳʱ��,��˸*/
static void timetick(unsigned int systick)
{

}

/*�����Լ�״̬����Ϣ*/
static void msg(void *thiz,void *p)
{


}



SCR scr_3_4_vehicle=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};




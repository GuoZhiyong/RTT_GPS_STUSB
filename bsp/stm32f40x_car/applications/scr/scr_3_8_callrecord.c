#include "scr.h"

	
/**/
static void show(void *parent)
{
	scr_3_8_callrecord.parent=(PSCR)parent;

}


/*��������*/
static void keypress(unsigned int key)
{
	switch(key)
	{
		case KEY_MENU_PRESS:
			break;
		case KEY_OK_PRESS:				/*�����ϼ��˵�*/
			break;
		case KEY_UP_PRESS:
			break;	
		case KEY_DOWN_PRESS:
			break;
	}
}

/*ϵͳʱ��*/
static void timetick(unsigned int systick)
{

}

/*�����Լ�״̬����Ϣ*/
static void msg(void *thiz,void *p)
{


}



SCR scr_3_8_callrecord=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};











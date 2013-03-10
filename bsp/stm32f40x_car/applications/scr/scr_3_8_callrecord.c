#include "scr.h"

	
/**/
static void show(void *parent)
{
	scr_3_8_callrecord.parent=(PSCR)parent;

}


/*按键处理*/
static void keypress(unsigned int key)
{
	switch(key)
	{
		case KEY_MENU_PRESS:
			break;
		case KEY_OK_PRESS:				/*返回上级菜单*/
			break;
		case KEY_UP_PRESS:
			break;	
		case KEY_DOWN_PRESS:
			break;
	}
}

/*系统时间*/
static void timetick(unsigned int systick)
{

}

/*处理自检状态的消息*/
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











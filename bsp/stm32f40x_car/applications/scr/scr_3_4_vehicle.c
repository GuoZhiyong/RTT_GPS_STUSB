#include "scr.h"



static SCR_ITEM scr_item[] =
{
	{ "车牌号码",	8, 0 },
	{ "车牌颜色",	8, 0 },
	{ "车牌分类",	8, 0 },
	{ "脉冲系数",	8, 0 },
};



static void show(void* parent)
{
	scr_3_4_vehicle.parent=(PSCR)parent;

}


/*按键处理*/
static void keypress(unsigned int key)
{
	switch(key)
	{
		case KEY_MENU_PRESS:
		case KEY_OK_PRESS:				/*返回上级菜单*/
			break;
		case KEY_UP_PRESS:
			break;	
		case KEY_DOWN_PRESS:
			break;
	}
}

/*系统时间,闪烁*/
static void timetick(unsigned int systick)
{

}

/*处理自检状态的消息*/
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




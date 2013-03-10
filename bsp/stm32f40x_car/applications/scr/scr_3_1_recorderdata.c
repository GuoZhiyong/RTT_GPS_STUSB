#include "scr.h"











static void show(void * parent)
{

}




/*按键处理*/
static void keypress(unsigned int key)
{

}

/*系统时间*/
static void timetick(unsigned int systick)
{

}

/*处理自检状态的消息*/
static void msg(void *thiz,void *p)
{


}



SCR scr_3_1_recorderdata=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};


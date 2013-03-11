#include "scr.h"



static unsigned char pos=0;

static void display(void)
{
	lcd_fill(0);

	lcd_update_all();
}

	
/**/
static void show(void *parent)
{
	scr_4_sms.parent=(PSCR)parent;
	display();
}


/*按键处理*/
static void keypress(unsigned int key)
{
	switch(key)
	{
		case KEY_MENU_PRESS:		/*返回上级菜单*/
			break;
		case KEY_OK_PRESS:				
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



SCR scr_4_1_smssend=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};











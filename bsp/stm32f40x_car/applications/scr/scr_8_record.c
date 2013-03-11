#include "scr.h"





static void display(void)
{
	lcd_fill(0);
	lcd_text12(0,0,"记录",4,LCD_MODE_INVERT);
	lcd_text12((121-6*11)/2,16,"无记录信息!",11,LCD_MODE_SET);
	lcd_update_all();
}

static void show(void *parent)
{
	scr_8_record.parent=(PSCR)parent;
	display();

}


/*按键处理*/
static void keypress(unsigned int key)
{
	switch(key)
	{
		case KEY_MENU_PRESS:/*返回上级菜单*/
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



SCR scr_8_record=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};









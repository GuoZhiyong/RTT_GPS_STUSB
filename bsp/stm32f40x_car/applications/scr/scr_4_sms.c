#include "scr.h"

/*
为了减少界面层次
集中部分界面，将操作少的界面集中
*/

static SCR_ITEM scr_item[] =
{
	{ "收件箱",		6,	0 },
	{ "1.事件信息",	10, 0 },
	{ "2.中心信息",	10, 0 },
	{ "3.手机信息",	10, 0 },
	{ "4.提问信息", 10, 0 },
	{ "5.信息点播", 10, 0 },
};




static unsigned char pos=0;

static void display(void)
{
	lcd_fill(0);
	lcd_text12(0,0,"信息",4,LCD_MODE_INVERT);
	lcd_text12(121-6*6,0,"0 未读",6,LCD_MODE_SET);

	/*间隔 (122-36*3)/2=7 */
	lcd_text12(0,16,"发信息",6,(pos==0)?LCD_MODE_INVERT:LCD_MODE_SET);
	lcd_text12(43,16,"收件箱",6,(pos==1)?LCD_MODE_INVERT:LCD_MODE_SET);
	lcd_text12(86,16,"发件箱",6,(pos==2)?LCD_MODE_INVERT:LCD_MODE_SET);
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
			if((void*)0==scr_3_geoinfo.parent) break;
			pscr=scr_4_sms.parent;
			pscr->show(0);
			break;
		case KEY_OK_PRESS:				
			break;
		case KEY_UP_PRESS:
			pos++;
			pos%=3;
			display();
			break;	
		case KEY_DOWN_PRESS:
			if(pos==0) pos=3;
			pos--;
			display();
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



SCR scr_4_sms=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};










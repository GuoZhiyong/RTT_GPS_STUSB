#include "scr.h"


static SCR_ITEM scr_item[] =
{
	{ "收件箱",		6,	0 },
	{ "1.事件信息",	10, 0 },
	{ "2.中心信息",	10, 0 },
	{ "3.手机信息",	10, 0 },
	{ "4.提问信息", 10, 0 },
	{ "5.信息点播", 10, 0 },
};

static unsigned char pos=1;

static void display(void)
{
	lcd_fill(0);
	if( pos & 0x01 )      /*是单数*/
	{
		lcd_text12( 0, 0, scr_item[pos-1].text, scr_item[pos-1].len, LCD_MODE_SET );
		lcd_fill_rect(0,16,121,28,LCD_MODE_SET);
		lcd_text12( 0, 16, scr_item[pos].text, scr_item[pos].len, LCD_MODE_INVERT);
	}
	else
	{
		lcd_fill_rect(0,0,121,12,LCD_MODE_SET);
		lcd_text12( 0, 0, scr_item[pos].text, scr_item[pos].len, LCD_MODE_INVERT);
		lcd_text12( 0, 16, scr_item[pos+1].text, scr_item[pos+1].len, LCD_MODE_SET );
	}

	lcd_update_all();
}

	
/**/
static void show(void *parent)
{
	scr_4_2_sms_inbox.parent=(PSCR)parent;
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



SCR scr_4_2_sms_inbox=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};












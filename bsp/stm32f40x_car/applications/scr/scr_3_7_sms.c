#include "scr.h"



static unsigned char pos=0;

static void display(void)
{
	lcd_fill(0);
	lcd_text12(0,0,"信息",4,LCD_MODE_INVERT);
	lcd_text12(121-36,0,"0 未读",6,LCD_MODE_SET);

	/*间隔 (122-36*3)/2=7 */
	if(0==pos)
	{ 
		lcd_text12(0,16,"发信息",6,LCD_MODE_INVERT);
		lcd_text12(43,16,"收件箱",6,LCD_MODE_SET);
		lcd_text12(86,16,"发件箱",6,LCD_MODE_SET);
	}
	else if(1==pos)
	{ 
		lcd_text12(0,16,"发信息",6,LCD_MODE_SET);
		lcd_text12(43,16,"收件箱",6,LCD_MODE_INVERT);
		lcd_text12(86,16,"发件箱",6,LCD_MODE_SET);
	}
	else
	{
		lcd_text12(0,16,"发信息",6,LCD_MODE_SET);
		lcd_text12(43,16,"收件箱",6,LCD_MODE_SET);
		lcd_text12(86,16,"发件箱",6,LCD_MODE_INVERT);
	}
	lcd_update_all();
}

	
/**/
static void show(void *parent)
{
	scr_3_7_sms.parent=(PSCR)parent;
	display();
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



SCR scr_3_7_sms=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};










#include "scr.h"



static unsigned char pos=1;

static SCR_ITEM scr_item[] =
{
	{ "媒体中心",	8,	0				 },
	{ "1.文件管理",	10, 0 },
	{ "2.导出日志",	10,0 },
	{ "3.导出视频",	10, 0 },
	{ "4.导出音频", 10,0 },
	{ "5.收音机", 8, 0 },
};


/*
只是显示信息，并没有子菜单
显示3页的信息 经纬度 定位 时间
*/
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

static void show(void *parent)
{
	scr_c_multimedia.parent=(PSCR)parent;
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
			if(pos==1) pos=6;
			pos--;
			display();
			break;	
		case KEY_DOWN_PRESS:
			if(pos==5) pos=0;
			pos++;
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



SCR scr_c_multimedia=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};









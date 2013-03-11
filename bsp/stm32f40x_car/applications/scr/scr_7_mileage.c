#include "scr.h"




static uint8_t pageindex=0;

/*
只是显示信息，并没有子菜单
显示3页的信息 经纬度 定位 时间
*/
static void showpage(void)
{
	lcd_fill(0);
	switch(pageindex){
		case 0:
			lcd_text12(0,0,"清零里程",8,LCD_MODE_INVERT);
			lcd_text12(121-6*3,0,"0KM",3,LCD_MODE_INVERT);
			lcd_text12(0,16,"总里程 ",8,LCD_MODE_INVERT);
			lcd_text12(121-6*3,16,"0KM",3,LCD_MODE_INVERT);
			break;
		case 1:
			lcd_text12(0,0,"昨日里程",6,LCD_MODE_INVERT);
			lcd_text12(121-6*3,0,"0KM",3,LCD_MODE_INVERT);
			lcd_text12(0,16,"今日里程",6,LCD_MODE_INVERT);
			lcd_text12(121-6*3,16,"0KM",3,LCD_MODE_INVERT);
			break;
	}		
	lcd_update_all();
}

static void show(void *parent)
{
	scr_7_mileage.parent=(PSCR)parent;
	showpage();

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
			if(pageindex==0) pageindex=2;
			pageindex--;
			showpage();
			break;	
		case KEY_DOWN_PRESS:
			pageindex++;
			pageindex%=2;
			showpage();
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



SCR scr_7_mileage=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};









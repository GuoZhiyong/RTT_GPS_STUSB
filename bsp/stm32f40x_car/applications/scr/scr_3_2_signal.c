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
			lcd_text12(0,0,"IO输入",6,LCD_MODE_INVERT);
			
			
			break;
		case 1:
			lcd_text12(0,0,"模拟量",6,LCD_MODE_INVERT);

			break;
		case 2:
			lcd_text12(0,0,"电源",4,LCD_MODE_INVERT);
			break;
	}		
	lcd_update_all();
}

static void show(void)
{
	pageindex=0;
	showpage();

}


/*按键处理*/
static void keypress(void *thiz,unsigned int key)
{
	switch(key)
	{
		case KEY_MENU_PRESS:
		case KEY_OK_PRESS:				/*返回上级菜单*/
			break;
		case KEY_UP_PRESS:
			pageindex--;
			if(pageindex==0) pageindex=2;
			showpage();
			break;	
		case KEY_DOWN_PRESS:
			pageindex++;
			pageindex%=3;
			showpage();
			break;
	}
}

/*系统时间*/
static void timetick(void *thiz,unsigned int systick)
{

}

/*处理自检状态的消息*/
static void msg(void *thiz,void *p)
{


}



SCR scr_3_2_signal=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};








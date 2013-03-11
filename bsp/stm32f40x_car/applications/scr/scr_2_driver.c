#include "scr.h"




static uint8_t pageindex=0;

/*
驾员信息
*/
static void showpage(void)
{
	lcd_fill(0);
	switch(pageindex){
		case 0:
			lcd_text12(0,0,"驾员姓名",8,LCD_MODE_INVERT);
			lcd_text12(112-6*4,0,"匿名",4,LCD_MODE_SET);
			lcd_text12(0,14,"ID",2,LCD_MODE_INVERT);
			lcd_text12(112-6*18,14,"------------------",18,LCD_MODE_SET);
			lcd_bitmap(116,28,&BMP_res_arrow_dn,LCD_MODE_SET);
			break;
		case 1:
			lcd_text12(0,0,"连续行驶里程",12,LCD_MODE_INVERT);
			lcd_text12(112-6*4,0,"--KM",4,LCD_MODE_SET);
			lcd_text12(0,14,"连续行驶时间",12,LCD_MODE_INVERT);
			lcd_text12(112-6*8,14,"--:--:--",8,LCD_MODE_SET);
			lcd_bitmap(110,28,&BMP_res_arrow_up,LCD_MODE_SET);
			lcd_bitmap(116,28,&BMP_res_arrow_dn,LCD_MODE_SET);
			break;
		case 2:
			lcd_text12(0,0,"超速速度",8,LCD_MODE_INVERT);
			lcd_text12(121-6*5,0,"0KM/H",5,LCD_MODE_SET);
			lcd_text12(0,14,"疲劳时间",8,LCD_MODE_INVERT);
			lcd_text12(121-6*8,14,"00:00:00",8,LCD_MODE_SET);
			lcd_bitmap(116,28,&BMP_res_arrow_up,LCD_MODE_SET);
			break;
	}		
	lcd_update_all();
}

static void show(void *parent)
{
	scr_2_driver.parent=(PSCR)parent;
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
			if(pageindex==0) pageindex=3;
			pageindex--;
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
static void timetick(unsigned int systick)
{

}

/*处理自检状态的消息*/
static void msg(void *thiz,void *p)
{


}



SCR scr_2_driver=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};









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
			lcd_text12(0,0,"经纬度",6,LCD_MODE_INVERT);
			
			lcd_bitmap(116,28,&BMP_res_arrow_dn,LCD_MODE_SET);
			break;
		case 1:
			lcd_text12(0,0,"定位",4,LCD_MODE_INVERT);
			lcd_text12(30,0,"速度 方向 高度",8,LCD_MODE_SET);
			lcd_bitmap(110,28,&BMP_res_arrow_dn,LCD_MODE_SET);
			lcd_bitmap(116,28,&BMP_res_arrow_up,LCD_MODE_SET);
			break;
		case 2:
			lcd_text12(0,0,"时间",4,LCD_MODE_INVERT);
			lcd_bitmap(116,28,&BMP_res_arrow_up,LCD_MODE_SET);
			break;
	}		
	lcd_update_all();
}

static void show(void* parent)
{
	scr_3_3_geoinfo.parent=(PSCR)parent;
	pageindex=0;
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
static void timetick(unsigned int systick)
{

}

/*处理自检状态的消息*/
static void msg(void *thiz,void *p)
{


}



SCR scr_3_3_geoinfo=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};




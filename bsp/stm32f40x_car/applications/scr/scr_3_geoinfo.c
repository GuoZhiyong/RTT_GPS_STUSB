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
			lcd_asc0608(121-6*12,12,"E:----------",12,LCD_MODE_INVERT);
			lcd_asc0608(121-6*12,20,"N:----------",12,LCD_MODE_INVERT);
			lcd_bitmap(121-5,28,&BMP_res_arrow_dn,LCD_MODE_SET);
			break;
		case 1:/*一行显示 [定位 速度 方向 高度]   8*2+3=19 19*6=114,注意排版美观*/
			lcd_text12(1,0,"定位",4,LCD_MODE_INVERT); /*空一个*/
			lcd_text12(33,0,"速度",4,LCD_MODE_SET);/*空8个*/
			lcd_text12(65,0,"方向",4,LCD_MODE_SET);
			lcd_text12(97,0,"高度",4,LCD_MODE_SET);
			lcd_asc0608(25-6*2,0,"--",2,LCD_MODE_SET); /*空一个*/
			lcd_asc0608(57-6*2,0,"--",2,LCD_MODE_SET);/*空8个*/
			lcd_asc0608(89-6*2,0,"--",2,LCD_MODE_SET);
			lcd_asc0608(121-6*2,0,"--",2,LCD_MODE_SET);
			lcd_bitmap(121-5*2,28,&BMP_res_arrow_dn,LCD_MODE_SET);
			lcd_bitmap(121-5,28,&BMP_res_arrow_up,LCD_MODE_SET);
			break;
		case 2:
			lcd_text12(0,0,"定位源",6,LCD_MODE_INVERT);
			lcd_text12((121-6*13)/2,16,"BD2 B1/GPS L1",13,LCD_MODE_SET);
			lcd_bitmap(121-5*2,28,&BMP_res_arrow_dn,LCD_MODE_SET);
			lcd_bitmap(121-5,28,&BMP_res_arrow_up,LCD_MODE_SET);
		case 3:
			lcd_text12(0,0,"时间",4,LCD_MODE_INVERT);
			lcd_asc0608(100-6*10,10,"2013-03-11",10,LCD_MODE_SET);
			lcd_asc0608(100-6*8,18,"09:07:14",8,LCD_MODE_SET);
			lcd_bitmap(121-5,28,&BMP_res_arrow_up,LCD_MODE_SET);

			break;
	}		
	lcd_update_all();
}

static void show(void* parent)
{
	scr_3_geoinfo.parent=(PSCR)parent;
	showpage();

}


/*按键处理*/
static void keypress(unsigned int key)
{
	switch(key)
	{
		case KEY_MENU_PRESS:   /*返回上级菜单*/
			if((void*)0==scr_3_geoinfo.parent) break;
			pscr=scr_3_geoinfo.parent;
			pscr->show(0);
			break;
		case KEY_OK_PRESS:				
			break;
		case KEY_UP_PRESS:
			if(pageindex==0) pageindex=4;
			pageindex--;
			showpage();
			break;	
		case KEY_DOWN_PRESS:
			pageindex++;
			pageindex%=4;
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



SCR scr_3_geoinfo=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};




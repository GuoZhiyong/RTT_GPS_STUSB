#include "scr.h"

static void show(void)
{
	lcd_fill(0);
	lcd_fill_rect(0,0,121,12,LCD_MODE_SET);
	lcd_text12(16,0,"正在导出数据...",15,LCD_MODE_INVERT);

	lcd_rect(10,18,80,10,LCD_MODE_SET);
	lcd_fill_rect(10,18,70,16+10-1,LCD_MODE_SET);


	lcd_update_all();
}


/*按键处理*/
static void keypress(void *thiz,unsigned int key)
{

}

/*系统时间*/
static void timetick(void *thiz,unsigned int systick)
{

}

/*处理自检状态的消息*/
static void msg(void *thiz,void *p)
{


}



SCR scr_9_export_usb=
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};






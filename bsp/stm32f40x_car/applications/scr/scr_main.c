/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		// 文件名
 * Author:			// 作者
 * Date:			// 日期
 * Description:		// 模块描述
 * Version:			// 版本信息
 * Function List:	// 主要函数及其功能
 *     1. -------
 * History:			// 历史修改记录
 *     <author>  <time>   <version >   <desc>
 *     David    96/10/12     1.0     build this moudle
 ***********************************************************/
#include "scr.h"




static SCR_ITEM scr_item[] =
{
	{ "1.信号",	6, &scr_1_signal},
	{ "2.驾员",	6, &scr_2_driver},
	{ "3.位置",	6, &scr_3_geoinfo},
	{ "4.信息", 6, &scr_4_sms},
	{ "5.电话", 6, &scr_5_phonecall},
	{ "6.高级",	6, &scr_6_advance},
	{ "7.里程",	6, &scr_7_mileage},
	{ "8.记录",	6, &scr_8_record},
	{ "9.疲劳",	6, &scr_9_tired},
	{ "A.车辆",	6, &scr_a_vehicle},
	{ "B.网络",	6, &scr_b_network},
	{ "C.媒体",	6, &scr_c_multimedia},
};


static uint8_t	selectpos = 0;  /*选定的位置*/
static uint32_t tick=0;

/*显示菜单*/
static void display( void )
{
	lcd_fill( 0 );
	if(selectpos<6)
	{
		lcd_bitmap(0,4,&BMP_res_arrow_dn,LCD_MODE_SET);
		lcd_text12( 2, 8, scr_item[0].text, scr_item[0].len, (selectpos==0)?LCD_MODE_INVERT:LCD_MODE_SET );
		lcd_text12( 43, 8, scr_item[1].text, scr_item[1].len,(selectpos==1)?LCD_MODE_INVERT:LCD_MODE_SET );
		lcd_text12( 84, 8, scr_item[2].text, scr_item[2].len, (selectpos==2)?LCD_MODE_INVERT:LCD_MODE_SET );
		lcd_text12( 2, 20, scr_item[3].text, scr_item[3].len, (selectpos==3)?LCD_MODE_INVERT:LCD_MODE_SET );
		lcd_text12( 43, 20, scr_item[4].text, scr_item[4].len, (selectpos==4)?LCD_MODE_INVERT:LCD_MODE_SET );
		lcd_text12( 84, 20, scr_item[5].text, scr_item[5].len, (selectpos==5)?LCD_MODE_INVERT:LCD_MODE_SET );
		
	}
	else
	{
		lcd_bitmap(0,0,&BMP_res_arrow_up,LCD_MODE_SET);
		lcd_text12( 2, 8, scr_item[6].text, scr_item[6].len, (selectpos==6)?LCD_MODE_INVERT:LCD_MODE_SET );
		lcd_text12( 43, 8, scr_item[7].text, scr_item[7].len,(selectpos==7)?LCD_MODE_INVERT:LCD_MODE_SET );
		lcd_text12( 84, 8, scr_item[8].text, scr_item[8].len, (selectpos==8)?LCD_MODE_INVERT:LCD_MODE_SET );
		lcd_text12( 2, 20, scr_item[9].text, scr_item[9].len, (selectpos==9)?LCD_MODE_INVERT:LCD_MODE_SET );
		lcd_text12( 43, 20, scr_item[10].text, scr_item[10].len, (selectpos==10)?LCD_MODE_INVERT:LCD_MODE_SET );
		lcd_text12( 84, 20, scr_item[11].text, scr_item[11].len, (selectpos==11)?LCD_MODE_INVERT:LCD_MODE_SET );
	}

	lcd_update_all( );
	
}

/***/
static void show( void* parent )
{
	scr_main.parent=(PSCR)parent;
	display( );
}

/*按键处理*/
static void keypress(unsigned int key )
{
	switch( key )
	{
		case KEY_MENU_PRESS:
			if((void*)0==scr_main.parent) break;
			pscr=scr_main.parent;
			pscr->show(&scr_main);
			break;
		case KEY_OK_PRESS:
			pscr=scr_item[selectpos].scr;
			pscr->show(&scr_main);
			break;		
		case KEY_UP_PRESS:
			if( selectpos == 0 ) selectpos = 12;
			selectpos--;
			display();
			break;
		case KEY_DOWN_PRESS:
			selectpos++;
			if( selectpos == 12 )selectpos = 0;
			display();
			break;

	}
}

/*系统时间*/
static void timetick(unsigned int systick )
{
	tick++;
	if(selectpos<6)
	{
		if((tick&0xf)==0x00)		/*0b0000*/
		{
			lcd_bitmap(0,4,&BMP_res_arrow_none,LCD_MODE_SET);
			lcd_update(4,8);
		}
		if((tick&0x0f)==0x08)		/*0b1000*/
		{
			lcd_bitmap(0,4,&BMP_res_arrow_dn,LCD_MODE_SET);
			lcd_update(4,8);
		}
	}
	else
	{
		if((tick&0xf)==0x00)		/*0b0000*/
		{
			lcd_bitmap(0,0,&BMP_res_arrow_none,LCD_MODE_SET);
			lcd_update(0,4);
		}
		if((tick&0x0f)==0x08)		/*0b1000*/
		{
			lcd_bitmap(0,0,&BMP_res_arrow_up,LCD_MODE_SET);
			lcd_update(0,4);
		}
	}
	


}

/**/
static void msg(  void *p )
{
}

SCR scr_main =
{
	&show,
	&keypress,
	&timetick,
	(void*)0
};

/************************************** The End Of File **************************************/

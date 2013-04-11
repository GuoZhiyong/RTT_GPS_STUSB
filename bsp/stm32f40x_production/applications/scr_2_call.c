#include "scr.h"







static void show(void *parent)
{
	scr_2_call.parent=(PSCR)parent;
	lcd_fill( 0 );
	lcd_asc0608((122-6*13)/2,12,"Incoming Call",LCD_MODE_SET);
	lcd_asc0608(30,24," OK  ",LCD_MODE_INVERT);
	lcd_asc0608(121-6*5,24," CLR ",LCD_MODE_INVERT);
	lcd_update(0,31);

}


/*按键处理*/
static void keypress(unsigned int key)
{
	switch(key)
	{
		case KEY_MENU_PRESS:

			break;
		case KEY_OK_PRESS:				/*接听*/
			gsm_write("ATA\r\n");
			lcd_asc0608((122-6*13)/2,12,"  Connected  ",LCD_MODE_SET);
			lcd_update(0,31);
			break;
		case KEY_UP_PRESS:

			break;	
		case KEY_DOWN_PRESS:			/*拒绝*/
			gsm_write("ATH\r\n");
			pscr=&scr_1_idle;
			pscr->show((void*)0);
			break;
	}
}

/*系统时间*/
static void timetick(unsigned int systick)
{

}

/*处理自检状态的消息*/
static void msg(void *pmsg)
{
	LCD_MSG			* plcd_msg	= (LCD_MSG* )pmsg;
	uint32_t		sec;

	if( plcd_msg->id == 0x0002 )
	{
		if(plcd_msg->info.payload[0]==2)	/*挂断通话*/
		{
			pscr=&scr_1_idle;
			pscr->show((void*)0);
		}	
	}
}



SCR scr_2_call=
{
	&show,
	&keypress,
	&timetick,
	&msg,
};









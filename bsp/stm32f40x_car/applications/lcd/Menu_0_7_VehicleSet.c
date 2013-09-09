#include  <string.h>
#include "Menu_Include.h"
#include "sed1520.h"

static uint8_t pos;
/**/
static void display( void )
{
	lcd_fill( 0 );
	lcd_text12(12,4,"有牌照车辆",10,3-pos*2);
	lcd_text12(12,16,"无牌照车辆",10,pos*2+1);
	lcd_update_all( );
}

/**/
static void show( void )
{
	pMenuItem->tick = rt_tick_get( );
	pos=0;
	display( );
}

static void msg(void)
{
	
}

/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static void keypress( unsigned int key )
{
	switch( key )
	{
		case KEY_MENU:
			pMenuItem = &Menu_0_loggingin;
			pMenuItem->show( );
			break;
		case KEY_OK:
			if(pos)
			{
				pMenuItem=&Menu_0_3_vin;
			}
			else
			{
				pMenuItem=&Menu_0_1_license;
			}
			pMenuItem->show();
			break;
		case KEY_UP:
		case KEY_DOWN:
			pos++;
			pos %= 2;
			display( );
			break;
	}
}




MENUITEM Menu_0_7_VehicleSet =
{
	"设置类型",
	7,				  0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};


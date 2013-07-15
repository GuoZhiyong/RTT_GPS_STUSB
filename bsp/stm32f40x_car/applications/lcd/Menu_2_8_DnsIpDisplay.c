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
#include "Menu_Include.h"
#include  <string.h>

#include "sed1520.h"


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void Disp_DnsIP( u8 par )
{
	u8	dns1[50]	= { "1. " }, dns2[50] = { "2. " }, apn[20] = { "APN:" };
	u8	len1		= 0, len2 = 0;
	u8	ip1[30]		= { "   .   .   .   :    " }, ip2[30] = { "   .   .   .   :    " };
	u8	i			= 0;
	if( par == 1 )
	{
		strcpy( dns1 + 3, jt808_param.id_0x0013 );
		strcpy( dns2 + 3, jt808_param.id_0x0017 );

		lcd_fill( 0 );
		lcd_text12( 0, 3, (char*)dns1, strlen( dns1 ), LCD_MODE_SET );
		lcd_text12( 0, 18, (char*)dns2, strlen( dns2 ), LCD_MODE_SET );
		lcd_update_all( );
	}else if( par == 2 )
	{
		#if NEED_TODO
		for( i = 0; i < 4; i++ )
		{
			ip1[i * 4] = RemoteIP_main[i] / 100 + '0';
		}
		for( i = 0; i < 4; i++ )
		{
			ip1[1 + i * 4] = RemoteIP_main[i] % 100 / 10 + '0';
		}
		for( i = 0; i < 4; i++ )
		{
			ip1[2 + i * 4] = RemoteIP_main[i] % 10 + '0';
		}

		//ip1[21]=RemotePort_1/10000+'0';
		ip1[16] = RemotePort_main % 10000 / 1000 + '0';
		ip1[17] = RemotePort_main % 1000 / 100 + '0';
		ip1[18] = RemotePort_main % 100 / 10 + '0';
		ip1[19] = RemotePort_main % 10 + '0';

		for( i = 0; i < 4; i++ )
		{
			ip2[i * 4] = RemoteIP_aux[i] / 100 + '0';
		}
		for( i = 0; i < 4; i++ )
		{
			ip2[1 + i * 4] = RemoteIP_aux[i] % 100 / 10 + '0';
		}
		for( i = 0; i < 4; i++ )
		{
			ip2[2 + i * 4] = RemoteIP_aux[i] % 10 + '0';
		}

#endif
		//ip2[21]=RemotePort_2/10000+'0';
		ip2[16] = jt808_param.id_0x0018 % 10000 / 1000 + '0';
		ip2[17] = jt808_param.id_0x0018 % 1000 / 100 + '0';
		ip2[18] = jt808_param.id_0x0018 % 100 / 10 + '0';
		ip2[19] = jt808_param.id_0x0018 % 10 + '0';

		strcpy( apn + 4, jt808_param.id_0x0010 );
		lcd_fill( 0 );
		lcd_text12( 0, 0, (char*)apn, 4 + len1, LCD_MODE_SET );
		lcd_text12( 0, 11, (char*)ip1, 20, LCD_MODE_SET );
		lcd_text12( 0, 22, (char*)ip2, 20, LCD_MODE_SET );
		lcd_update_all( );
	}
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
static void msg( void *p )
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
static void show( void )
{
	pMenuItem->tick=rt_tick_get();

	lcd_fill( 0 );
	lcd_text12( 24, 3, "查看设置信息", 12, LCD_MODE_SET );
	lcd_text12( 30, 18, "请按确认键", 10, LCD_MODE_SET );
	lcd_update_all( );
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
			pMenuItem = &Menu_2_InforCheck;
			pMenuItem->show( );
			CounterBack = 0;
			break;
		case KEY_OK:
			Disp_DnsIP( 1 );
			break;
		case KEY_UP:
			Disp_DnsIP( 1 );
			break;
		case KEY_DOWN:
			Disp_DnsIP( 2 );
			break;
	}
}


MENUITEM Menu_2_8_DnsIpDisplay =
{
	"DNS显示",
	7,0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

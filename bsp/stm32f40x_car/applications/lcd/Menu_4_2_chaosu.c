/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		// �ļ���
 * Author:			// ����
 * Date:			// ����
 * Description:		// ģ������
 * Version:			// �汾��Ϣ
 * Function List:	// ��Ҫ�������书��
 *     1. -------
 * History:			// ��ʷ�޸ļ�¼
 *     <author>  <time>   <version >   <desc>
 *     David    96/10/12     1.0     build this moudle
 ***********************************************************/
#include  <string.h>
#include "Menu_Include.h"
#include "sed1520.h"


static uint8_t	count;
static uint8_t	pos;

REC_OVERSPEED rec_overspeed[8];

/**/
static void chaosu_display( void )
{
	uint8_t *p;
	uint8_t len_tel, len_man;
	char	buf[32];
	lcd_fill( 0 );
	if( count == 0 )
	{
		lcd_fill( 0 );
		lcd_text12( ( 122 - 16 * 6 ) >> 1, 14, "[�޳��ټ�ʻ��¼]", 16, LCD_MODE_SET );
	}else
	{
		sprintf(buf,"[%d]%02d-%02d-%02d %02d:%02d:%02d",
			pos+1,
			rec_overspeed[pos].start[0],
			rec_overspeed[pos].start[1],
			rec_overspeed[pos].start[2],
			rec_overspeed[pos].start[3],
			rec_overspeed[pos].start[4],
			rec_overspeed[pos].start[5]);
			
		lcd_text12( 0, 0, buf, strlen( buf ), LCD_MODE_SET );

		sprintf(buf,"   %02d-%02d-%02d %02d:%02d:%02d",
			rec_overspeed[pos].end[0],
			rec_overspeed[pos].end[1],
			rec_overspeed[pos].end[2],
			rec_overspeed[pos].end[3],
			rec_overspeed[pos].end[4],
			rec_overspeed[pos].end[5]);

		lcd_text12( 0, 11, buf, strlen( buf ), LCD_MODE_SET );
		sprintf(buf,"���%03d  ƽ��%03d",rec_overspeed[pos].max,rec_overspeed[pos].avg);
		lcd_text12( 16, 21, buf, strlen( buf ), LCD_MODE_SET );
	}
	lcd_update_all( );
}

/**/
static void msg( void *p )
{
}

/**/
static void show( void )
{
	pMenuItem->tick = rt_tick_get( );
	count			= vdr_16_get(0x0,0xFFFFFFFF,10,(uint8_t *)&rec_overspeed);
	pos				= 0;
	chaosu_display( );
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
			pMenuItem = &Menu_5_other;
			pMenuItem->show( );
			break;
		case KEY_OK:
			break;
		case KEY_UP:
			if( pos )
			{
				pos--;
				chaosu_display( );
			}
			break;
		case KEY_DOWN:
			if( pos < count - 1 )
			{
				pos++;
				chaosu_display( );
			}
			break;
	}
}



MENUITEM Menu_4_2_chaosu =
{
	"���ټ�ʻ�鿴",
	12,				  0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

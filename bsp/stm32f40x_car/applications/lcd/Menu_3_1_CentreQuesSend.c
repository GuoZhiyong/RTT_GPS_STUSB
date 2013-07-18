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
#include "Menu_Include.h"
#include <stdio.h>
#include <string.h>
#include "sed1520.h"

//-----  ���� ------

#define VIEW_NONE	0
#define VIEW_ITEM	1
#define VIEW_DETAIL 2

static CENTER_ASK	center_ask;             /*���ݽ�������Ϣ*/
static uint8_t		item_pos		= 0;    /*��ǰҪ���ʵ�*/
static uint8_t		item_pos_read	= 0xff; /*��ǰ�Ѷ�ȡ��*/

static uint8_t		line_pos = 0;           /*��ǰ�к�*/

static uint8_t		view_mode	= VIEW_NONE;
static uint16_t		attr		= 0xffff;   /*��¼��ǰѡ�е�����*/

static DISP_ROW		disp_row[32];
static uint8_t		split_lines_count = 0;

static uint32_t		center_ask_addr;

/*����������Ϣ*/
uint8_t analy_question( void )
{
	uint8_t		*p;
	uint8_t		len;
	uint8_t		count;
	uint8_t		rows	= 0;
	uint8_t		i		= 0;
	uint8_t		ans_id;
	uint16_t	ans_len;
	uint16_t	pos, end;

	/*���ⲿ��*/
	len		= center_ask.body[0];
	p		= center_ask.body + 1;
	count	= split_content( p, len, disp_row, 20 );
	for( i = 0; i < count; i++ )
	{
		disp_row[i].attrib	= 0x8000;
		disp_row[i].start	+= 1;           /*������ʼ ����*/
		rt_kprintf( "i=%d attr=%04x start=%d count=%d\n", i, disp_row[i].attrib, disp_row[i].start, disp_row[i].count );
	}

	rows	+= count;
	end		= center_ask.len;

	pos = 1 + len;                          /*���е�ƫ�ƶ��������body����ģ�����Ҫ����*/
	/*��ѡ���б�*/
	while( pos < end )
	{
		p		= center_ask.body + pos;    /*һ���ֽڵ�id,�ͳ���(2bytes)*/
		ans_id	= p[0];
		ans_len = ( p[1] << 8 ) | p[2];
		p[0]	= ans_id / 10 + 0x30;       /*���½���һ��,�Ƿ�ʹ���Լ��ĵ������*/
		p[1]	= ans_id % 10 + 0x30;
		p[2]	= '.';
		count	= split_content( p, ans_len + 3, disp_row + rows, 20 );
		for( i = rows; i < rows + count; i++ )
		{
			disp_row[i].attrib = ans_id;
			//disp_row[i].start	+= ( pos + 3 );
			//disp_row[i].count-=3;
			disp_row[i].start += pos;
			rt_kprintf( "i=%d attr=%04x start=%d count=%d\n", i, disp_row[i].attrib, disp_row[i].start, disp_row[i].count );
		}
		rows	+= count;
		pos		+= ( 3 + ans_len );
	}
	return rows;
}

/*
   ��ʾitem 9���ֽڵ�ͷ
   uint32_t	id;        �������
   MYTIME		datetime;  �յ���ʱ��
   uint8_t	len;       ����

   ��11�ֽ���flag,
   ���ݴӵ�12�ֽڿ�ʼ������ 12x12 ÿ��10�� ASC 0612 ÿ�� 20��

 */
static void display_item( )
{
	char buf1[32];

	if( item_pos >= center_ask_count )
	{
		return;                     /*item_pos=0���Ի���*/
	}
	/*item_pos ��Χ[0..textmsg_count-1]*/
	if( item_pos_read != item_pos ) /*�б仯��Ҫ��*/
	{
		center_ask_addr		= jt808_center_ask_get( item_pos, &center_ask );
		item_pos_read		= item_pos;
		split_lines_count	= analy_question( );
	}

	lcd_fill( 0 );
	sprintf( buf1, "[%02d] %02d/%02d/%02d %02d:%02d",
	         item_pos + 1,
	         YEAR( center_ask.datetime ),
	         MONTH( center_ask.datetime ),
	         DAY( center_ask.datetime ),
	         HOUR( center_ask.datetime ),
	         MINUTE( center_ask.datetime ) );

	line_pos	= 0;                                                                                        /**/
	attr		= 0xFFFF;                                                                                   /*��ǰû��ѡ�е�����*/
	lcd_text12( 0, 4, buf1, strlen( buf1 ), LCD_MODE_SET );
	lcd_text12( 0, 16, (char*)( center_ask.body + disp_row[0].start ), disp_row[0].count, LCD_MODE_SET );   /*����*/
	lcd_update_all( );
}

/*��ʾ��ϸ����*/
static void display_detail( void )
{
	uint8_t i, col;

	i = line_pos & 0xFE;                                      /*���뵽ż������*/

	lcd_fill( 0 );

	for( col = 4; col < 20; col += 12 )
	{
		if( i < split_lines_count )
		{
			if( disp_row[i].attrib >= 0x8000 )          /*����*/
			{
				lcd_text12( 0, col, (char*)( center_ask.body + disp_row[i].start ), disp_row[i].count, LCD_MODE_SET );
				attr		= 0xFFFF;
				line_pos	= i;
			}else /*��*/
			{
				if( attr == 0xFFFF )
				{
					attr		= disp_row[i].attrib;
					line_pos	= i;
				}
				if( line_pos == i )
				{
					lcd_text12( 0, col, (char*)( center_ask.body + disp_row[i].start ), disp_row[i].count, LCD_MODE_INVERT );
				}else
				{
					lcd_text12( 0, col, (char*)( center_ask.body + disp_row[i].start ), disp_row[i].count, LCD_MODE_SET );
				}
			}
		}
		i++;
	}

	lcd_update_all( );
}

/**/
static void msg( void *p )
{
}

/*��ʾ�����·���Ϣ*/
static void show( void )
{
	if( center_ask_count == 0 )
	{
		lcd_fill( 0 );
		lcd_text12( ( 122 - 8 * 12 ) >> 1, 16, "[�����������·�]", 16, LCD_MODE_SET );
		lcd_update_all( );
		view_mode = VIEW_NONE;
		return;
	}
	pMenuItem->tick = rt_tick_get( );
	item_pos		= 0;
	item_pos_read	= 0xff;
	view_mode		= VIEW_ITEM;
	display_item( );
}

/*����������ΪITEM��DETAIL���ַ�ʽ�鿴*/
static void keypress( unsigned int key )
{
	uint8_t buf[8];
	switch( key )
	{
		case KEY_MENU:
			if( view_mode <= VIEW_ITEM )
			{
				pMenuItem = &Menu_2_InforCheck;
				pMenuItem->show( );
			}else
			{
				display_item( );
			}

			break;
		case KEY_OK:                                /*�鿴ģʽ�л�*/
			if( view_mode == VIEW_ITEM )            /*�鿴��Ŀģʽ�£��л����鿴ϸ��ģʽ*/
			{
				line_pos = 0;
				display_detail( );
				view_mode = VIEW_DETAIL;
			}else if( view_mode == VIEW_DETAIL )    /*ѡ���ض��������Ӧ��*/
			{
				rt_kprintf( "line_pos=%d attr=0x%x\n", line_pos, disp_row[line_pos].attrib );
				if( center_ask.flag )
				{
					buf[0]	= center_ask.seq[0];
					buf[1]	= center_ask.seq[1];
					buf[2]	= disp_row[line_pos].attrib;
					jt808_tx_ack( 0x0302, buf, 3 );

					center_ask.flag = 0; /*���Ϊ�ѻش�*/
					rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
					sst25_write_through( center_ask_addr, (uint8_t*)&center_ask, 12 );
					rt_sem_release( &sem_dataflash );
				}
				display_item( );
				view_mode = VIEW_ITEM;
			}
			break;
		case KEY_UP:
			if( view_mode == VIEW_ITEM )
			{
				if( item_pos )
				{
					item_pos--; //��Ϊ item_pos=textmsg_count-1 ���Ի���
				}
				display_item( );
			}else if( view_mode == VIEW_DETAIL )
			{
				if( line_pos > 0 )
				{
					line_pos--;
				}
				display_detail( );
			}
			break;
		case KEY_DOWN:
			if( view_mode == VIEW_ITEM )
			{
				if( item_pos < textmsg_count - 1 )
				{
					item_pos++;
				}
				display_item( );
			}else if( view_mode == VIEW_DETAIL )
			{
				if( line_pos < split_lines_count - 1 )
				{
					line_pos++;
				}
				display_detail( );
			}
			break;
	}
}

MENUITEM Menu_3_1_CenterQuesSend =
{
	"����������Ϣ",
	12,				  0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

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
#include <stdio.h>
#include <string.h>
#include "sed1520.h"

//-----  提问 ------

#define VIEW_NONE	0
#define VIEW_ITEM	1
#define VIEW_DETAIL 2

static CENTER_ASK	center_ask;             /*传递进来的信息*/
static uint8_t		item_pos		= 0;    /*当前要访问的*/
static uint8_t		item_pos_read	= 0xff; /*当前已读取的*/

static uint8_t		line_pos = 0;           /*当前行号*/

static uint8_t		view_mode	= VIEW_NONE;
static uint16_t		attr		= 0xffff;   /*记录当前选中的问题*/

static DISP_ROW		disp_row[32];
static uint8_t		split_lines_count = 0;

static uint32_t		center_ask_addr;

/*分析提问信息*/
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

	/*问题部分*/
	len		= center_ask.body[0];
	p		= center_ask.body + 1;
	count	= split_content( p, len, disp_row, 20 );
	for( i = 0; i < count; i++ )
	{
		disp_row[i].attrib	= 0x8000;
		disp_row[i].start	+= 1;           /*跳出开始 长度*/
		rt_kprintf( "i=%d attr=%04x start=%d count=%d\n", i, disp_row[i].attrib, disp_row[i].start, disp_row[i].count );
	}

	rows	+= count;
	end		= center_ask.len;

	pos = 1 + len;                          /*所有的偏移都是相对于body计算的，所以要调整*/
	/*候选答案列表，*/
	while( pos < end )
	{
		p		= center_ask.body + pos;    /*一个字节的id,和长度(2bytes)*/
		ans_id	= p[0];
		ans_len = ( p[1] << 8 ) | p[2];
		p[0]	= ans_id / 10 + 0x30;       /*重新解释一下,是否使用自己的递增序号*/
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
   显示item 9个字节的头
   uint32_t	id;        单增序号
   MYTIME		datetime;  收到的时间
   uint8_t	len;       长度

   第11字节是flag,
   数据从第12字节开始，汉字 12x12 每行10个 ASC 0612 每行 20个

 */
static void display_item( )
{
	char buf1[32];

	if( item_pos >= center_ask_count )
	{
		return;                     /*item_pos=0可以环回*/
	}
	/*item_pos 范围[0..textmsg_count-1]*/
	if( item_pos_read != item_pos ) /*有变化，要读*/
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
	attr		= 0xFFFF;                                                                                   /*当前没有选中的问题*/
	lcd_text12( 0, 4, buf1, strlen( buf1 ), LCD_MODE_SET );
	lcd_text12( 0, 16, (char*)( center_ask.body + disp_row[0].start ), disp_row[0].count, LCD_MODE_SET );   /*问题*/
	lcd_update_all( );
}

/*显示详细内容*/
static void display_detail( void )
{
	uint8_t i, col;

	i = line_pos & 0xFE;                                      /*对齐到偶数行上*/

	lcd_fill( 0 );

	for( col = 4; col < 20; col += 12 )
	{
		if( i < split_lines_count )
		{
			if( disp_row[i].attrib >= 0x8000 )          /*问题*/
			{
				lcd_text12( 0, col, (char*)( center_ask.body + disp_row[i].start ), disp_row[i].count, LCD_MODE_SET );
				attr		= 0xFFFF;
				line_pos	= i;
			}else /*答案*/
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

/*显示中心下发信息*/
static void show( void )
{
	if( center_ask_count == 0 )
	{
		lcd_fill( 0 );
		lcd_text12( ( 122 - 8 * 12 ) >> 1, 16, "[无中心提问下发]", 16, LCD_MODE_SET );
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

/*按键处理，分为ITEM和DETAIL两种方式查看*/
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
		case KEY_OK:                                /*查看模式切换*/
			if( view_mode == VIEW_ITEM )            /*查看项目模式下，切换到查看细节模式*/
			{
				line_pos = 0;
				display_detail( );
				view_mode = VIEW_DETAIL;
			}else if( view_mode == VIEW_DETAIL )    /*选择特定的项进行应答*/
			{
				rt_kprintf( "line_pos=%d attr=0x%x\n", line_pos, disp_row[line_pos].attrib );
				if( center_ask.flag )
				{
					buf[0]	= center_ask.seq[0];
					buf[1]	= center_ask.seq[1];
					buf[2]	= disp_row[line_pos].attrib;
					jt808_tx_ack( 0x0302, buf, 3 );

					center_ask.flag = 0; /*标记为已回答*/
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
					item_pos--; //置为 item_pos=textmsg_count-1 可以环回
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
	"中心提问消息",
	12,				  0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

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
#include <string.h>
#include "sed1520.h"

static TEXTMSG	textmsg;                /*传递进来的信息，包含TEXTMSG_HEAD*/
static uint8_t	item_pos		= 0;    /*当前要访问的*/
static uint8_t	item_pos_read	= 0xff; /*当前已读取的*/

static uint8_t	line_pos = 0;           /*当前行号*/

/*查看模式*/
#define VIEW_ITEM	0
#define VIEW_DETAIL 0xFF

uint8_t view_mode = VIEW_ITEM;

/*最多支持32行记录每一行开始的位置*/
uint8_t split_lines_pos[32][2];
uint8_t split_lines_count = 0;

#if 0
/*内容分隔为行，记录行首、行尾地址*/
uint8_t split_content( void )
{
	uint8_t count;
	uint8_t pos = 0;
	char	* p;
	uint8_t linebreak	= 0;
	int		nbfields	= 0;
	uint8_t start_of_field;

	memset( split_lines_pos, 0, 64 );

	p = textmsg.body;
	/*剔除开始的不可见*/
	while( *p < 0x20 )
	{
		p++;
		pos++;
	}

	start_of_field	= pos;
	count			= 0;
	for(; pos < textmsg.len - 1; ++pos )
	{
		if( *p < 0x20 )
		{
			linebreak = 1;
			p++;
		}else if( *p > 0x7F )   /*要增加2个*/
		{
			if( count == 9 )    /*不够了*/
			{
				count = 10;
			}else
			{
				p++;
				count++;
			}
		}else
		{
			p++;
			count++;
		}
		if( ( linebreak ) || ( count == 10 ) )
		{
			if( linebreak ) /*截短*/
			{
				if( count )
				{
					split_lines_pos[nbfields][0]	= start_of_field;
					split_lines_pos[nbfields][1]	= pos - 1;
					nbfields++;
				}
			}else //if( count ) /*有数据*/
			{
				split_lines_pos[nbfields][0]	= start_of_field;
				split_lines_pos[nbfields][1]	= pos;
				nbfields++;
			}
			start_of_field	= pos + 1;
			count			= 0;
			linebreak		= 0;
		}
	}
	split_lines_pos[nbfields][0]	= start_of_field;   /*记录结束的位置*/
	split_lines_pos[nbfields][1]	= pos - 1;          /*记录结束的位置*/
	return nbfields + 1;
}

#endif

/*内容分隔为行，记录行首、行尾地址*/
uint8_t split_content( void )
{
	uint8_t count;
	uint8_t pos = 0;
	uint8_t	* p;
	int		nbfields	= 0;

	uint8_t start = 0;

	memset( split_lines_pos, 0, 64 );

	p		= textmsg.body;
	count	= 0;
	pos		= 0;
	while( pos < textmsg.len )
	{
		if( *p < 0x20 )
		{
			if( count ) /*有数据*/
			{
				split_lines_pos[nbfields][0]	= start;
				split_lines_pos[nbfields][1]	= pos - 1;
				nbfields++;
				count = 0;
			}
			pos++;
			p++;
		}else
		{
			if( count == 0 )        /*重新设置开始*/
			{
				start = pos;
			}
			if( *p > 0x7F ) /*有可能是汉字的开始或结束*/
			{
				if( count == 9 )    /*不够满行,生成一个新行*/
				{
					split_lines_pos[nbfields][0]	= start;
					split_lines_pos[nbfields][1]	= pos - 1;
					nbfields++;
					start=pos;		/*另开一行*/
				}
				pos+=2;  /*需要增加两个*/
				p+=2;
				count+=2;
				if(count==10)
				{
					split_lines_pos[nbfields][0]	= start;
					split_lines_pos[nbfields][1]	= pos-1;
					nbfields++;
					count=0;
				}	
			
			}else
			{
				count++;
				if( count == 10 ) /*正好*/
				{
					split_lines_pos[nbfields][0]	= start;
					split_lines_pos[nbfields][1]	= pos;
					nbfields++;
					count = 0;
				}
				pos++;
				p++;
			}
		}
	}

	if( count )
	{
		split_lines_pos[nbfields][0]	= start;
		split_lines_pos[nbfields][1]	= pos-1;
		return nbfields + 1;
	}else
	{
		return nbfields;
	}
}

/**/
uint8_t get_line( uint8_t pos, char *pout )
{
	char		* pdst = pout;
	signed char len;
	uint8_t		from, to;
	from	= split_lines_pos[pos][0];
	to		= split_lines_pos[pos][1];
	len		= to - from + 1;
	if( len > 0 )
	{
		memcpy( pdst, textmsg.body + from, len );
	}
	pdst[len] = '\0';
	return len;
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
	char buf1[32], buf2[32];
	uint8_t len, ret;

	if( textmsg_count == 0 )
	{
		lcd_fill( 0 );
		lcd_text12( ( 122 - 4 * 12 ) >> 1, 16, "[无记录]", 8, LCD_MODE_SET );
		lcd_update_all( );
		return;
	}
	if( item_pos >= textmsg_count )
	{
		return;                     /*item_pos=0可以环回*/
	}
	/*item_pos 范围[0..textmsg_count-1]*/
	if( item_pos_read != item_pos ) /*有变化，要读*/
	{
		jt808_textmsg_get( item_pos, &textmsg );
		item_pos_read		= item_pos;
		split_lines_count	= split_content( );
#if 1
		rt_kprintf( "split_lines_count=%d\r\n", split_lines_count );
		for( len = 0; len < split_lines_count; len++ )
		{
			ret = get_line( len, buf1 );
			rt_kprintf( "\r\nid=%d len=%d>%s", len, ret, buf1 );
		}
#endif
	}

	lcd_fill( 0 );
	sprintf( buf1, "[%02d] %02d/%02d/%02d %02d:%02d",
	         item_pos + 1,
	         YEAR( textmsg.datetime ),
	         MONTH( textmsg.datetime ),
	         DAY( textmsg.datetime ),
	         HOUR( textmsg.datetime ),
	         MINUTE( textmsg.datetime ) );

	line_pos	= 0; /**/
	len			= get_line( 0, buf2 );
	if( len )
	{
		lcd_text12( 0, 4, buf1, strlen( buf1 ), LCD_MODE_SET );
		lcd_text12( 0, 16, buf2, len, LCD_MODE_SET );
	}
	lcd_update_all( );
}

/*显示详细内容*/
static void display_detail( void )
{
	char buf1[32], buf2[32];
	int8_t	len1, len2;
	lcd_fill( 0 );
	if( line_pos < split_lines_count )
	{
		len1 = get_line( line_pos, buf1 );
		lcd_text12( 0, 4, buf1, len1, LCD_MODE_SET );
	}
	if( ( line_pos + 1 ) < split_lines_count )
	{
		len2 = get_line( line_pos + 1, buf2 );
		lcd_text12( 0, 16, buf2, len2, LCD_MODE_SET );
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
	pMenuItem->tick = rt_tick_get( );
	item_pos		= 0;
	item_pos_read	= 0xff;
	view_mode		= VIEW_ITEM;
	display_item( );
}

/*按键处理，分为ITEM和DETAIL两种方式查看*/
static void keypress( unsigned int key )
{
	switch( key )
	{
		case KEY_MENU:
			pMenuItem = &Menu_2_InforCheck;
			pMenuItem->show( );
			break;
		case KEY_OK:            /*查看模式切换*/
			view_mode ^= 0xFF;
			if( view_mode == VIEW_ITEM )
			{
				display_item( );
			}else
			{
				line_pos = 0;   /*重新显示*/
				display_detail( );
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
			}else
			{
				if( line_pos > 1 )
				{
					line_pos -= 2;
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
			}else
			{
				if( line_pos < split_lines_count - 2 ) /*一次变化两行*/
				{
					line_pos += 2;
				}
				display_detail( );
			}
			break;
	}
}

MENUITEM Menu_2_3_CentreTextStor =
{
	"文本消息查看",
	12,				  0,
	&show,
	&keypress,
	&timetick_default,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

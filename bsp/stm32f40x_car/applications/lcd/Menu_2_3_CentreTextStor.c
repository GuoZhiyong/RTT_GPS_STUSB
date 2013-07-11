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

static uint8_t	line_pos	= 0;        /*当前行号*/
static uint8_t	total_lines = 0;

/*查看模式*/
#define VIEW_ITEM	0
#define VIEW_DETAIL 0xFF

uint8_t view_mode = VIEW_ITEM;

/*最多支持32行记录每一行开始的位置*/
uint8_t split_lines_pos[32];
uint8_t split_lines_count=0;


/*
   对内容进行分行，以20个ASC或<CR>作为换行符
   记录每一行的首位置，返回总行数
 */
uint8_t split_content( void )
{
	uint8_t count;
	uint8_t pos = 0;
	char	* p;
	uint8_t linebreak	= 0;
	int		nbfields	= 0;
	uint8_t start_of_field;

	memset( split_lines_pos, 0, 32 );

	p = textmsg.body;
	/*剔除开始的不可见*/
	while( *p < 0x20 )
	{
		p++;
		pos++;
	}

	start_of_field	= pos;
	count			= 0;
	for(; pos < textmsg.len; pos++ )
	{
		if( *p < 0x20 )
		{
			linebreak = 1;
			p++;
		}else if( *p > 0x7F )   /*要增加2个*/
		{
			if( count == 19 )   /*不够了*/
			{
				linebreak = 1;
			}else
			{
				p		+= 2;
				count	+= 2;
			}
		}else
		{
			p++;
			count++;
		}
		if(( linebreak )||(count==20))
		{
			if( count ) /*有数据*/
			{
				split_lines_pos[nbfields] = start_of_field;
				nbfields++;
			}
			start_of_field = pos + 1;
			count=0;
			linebreak=0;
		}
	}
	return nbfields+1;
}

/*
   获取字符串内容,并分行
   指定源为ptextmsg,
   行号

   返回截获的长度
 */
static uint8_t get_line( uint8_t line_no, uint8_t *pout )
{
	uint8_t count;
	uint8_t * p;
	uint8_t * pdst		= pout;
	uint8_t line_curr	= 0;

	uint8_t buf[30]; /*最长20个字节*/
	uint8_t len = 0;

	uint8_t linebreak = 0;

	/*内部分行*/
	p		= textmsg.body;
	pdst	= buf;
	count	= 0;
	while( len < textmsg.len )
	{
		if( linebreak )                     /*到了分行*/
		{
			if( count )                     /*有数据*/
			{
				if( line_curr == line_no )  /*是需要的行*/
				{
					memcpy( pout, buf, count );
					return count;
				}
				line_curr++;
			}
			count		= 0;                /*重新开始找*/
			pdst		= buf;
			linebreak	= 0;
		}

		if( *p < 0x20 )                     /*应该判断回车换行的操作*/
		{
			linebreak = 1;
			len++;
			p++;
		}else if( *p > 0x7F )               /*要增加2个*/
		{
			if( count == 19 )               /*不够了*/
			{
				linebreak = 1;
			}else
			{
				*pdst++ = *p++;
				*pdst++ = *p++;
				count	+= 2;
				if( count == 20 )
				{
					linebreak = 1;
				}
			}
		}else
		{
			*pdst++ = *p++;
			count++;
			len++;
			if( count == 20 )
			{
				linebreak = 1;
			}
		}
	}
	if( count )                     /*结尾还有*/
	{
		if( line_curr == line_no )  /*是需要的行*/
		{
			memcpy( pout, buf, count );
			return count;
		}
	}
	return 0;
}

/*
   显示item 12个字节的头
   //	uint16_t	mn;         /*幻数 TEXT 0x5445
   //	uint32_t	id;        /*单增序号
   //	MYTIME		datetime;  /*收到的时间
   //	uint8_t	len;       /*长度

   第11字节是flag,
   数据从第12字节开始，汉字 12x12 每行10个 ASC 0612 每行 20个

 */
static void display_item( )
{
	uint8_t buf1[32], buf2[32];
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
		item_pos_read = item_pos;
		split_lines_count=split_content();
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
	uint8_t buf1[32], buf2[32];
	uint8_t len1, len2;
	lcd_fill( 0 );
	len1	= get_line( line_pos, buf1 );
	len2	= get_line( line_pos + 1, buf2 );
	if( len1 )
	{
		lcd_text12( 0, 4, buf1, len1, LCD_MODE_SET );
	}
	if( len2 )
	{
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
	uint8_t *pinfo;
	uint8_t res;
	item_pos		= 0;
	item_pos_read	= 0xff;
	display_item( );
}

/*按键处理，分为ITEM和DETAIL两种方式查看*/
static void keypress( unsigned int key )
{
	u8	CurrentDisplen	= 0;
	u8	i				= 0;

	switch( key )
	{
		case KEY_MENU:
			pMenuItem = &Menu_2_InforCheck;
			pMenuItem->show( );
			break;
		case KEY_OK: /*查看模式切换*/
			view_mode ^= 0xFF;
			if( view_mode == VIEW_ITEM )
			{
				display_item( );
			}else
			{
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
				item_pos++;
				display_item( );
			}else
			{
				line_pos += 2;
				display_detail( );
			}
			break;
	}
}

/**/
static void timetick( unsigned int systick )
{
	CounterBack++;
	if( CounterBack != MaxBankIdleTime )
	{
		return;
	} else
	{
		pMenuItem = &Menu_1_Idle;
		pMenuItem->show( );
	}
}

MENUITEM Menu_2_3_CentreTextStor =
{
	"文本消息查看",
	12,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

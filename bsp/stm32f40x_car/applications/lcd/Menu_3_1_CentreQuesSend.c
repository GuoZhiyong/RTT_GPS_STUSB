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

/*�鿴ģʽ*/
#define VIEW_ITEM	0
#define VIEW_DETAIL 0xFF

static CENTER_ASK	center_ask;             /*���ݽ�������Ϣ*/
static uint8_t		item_pos		= 0;    /*��ǰҪ���ʵ�*/
static uint8_t		item_pos_read	= 0xff; /*��ǰ�Ѷ�ȡ��*/

static uint8_t		line_pos = 0;           /*��ǰ�к�*/

static uint8_t		view_mode = VIEW_ITEM;

struct _stu_question
{
	char	len;                            /*���ⳤ��*/
	char	* question;                     /*����*/
	char	ans_count;                      /*��ѡ����*/
	struct
	{
		char	id;                         /*��ѡid*/
		char	len;                        /*��ѡ�𰸳���*/
		char	* ans;                      /*��ѡ��*/
	} ans[5];
}				question;

static DISP_ROW disp_row[32];
static uint8_t	split_lines_count = 0;


/*����������Ϣ*/
uint8_t analy_question( void )
{
	uint8_t		*p;
	uint8_t		len;
	uint8_t		count;
	uint8_t		rows=0;
	uint8_t		i = 0;
	uint8_t		ans_id;
	uint16_t	ans_len;

	uint16_t	pos, end;

	/*���ⲿ��*/
	len		= center_ask.body[0];
	p		= center_ask.body + 1;
	count	= split_content( p, len, disp_row );
	for( i = 0; i < count; i++ )
	{
		disp_row[i].attrib = 0x8000;
	}
	rt_kprintf( "1.count=%d\r\n", count );
	rows += count;

	end = center_ask.len;
	
	pos =  1 + len;        /*���е�ƫ�ƶ��������body����ģ�����Ҫ����*/
	/*��ѡ���б�*/
	while( pos < end )
	{
		p		= center_ask.body + pos;  /*һ���ֽڵ�id,�ͳ���(2bytes)*/
		ans_id	= *p++;
		ans_len = ( *p++ ) << 8;
		ans_len |= *p++;
		count	= split_content( p, ans_len, disp_row + rows );
		for( i = rows; i < rows + count; i++ )
		{
			disp_row[i].attrib = ans_id;
			disp_row[i].start+=(pos+3);
		}
		rows	+= count;
		pos		+= ( 3 + ans_len );
		rt_kprintf( "count=%d rows=%d pos=%d\r\n", count, rows, pos );
	}

	return rows;
}

/**/
static uint8_t get_line( uint8_t pos, char *pout )
{
	char		* pdst = pout;
	memcpy( pdst, center_ask.body+disp_row[pos].start, disp_row[pos].count );
	pdst[disp_row[pos].count] = '\0';
	return disp_row[pos].count;
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
	char	buf1[32], buf2[32];
	uint8_t len, ret;

	if( center_ask_count == 0 )
	{
		lcd_fill( 0 );
		lcd_text12( ( 122 - 8 * 12 ) >> 1, 16, "[�����������·�]", 16, LCD_MODE_SET );
		lcd_update_all( );
		return;
	}
	if( item_pos >= center_ask_count )
	{
		return;                     /*item_pos=0���Ի���*/
	}
	/*item_pos ��Χ[0..textmsg_count-1]*/
	if( item_pos_read != item_pos ) /*�б仯��Ҫ��*/
	{
		jt808_center_ask_get( item_pos, &center_ask );
		item_pos_read = item_pos;
		split_lines_count=analy_question( );
	}

	lcd_fill( 0 );
	sprintf( buf1, "[%02d] %02d/%02d/%02d %02d:%02d",
	         item_pos + 1,
	         YEAR( center_ask.datetime ),
	         MONTH( center_ask.datetime ),
	         DAY( center_ask.datetime ),
	         HOUR( center_ask.datetime ),
	         MINUTE( center_ask.datetime ) );

	line_pos	= 0; /**/
	len			= get_line( 0, buf2 );
	if( len )
	{
		lcd_text12( 0, 4, buf1, strlen( buf1 ), LCD_MODE_SET );
		lcd_text12( 0, 16, buf2, len, LCD_MODE_SET );
	}
	lcd_update_all( );
}

/*��ʾ��ϸ����*/
static void display_detail( void )
{
	char	buf1[32], buf2[32];
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

/*��ʾ�����·���Ϣ*/
static void show( void )
{
	pMenuItem->tick = rt_tick_get( );
	item_pos		= 0;
	item_pos_read	= 0xff;
	view_mode		= VIEW_ITEM;
	display_item( );
}

/*����������ΪITEM��DETAIL���ַ�ʽ�鿴*/
static void keypress( unsigned int key )
{
	switch( key )
	{
		case KEY_MENU:
			pMenuItem = &Menu_2_InforCheck;
			pMenuItem->show( );
			break;
		case KEY_OK:            /*�鿴ģʽ�л�*/
			view_mode ^= 0xFF;
			if( view_mode == VIEW_ITEM )
			{
				display_item( );
			}else
			{
				line_pos = 0;   /*������ʾ*/
				display_detail( );
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
				if( line_pos < split_lines_count - 2 ) /*һ�α仯����*/
				{
					line_pos += 2;
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

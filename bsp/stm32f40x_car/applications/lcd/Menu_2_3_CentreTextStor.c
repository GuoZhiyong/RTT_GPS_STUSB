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
#include <string.h>
#include "sed1520.h"

static TEXTMSG	textmsg;                /*���ݽ�������Ϣ������TEXTMSG_HEAD*/
static uint8_t	item_pos		= 0;    /*��ǰҪ���ʵ�*/
static uint8_t	item_pos_read	= 0xff; /*��ǰ�Ѷ�ȡ��*/

static uint8_t	line_pos	= 0;        /*��ǰ�к�*/
static uint8_t	total_lines = 0;

/*�鿴ģʽ*/
#define VIEW_ITEM	0
#define VIEW_DETAIL 0xFF

uint8_t view_mode = VIEW_ITEM;

/*���֧��32�м�¼ÿһ�п�ʼ��λ��*/
uint8_t split_lines_pos[32];
uint8_t split_lines_count=0;


/*
   �����ݽ��з��У���20��ASC��<CR>��Ϊ���з�
   ��¼ÿһ�е���λ�ã�����������
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
	/*�޳���ʼ�Ĳ��ɼ�*/
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
		}else if( *p > 0x7F )   /*Ҫ����2��*/
		{
			if( count == 19 )   /*������*/
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
			if( count ) /*������*/
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
   ��ȡ�ַ�������,������
   ָ��ԴΪptextmsg,
   �к�

   ���ؽػ�ĳ���
 */
static uint8_t get_line( uint8_t line_no, uint8_t *pout )
{
	uint8_t count;
	uint8_t * p;
	uint8_t * pdst		= pout;
	uint8_t line_curr	= 0;

	uint8_t buf[30]; /*�20���ֽ�*/
	uint8_t len = 0;

	uint8_t linebreak = 0;

	/*�ڲ�����*/
	p		= textmsg.body;
	pdst	= buf;
	count	= 0;
	while( len < textmsg.len )
	{
		if( linebreak )                     /*���˷���*/
		{
			if( count )                     /*������*/
			{
				if( line_curr == line_no )  /*����Ҫ����*/
				{
					memcpy( pout, buf, count );
					return count;
				}
				line_curr++;
			}
			count		= 0;                /*���¿�ʼ��*/
			pdst		= buf;
			linebreak	= 0;
		}

		if( *p < 0x20 )                     /*Ӧ���жϻس����еĲ���*/
		{
			linebreak = 1;
			len++;
			p++;
		}else if( *p > 0x7F )               /*Ҫ����2��*/
		{
			if( count == 19 )               /*������*/
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
	if( count )                     /*��β����*/
	{
		if( line_curr == line_no )  /*����Ҫ����*/
		{
			memcpy( pout, buf, count );
			return count;
		}
	}
	return 0;
}

/*
   ��ʾitem 12���ֽڵ�ͷ
   //	uint16_t	mn;         /*���� TEXT 0x5445
   //	uint32_t	id;        /*�������
   //	MYTIME		datetime;  /*�յ���ʱ��
   //	uint8_t	len;       /*����

   ��11�ֽ���flag,
   ���ݴӵ�12�ֽڿ�ʼ������ 12x12 ÿ��10�� ASC 0612 ÿ�� 20��

 */
static void display_item( )
{
	uint8_t buf1[32], buf2[32];
	uint8_t len, ret;

	if( textmsg_count == 0 )
	{
		lcd_fill( 0 );
		lcd_text12( ( 122 - 4 * 12 ) >> 1, 16, "[�޼�¼]", 8, LCD_MODE_SET );
		lcd_update_all( );
		return;
	}
	if( item_pos >= textmsg_count )
	{
		return;                     /*item_pos=0���Ի���*/
	}
	/*item_pos ��Χ[0..textmsg_count-1]*/
	if( item_pos_read != item_pos ) /*�б仯��Ҫ��*/
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

/*��ʾ��ϸ����*/
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

/*��ʾ�����·���Ϣ*/
static void show( void )
{
	uint8_t *pinfo;
	uint8_t res;
	item_pos		= 0;
	item_pos_read	= 0xff;
	display_item( );
}

/*����������ΪITEM��DETAIL���ַ�ʽ�鿴*/
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
		case KEY_OK: /*�鿴ģʽ�л�*/
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
	"�ı���Ϣ�鿴",
	12,
	&show,
	&keypress,
	&timetick,
	&msg,
	(void*)0
};

/************************************** The End Of File **************************************/

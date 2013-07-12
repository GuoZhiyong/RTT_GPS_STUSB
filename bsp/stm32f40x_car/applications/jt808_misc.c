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
#include <string.h>
#include "jt808_misc.h"
#include "jt808.h"
#include "jt808_gps.h"
#include "sst25.h"

#define TEXTMSG_START		0x3B3000
#define TEXTMSG_SECTORS		2
#define TEXTMSG_BLOCK_SIZE	256
#define TEXTMSG_END			( TEXTMSG_START + TEXTMSG_SECTORS * 4096 )

#define EVENT_START		( TEXTMSG_END )
#define EVENT_SECTORS	1
#define EVENT_END		( EVENT_START + EVENT_SECTORS * 4096 )

#define ASK_START	( EVENT_END )
#define ASK_SECTORS 2
#define ASK_END		( ASK_START + ASK_SECTORS * 4096 )

uint32_t	textmsg_curr_addr;  /*当前写入最新数据的消息地址*/
uint32_t	textmsg_curr_id;
uint8_t		textmsg_count = 0;

#if 0
struct _sector_info
{
	uint32_t	start;          /*起始地址*/
	uint8_t		sectors;        /*占用的扇区数*/
	uint8_t		block_size;     /*每个记录块大小*/
	uint32_t	addr_wr;        /*当前写入地址*/
	uint32_t	addr_rd;        /*当前读出地址*/
} sector_info[4] =
{
	{ TEXTMSG_START, TEXTMSG_SECTORS, 128, TEXTMSG_END, TEXTMSG_END },
};
#endif


/*
   文本信息保存
   要不要保证一包信息在一个扇区内，不会跨扇区保存
   在这里面，借用了info头部来存储TEXTMSG_HEAD,这样回写更方便
 */
uint8_t jt808_textmsg_put( uint8_t* pinfo )
{
	uint8_t		* psrc	= pinfo;
	uint16_t	len		= ( ( psrc[2] << 8 ) | psrc[3] ) & 0x3FF - 1; /*有个标志字节*/
	uint8_t		flag	= psrc[12];

	uint32_t	addr;
	uint8_t		count, count_need;

	TEXTMSG		textmsg;

	memset( (uint8_t*)&textmsg, 0, 256 );

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	textmsg_curr_addr += 256; /*定位到下一个*/
	if( textmsg_curr_addr >= TEXTMSG_END )
	{
		textmsg_curr_addr = TEXTMSG_START;
	}
	textmsg_curr_id++;
	textmsg.id			= textmsg_curr_id;
	textmsg.datetime	= mytime_now;
	if( len > ( 256 - 9 ) )
	{
		len = 256 - 9;
	}
	textmsg.len = len;

/*保存记录,跳出开始的标志位*/
	memcpy( textmsg.body, pinfo + 13, len );
/*保存记录内容*/
	sst25_write_back( textmsg_curr_addr, (uint8_t*)&textmsg, 256 );
	rt_sem_release( &sem_dataflash );

	if( textmsg_count <= TEXTMSG_SECTORS * 4096 / 256 )
	{
		textmsg_count++;
	}
}

/*
   读取一条文本信息
   以当前为基准，偏移的记录数
   返回值 0:没找到
    1:找到
 */
void jt808_textmsg_get( uint8_t index, TEXTMSG* pout )
{
	uint8_t		count;
	uint32_t	addr;
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );

	addr = textmsg_curr_addr;
	for( count = 0; count < index; count++ )
	{
		if( addr == TEXTMSG_START )
		{
			addr = TEXTMSG_END;
		}
		addr -= 256;
	}
	sst25_read( addr, (uint8_t*)pout, 256 );
	rt_sem_release( &sem_dataflash );
}

/*收到中心下发的0x8300信息*/
void jt808_misc_0x8300( uint8_t *pmsg )
{
	uint8_t		flag	= pmsg[12];
	uint16_t	id		= ( ( pmsg[0] << 8 ) | pmsg[1] );
	uint16_t	seq		= ( ( pmsg[10] << 8 ) | pmsg[11] );
	uint16_t	len		= ( ( pmsg[2] << 8 ) | pmsg[3] ) & 0x3FF;
	uint8_t		* ptts, *p, *psrc;
	uint16_t	i;
	if( flag & 0x01 )               /*紧急，直接弹出*/
	{
	}
	if( flag & 0x04 )               /*终端显示器显示,指示有未读信息*/
	{
		jt808_textmsg_put( pmsg );
	}
	if( flag & 0x08 )               /*TTS播报*/
	{
#if 0
		ptts = rt_malloc( len );    /*长度-标志字节=>转成hexasc,多分配了结束符*/
		if( ptts != RT_NULL )
		{
			p		= ptts;
			psrc	= pmsg + 13;
			for( i = 0; i < ( len - 1 ); i++ )
			{
				*p++ = *psrc++;
			}
			*p = '\0';
			tts_write( ptts );
			rt_free( ptts );
		}
#else
		tts_write( pmsg + 13, len - 1 );
#endif
	}
	if( flag & 0x10 )   /*广告屏*/
	{
	}
	if( flag & 0x20 )   /*紧急*/
	{
	}
/*返回应答*/
	jt808_tx_0x0001( seq, id, 0x0 );
}

/*遍历所有元素，执行操作*/
void foreach_event( uint8_t *pdata )
{
	uint32_t	i;
	EVENT		*pevent;

	for( i = 0; i < 128; i++ )
	{
		pevent = (EVENT*)( pdata + 32 * i );

		if( pdata[i] == 'E' )
		{
		}
	}
}

/*收到中心下发的0x8300信息*/
void jt808_misc_0x8301( uint8_t *pmsg )
{
	uint8_t		type	= pmsg[12];
	uint16_t	id		= ( ( pmsg[0] << 8 ) | pmsg[1] );
	uint16_t	seq		= ( ( pmsg[10] << 8 ) | pmsg[11] );
	uint16_t	len		= ( ( pmsg[2] << 8 ) | pmsg[3] ) & 0x3FF;
	uint8_t		* ptts, *p, *psrc;
	uint16_t	i;
	uint8_t		*pevent;
	uint8_t		res = 0;

	pevent = rt_malloc( 4096 );
	if( pevent == RT_NULL )
	{
		res = 1;
		goto lbl_end_8301;
	}

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	sst25_read( EVENT_START, pevent, 4096 );

	if( type == 0 ) /*删除现在所有事件*/
	{
		sst25_erase_4k( EVENT_START );
	}
	if( type == 1 ) /*更新事件*/
	{
	}
	if( type == 2 ) /*追加事件*/
	{
	}
	if( type == 3 ) /*修改事件*/
	{
	}
	if( type == 4 ) /*删除特定事件*/
	{
	}
	rt_sem_release( &sem_dataflash );

/*返回应答*/
lbl_end_8301:
	jt808_tx_0x0001( seq, id, res );
}

/*要保存一下吗?*/
static jt808_ask_put( uint8_t * pinfo )
{
}


/*
   提问下发
   有提问应答
 */
void jt808_misc_0x8302( uint8_t *pmsg )
{
	uint8_t		flag	= pmsg[12];
	uint16_t	id		= ( ( pmsg[0] << 8 ) | pmsg[1] );
	uint16_t	seq		= ( ( pmsg[10] << 8 ) | pmsg[11] );
	uint16_t	len		= ( ( pmsg[2] << 8 ) | pmsg[3] ) & 0x3FF;
	uint16_t	ans_len = 0;
	uint8_t		*p;
	uint16_t	ask_len = 0;
	uint16_t	pos;
	if( flag & 0x01 )                       /*紧急，直接弹出*/
	{
	}
	if( flag & 0x08 )                       /*TTS播报，分隔一下,*/
	{
		tts_write( "提问", 4 );
		ask_len = pmsg[13];                 /*问题内容长度*/
		tts_write( pmsg + 14, ask_len );    /*问题*/
		tts_write( "请选择", 6 );
		pos = 14 + ask_len;
		while( pos < len )
		{
			p		= pmsg + pos;           /*指向候选答案列表*/
			p[0]	= p[0] + 0x30;          /*答案ID变为可播报的*/
			ans_len = ( p[1] << 8 ) | p[2]; /*答案长度*/
			p[1]	= ',';                  /*替换为逗号，可播报*/
			p[2]	= ',';
			pos		= pos + ans_len + 3;    /*加3是因为答案ID和答案内容长度没有计算*/
		}
		tts_write( pmsg + 14 + ask_len, len - ask_len - 2 );
	}
	if( flag & 0x10 )                       /*广告屏*/
	{
	}
	/*返回应答*/
	jt808_tx_0x0001( seq, id, 0x0 );
}

/*信息点播菜单设置*/
void jt808_misc_0x8303( uint8_t *pmsg )
{
}

/*信息服务*/
void jt808_misc_0x8304( uint8_t *pmsg )
{
}

/*电话回拨*/
void jt808_misc_0x8400( uint8_t *pmsg )
{
	uint8_t		dialbuf[32];
	uint8_t		flag	= pmsg[12];
	uint16_t	len		= ( ( pmsg[2] << 8 ) | pmsg[3] ) & 0x3FF;
	strcpy(dialbuf,"ATD");
	strncpy(dialbuf+3,&pmsg[13],len-1);
	strcat(dialbuf,";\r\n");
	GPIO_ResetBits( GPIOD, GPIO_Pin_9 );             /*开功放*/
	at(dialbuf);
	
}

/*设置电话本*/
void jt808_misc_0x8401( uint8_t *pmsg )
{
}

/*
   遍历当前的记录
   256字节一个block
   要求50条

 */
uint8_t jt808_textmsg_init( void )
{
	uint32_t	addr;
	uint32_t	id = 0;
	uint8_t		buf[16];

	textmsg_curr_addr	= TEXTMSG_END;               /*指向最后*/
	textmsg_curr_id		= 0;
	textmsg_count		= 0;

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	for( addr = TEXTMSG_START; addr < TEXTMSG_END; addr += 256 )
	{
		sst25_read( addr, buf, 16 );
		id = ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | ( buf[3] );
		if( id != 0xFFFFFFFF )
		{
			textmsg_count++;
			if( id > textmsg_curr_id )
			{
				textmsg_curr_addr	= addr;
				textmsg_curr_id		= id;
			}
		}
	}
	rt_sem_release( &sem_dataflash );
}

/*电话簿*/
uint8_t jt808_phonebook_init( void )
{
}

/*初始化吧*/
void jt808_misc_init( void )
{
	jt808_textmsg_init( );
}

/************************************** The End Of File **************************************/

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

/*中心提问*/
#define CENTER_ASK_START	( EVENT_END )
#define CENTER_ASK_SECTORS 2
#define CENTER_ASK_END		( CENTER_ASK_START + CENTER_ASK_SECTORS * 4096 )

/*信息点播*/
#define INFO_ONDEMAND_START		( CENTER_ASK_END )
#define INFO_ONDEMAND_SECTORS	1
#define INFO_ONDEMAND_END		( INFO_ONDEMAND_START + INFO_ONDEMAND_SECTORS * 4096 )


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


uint32_t	textmsg_curr_addr;  /*当前写入最新数据的消息地址*/
uint32_t	textmsg_curr_id;
uint8_t		textmsg_count = 0;

/*
   文本信息保存
   要不要保证一包信息在一个扇区内，不会跨扇区保存
   在这里面，借用了info头部来存储TEXTMSG_HEAD,这样回写更方便
 */
uint8_t jt808_textmsg_put( uint8_t* pinfo )
{
	uint8_t		* psrc	= pinfo;
	uint16_t	len		= ( ( ( psrc[2] << 8 ) | psrc[3] ) & 0x3FF ) - 1; /*有个标志字节*/
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
	if( len > ( 256 - 10 ) )
	{
		len = 256 - 10;
	}
	textmsg.len = len;
	textmsg.flag=1;    /*未读*/

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
	if( flag & 0x01 )   /*紧急，直接弹出*/
	{
	}
	if( flag & 0x04 )   /*终端显示器显示,指示有未读信息*/
	{
		jt808_textmsg_put( pmsg );
	}
	if( flag & 0x08 )   /*TTS播报*/
	{
		tts_write( pmsg + 13, len - 1 );
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



/*本来想用指向指针的指针来传递参数 ;-( */
uint8_t* event_buf = RT_NULL;




/*
   通过id查找事件
   如果找到了，返回找到的地址
   没找到，返回第一个可用的地址，如果没有可用，返回0xFFFF
 */
uint8_t event_find_by_id( uint8_t *peventbuf, uint8_t id, uint16_t *addr )
{
	uint8_t		*pinfo = peventbuf;
	uint32_t	i;
	EVENT		*pevent;

	*addr = 0xFFFF;
	for( i = 0; i < 4096; i += 64 )
	{
		pevent = (EVENT*)( pinfo + i );
		if( pevent->flag == 'E' )
		{
			if( pevent->id == id )
			{
				*addr = i;
				return 1;
			}
		}else /*第一个可用的地址*/
		{
			if( *addr == 0xFFFF )
			{
				*addr = i;
			}
		}
	}
	return 0;
}

/*
   获取全部事件,动态生成.
   返回 记录数
 */
uint8_t jt808_event_get( void )
{
	uint8_t		count = 0;
	uint8_t		buf[16];
	uint32_t	addr;

	if( event_buf != RT_NULL )
	{
		rt_free( event_buf );
	}
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	for( addr = EVENT_START; addr < EVENT_END; addr += 64 )
	{
		sst25_read( addr, buf, 16 );
		if( buf[0] == 'E' )
		{
			count++;
		}else
		{
			break;
		}
	}

	if( count )
	{
		event_buf = rt_malloc( count * 64 );
		if( event_buf == RT_NULL )
		{
			count = 0;
		}else
		{
			sst25_read( EVENT_START, event_buf, count * 64 );
		}
	}
	rt_sem_release( &sem_dataflash );
	return count;
}

/*收到中心下发的0x8301事件设置信息*/
void jt808_misc_0x8301( uint8_t *pmsg )
{
	uint8_t		type	= pmsg[12];
	uint16_t	id		= ( ( pmsg[0] << 8 ) | pmsg[1] );
	uint16_t	seq		= ( ( pmsg[10] << 8 ) | pmsg[11] );
	uint16_t	i, j;
	uint8_t		* pdata;
	uint8_t		*pevtbuf;
	uint8_t		count, res = 0, len = 0;
	uint16_t	addr;
	uint16_t	tmpbuf[64];

	pevtbuf = rt_malloc( 4096 );
	if( pevtbuf == RT_NULL )
	{
		res = 1;
		goto lbl_end_8301;
	}
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	sst25_read( EVENT_START, pevtbuf, 4096 );

	if( type == 0 )                                 /*删除现在所有事件*/
	{
		sst25_erase_4k( EVENT_START );
		goto lbl_end_8301;
	}else if( type == 4 )                           /*删除特定事件*/
	{
		pdata = pmsg + 14;
		for( count = 0; count < pmsg[13]; count++ ) /*事件总数*/
		{
			if( event_find_by_id( pevtbuf, *pdata, &addr ) )
			{
				memset( pevtbuf + addr, 0xff, sizeof( EVENT ) );
			}
			pdata++;
		}
	}else /*1.更新 2 追加 3 修改 凡是id一样的就更新，没有的就增加*/
	{
		pdata = pmsg + 14;
		for( count = 0; count < pmsg[13]; count++ ) /*事件总数*/
		{
			len = pdata[1];
			event_find_by_id( pevtbuf, *pdata, &addr );
			if( addr != 0xFFFF ) /*不管找到没找到，反正有个可用的地址*/
			{
				if( len > 62 )
				{
					len = 62;
				}
				memset(pevtbuf+addr,0,64);
				pevtbuf[addr] = 'E';
				memcpy( pevtbuf + addr + 1, pdata, len+2 );
			}
			pdata += ( len + 2 );
		}
	}
/*重新整理,去除空闲的*/
	addr	= 0xFFFF;               /*第一个为空的地址*/
	count	= 0;                    /*记录数*/
	for( i = 0; i < 4096; i += 64 )
	{
		if( pevtbuf[i] == 'E' )     /*是有效地址*/
		{
			count++;
			if( addr != 0xFFFF )    /*有空的*/
			{
				memcpy( pevtbuf + addr, pevtbuf + i, sizeof( EVENT ) );
				memset( pevtbuf + i, 0xFF, sizeof( EVENT ) );
				addr = i;           /*成为新的未用的*/
			}
		}else /*没有用的地址*/
		{
			if( addr == 0xFFFF )
			{
				addr = i;
			}
		}
	}


/*排序
   for(int i = 0;i<cnt;i++)
   {
   for(j=0;j<cnt-1;j++)
   {
        if(a[j]>a[j+1])
        {
               tmp = a[j];
               a[j] = a[j+1];
               a[j+1] = tmp;
     }
   }
   }
 */
	for( i = 0; i < count; i++ )
	{
		for( j = 0; j < ( count - 1 ); j++ )
		{
			if( pevtbuf[j * 64 + 1] > pevtbuf[j * 64 + 65] )
			{
				memcpy( tmpbuf, pevtbuf + j * 64, 64 );
				memcpy( pevtbuf + j * 64 , pevtbuf + j * 64 + 64, 64 );
				memcpy( pevtbuf + j * 64 + 64, tmpbuf, 64 );
			}
		}
	}
	sst25_write_back( EVENT_START, pevtbuf, 4096 );

lbl_end_8301:
	rt_sem_release( &sem_dataflash );
	jt808_tx_0x0001( seq, id, res ); /*返回应答*/
}

/***********************************************************
* Function:
* Description:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
uint8_t jt808_event_init( void )
{
}



uint32_t	center_ask_curr_addr;  /*当前写入最新数据的消息地址*/
uint32_t	center_ask_curr_id;
uint8_t		center_ask_count = 0;


/*
   文本信息保存
   要不要保证一包信息在一个扇区内，不会跨扇区保存
   在这里面，借用了info头部来存储TEXTMSG_HEAD,这样回写更方便
 */
uint8_t jt808_center_ask_put( uint8_t* pinfo )
{
	uint8_t		* psrc	= pinfo;
	uint16_t	len		= ( ( ( psrc[2] << 8 ) | psrc[3] ) & 0x3FF ) - 1; /*有个标志字节*/
	uint8_t		flag	= psrc[12];

	uint32_t	addr;
	uint8_t		count, count_need;

	CENTER_ASK		center_ask_msg;

	memset( (uint8_t*)&center_ask_msg, 0, 256 );

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	center_ask_curr_addr += 256; /*定位到下一个*/
	if( center_ask_curr_addr >= CENTER_ASK_END )
	{
		center_ask_curr_addr = CENTER_ASK_START;
	}
	center_ask_curr_id++;
	center_ask_msg.id			= center_ask_curr_id;
	center_ask_msg.datetime	= mytime_now;
	if( len > ( 256 - 10) )
	{
		len = 256 - 10;
	}
	center_ask_msg.len = len;
	center_ask_msg.flag=1;   /*未回答*/

/*保存记录,跳出开始的标志位*/
	memcpy( center_ask_msg.body, pinfo + 13, len );
/*保存记录内容*/
	sst25_write_back( center_ask_curr_addr, (uint8_t*)&center_ask_msg, 256 );
	rt_sem_release( &sem_dataflash );

	if( center_ask_count <= CENTER_ASK_SECTORS * 4096 / 256 )
	{
		center_ask_count++;
	}
}

/*
   读取一条文本信息
   以当前为基准，偏移的记录数
   返回值 0:没找到
    1:找到
 */
void jt808_center_ask_get( uint8_t index, CENTER_ASK* pout )
{
	uint8_t		count;
	uint32_t	addr;
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );

	addr = center_ask_curr_addr;
	for( count = 0; count < index; count++ )
	{
		if( addr == CENTER_ASK_START )
		{
			addr = CENTER_ASK_END;
		}
		addr -= 256;
	}
	sst25_read( addr, (uint8_t*)pout, 256 );
	rt_sem_release( &sem_dataflash );
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
		jt808_center_ask_put( pmsg );
	}
	if( flag & 0x08 )                       /*TTS播报，分隔一下,*/
	{
		tts_write( "中心下发提问", 12 );
		ask_len = pmsg[13];                 /*问题内容长度*/
		tts_write( pmsg + 14, ask_len );    /*问题*/
		tts_write( "请选择", 6 );
		pos = 14 + ask_len;  /*指向答案项*/
		rt_kprintf("pos=%d len=%d\r\n",pos,len);
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

}



uint8_t* info_buffer;

/*信息点播菜单设置*/
uint8_t info_find_by_id( uint8_t *peventbuf, uint8_t id, uint16_t *addr )
{
	uint8_t		*pinfo = peventbuf;
	uint32_t	i;
	EVENT		*pevent;

	*addr = 0xFFFF;
	for( i = 0; i < 4096; i += 64 )
	{
		pevent = (EVENT*)( pinfo + i );
		if( pevent->flag == 'I' )
		{
			if( pevent->id == id )
			{
				*addr = i;
				return 1;
			}
		}else /*第一个可用的地址*/
		{
			if( *addr == 0xFFFF )
			{
				*addr = i;
			}
		}
	}
	return 0;
}

/*信息点播菜单设置*/
void jt808_misc_0x8303( uint8_t *pmsg )
{
	uint8_t		type	= pmsg[12];
	uint16_t	id		= ( ( pmsg[0] << 8 ) | pmsg[1] );
	uint16_t	seq		= ( ( pmsg[10] << 8 ) | pmsg[11] );
	uint16_t	i, j;
	uint8_t		* pdata;
	//uint8_t		*pevtbuf;
	uint8_t		count, res = 0, len = 0;
	uint16_t	addr;
	uint16_t	tmpbuf[64];

#if 0
	pevtbuf = rt_malloc( 4096 );
	if( pevtbuf == RT_NULL )
	{
		res = 1;
		goto lbl_end_8301;
	}
#endif
	uint8_t pevtbuf[4096];

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	sst25_read( INFO_ONDEMAND_START, pevtbuf, 4096 );

	if( type == 0 )                                 /*删除现在所有事件*/
	{
		sst25_erase_4k( INFO_ONDEMAND_START );
		goto lbl_end_8303;
	}else if( type == 4 )                           /*删除特定事件*/
	{
		pdata = pmsg + 14;
		for( count = 0; count < pmsg[13]; count++ ) /*事件总数*/
		{
			if( info_find_by_id( pevtbuf, *pdata, &addr ) )
			{
				memset( pevtbuf + addr, 0xff, sizeof( EVENT ) );
			}
			pdata++;
		}
	}else /*1.更新 2 追加 3 修改 凡是id一样的就更新，没有的就增加*/
	{
		pdata = pmsg + 14;
		for( count = 0; count < pmsg[13]; count++ ) /*事件总数*/
		{
			len = pdata[1];
			info_find_by_id( pevtbuf, *pdata, &addr );
			if( addr != 0xFFFF ) /*不管找到没找到，反正有个可用的地址*/
			{
				if( len > 63 )
				{
					len = 63;
				}
				pevtbuf[addr] = 'E';
				memcpy( pevtbuf + addr + 1, pdata, len );
			}
			pdata += ( len + 2 );
		}
	}
	/*重新整理,去除空闲的*/
	addr	= 0xFFFF;               /*第一个为空的地址*/
	count	= 0;                    /*记录数*/
	for( i = 0; i < 4096; i += 64 )
	{
		if( pevtbuf[i] == 'I' )     /*是有效地址*/
		{
			count++;
			if( addr != 0xFFFF )    /*有空的*/
			{
				memcpy( pevtbuf + addr, pevtbuf + i, sizeof( EVENT ) );
				memset( pevtbuf + i, 0xFF, sizeof( EVENT ) );
				addr = i;           /*成为新的未用的*/
			}
		}else /*没有用的地址*/
		{
			if( addr == 0xFFFF )
			{
				addr = i;
			}
		}
	}


	/*排序
	   for(int i = 0;i<cnt;i++)
	   {
	   for(j=0;j<cnt-1;j++)
	   {
	   if(a[j]>a[j+1])
	   {
	      tmp = a[j];
	      a[j] = a[j+1];
	      a[j+1] = tmp;
	   }
	   }
	   }
	 */
	for( i = 0; i < count; i++ )
	{
		for( j = 0; j < ( count - 1 ); j++ )
		{
			if( pevtbuf[j * 64 + 1] > pevtbuf[j * 64 + 65] )
			{
				memcpy( tmpbuf, pevtbuf + j * 64, 64 );
				memcpy( pevtbuf + j * 64 + 1, pevtbuf + j * 64 + 65, 64 );
				memcpy( pevtbuf + j * 64 + 65, tmpbuf, 64 );
			}
		}
	}
	sst25_write_back( INFO_ONDEMAND_START, pevtbuf, 4096 );

lbl_end_8303:
	rt_sem_release( &sem_dataflash );
	jt808_tx_0x0001( seq, id, res ); /*返回应答*/
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
	strcpy( dialbuf, "ATD" );
	strncpy( dialbuf + 3, &pmsg[13], len - 1 );
	strcat( dialbuf, ";\r\n" );
	GPIO_ResetBits( GPIOD, GPIO_Pin_9 ); /*开功放*/
	at( dialbuf );
}

/*设置电话本*/
void jt808_misc_0x8401( uint8_t *pmsg )
{
}

/***********************************************************
* Function:
* Description:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void jt808_misc_0x8500( uint8_t *pmsg )
{
}

/*
   遍历当前的记录
   256字节一个block
   要求50条

 */
void jt808_textmsg_init( void )
{
	uint32_t	addr;
	uint32_t	id = 0;
	uint8_t		buf[16];

	textmsg_curr_addr	= TEXTMSG_END; /*指向最后*/
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


/*
   遍历当前的记录
   256字节一个block
   要求50条

 */
void jt808_center_ask_init( void )
{
	uint32_t	addr;
	uint32_t	id = 0;
	uint8_t		buf[16];

	center_ask_curr_addr	= TEXTMSG_END; /*指向最后*/
	center_ask_curr_id		= 0;
	center_ask_count		= 0;

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	for( addr = CENTER_ASK_START; addr < CENTER_ASK_END; addr += 256 )
	{
		sst25_read( addr, buf, 16 );
		id = ( buf[0] << 24 ) | ( buf[1] << 16 ) | ( buf[2] << 8 ) | ( buf[3] );
		if( id != 0xFFFFFFFF )
		{
			center_ask_count++;
			if( id > center_ask_curr_id )
			{
				center_ask_curr_addr	= addr;
				center_ask_curr_id		= id;
			}
		}
	}
	rt_sem_release( &sem_dataflash );
}



/*初始化吧*/
void jt808_misc_init( void )
{
	jt808_center_ask_init();
	jt808_textmsg_init( );
	jt808_event_init( );

}

/************************************** The End Of File **************************************/

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
#include <stdio.h>

#include <board.h>
#include <rtthread.h>
#include <finsh.h>

#include "stm32f4xx.h"
#include "jt808.h"
#include "msglist.h"
#include "jt808_sprintf.h"
#include "sst25.h"

#include "m66.h"

#include "jt808_param.h"
#include "jt808_sms.h"
#include "jt808_gps.h"
#include "vdr.h"

#pragma diag_error 223

#define BYTESWAP2( val )    \
    ( ( ( val & 0xff ) << 8 ) |   \
      ( ( val & 0xff00 ) >> 8 ) )

#define BYTESWAP4( val )    \
    ( ( ( val & 0xff ) << 24 ) |   \
      ( ( val & 0xff00 ) << 8 ) |  \
      ( ( val & 0xff0000 ) >> 8 ) |  \
      ( ( val & 0xff000000 ) >> 24 ) )

typedef struct
{
	uint16_t id;
	int ( *func )( uint8_t linkno, uint8_t *pmsg );
}HANDLE_JT808_RX_MSG;

/*gprs收到信息的邮箱*/
static struct rt_mailbox	mb_gprsrx;
#define MB_GPRSDATA_POOL_SIZE 32
static uint8_t				mb_gprsrx_pool[MB_GPRSDATA_POOL_SIZE];

uint8_t						mobile[6];


/*
   AT命令发送使用的mailbox
   供 VOICE TTS SMS TTS使用
 */
#define MB_TTS_POOL_SIZE 32
static struct rt_mailbox	mb_tts;
static uint8_t				mb_tts_pool[MB_TTS_POOL_SIZE];

#define MB_AT_TX_POOL_SIZE 32
static struct rt_mailbox	mb_at_tx;
static uint8_t				mb_at_tx_pool[MB_AT_TX_POOL_SIZE];

#define MB_VOICE_POOL_SIZE 32
static struct rt_mailbox	mb_voice;
static uint8_t				mb_voice_pool[MB_VOICE_POOL_SIZE];

#define MB_SMS_POOL_SIZE 32
static struct rt_mailbox	mb_sms;
static uint8_t				mb_sms_pool[MB_SMS_POOL_SIZE];

static uint16_t				tx_seq = 0;             /*发送序号*/

static uint16_t				total_send_error = 0;   /*总的发送出错计数如果达到一定的次数要重启M66*/

/*发送信息列表*/
MsgList* list_jt808_tx;

/*接收信息列表*/
MsgList				* list_jt808_rx;

static rt_device_t	dev_gsm;

typedef enum
{
	CONNECT_NONE	= 0,            /*不连接*/
	CONNECT_IDLE	= 1,            /*空闲，准备连接*/
	CONNECT_PEER,                   /*正在连接到对端*/
	CONNECTED,                      /*连接成功*/
	CONNECT_ERROR,                  /*连接错误*/
	CONNECT_CLOSE,					/*连接关闭，区分是主动还是被动*/
}CONN_STATE;

struct
{
	uint32_t	disable_connect;    /*禁止链接标志，协议控制 0:允许链接*/
	CONN_STATE	server_state;
	uint8_t		server_index;
	CONN_STATE	auth_state;
	uint8_t		auth_index;
} connect_state =
{ 0, CONNECT_IDLE, 0, CONNECT_NONE, 0 };

#if 0

T_SOCKET_STATE	socket_jt808_state	= SOCKET_IDLE;
uint16_t		socket_jt808_index	= 0;

T_SOCKET_STATE	socket_iccard_state = SOCKET_IDLE;
uint16_t		socket_iccard_index = 0;


/*
   同时准备好可用的四个连接，根据要求选择处理,依次为
   实际中并不会同时对多个连接建立，只能依次分组来处理
   主808服务器
   备份808服务器
   主IC卡鉴权服务器
   备份IC卡鉴权服务器

 */
GSM_SOCKET gsm_socket[MAX_GSM_SOCKET];
#endif


/*
   jt808格式数据解码判断
   <标识0x7e><消息头><消息体><校验码><标识0x7e>

   返回有效的数据长度,为0 表明有错

 */
static uint16_t jt808_decode_fcs( uint8_t * pinfo, uint16_t length )
{
	uint8_t		* psrc, * pdst;
	uint16_t	count, len;
	uint8_t		fstuff	= 0;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          /*是否字节填充*/
	uint8_t		fcs		= 0;

	if( length < 5 )
	{
		return 0;
	}
	if( *pinfo != 0x7e )
	{
		return 0;
	}
	if( *( pinfo + length - 1 ) != 0x7e )
	{
		return 0;
	}
	psrc	= pinfo + 1;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    /*1byte标识后为正式信息*/
	pdst	= pinfo;
	count	= 0;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            /*转义后的长度*/
	len		= length - 2;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   /*去掉标识位的数据长度*/

	while( len )
	{
		if( fstuff )
		{
			*pdst	= *psrc + 0x7c;
			fstuff	= 0;
			count++;
			fcs ^= *pdst;
		} else
		{
			if( *psrc == 0x7d )
			{
				fstuff = 1;
			} else
			{
				*pdst	= *psrc;
				fcs		^= *pdst;
				count++;
			}
		}
		psrc++;
		pdst++;
		len--;
	}
	if( fcs != 0 )
	{
		rt_kprintf( "%s>fcs error\r\n", __func__ );
		return 0;
	}
	rt_kprintf( "count=%d\r\n", count );
	return count;
}

#if 0
/**添加一个字节**/
static uint16_t jt808_pack_byte( uint8_t * buf, uint8_t * fcs, uint8_t data )
{
	uint8_t * p = buf;
	*fcs ^= data;
	if( ( data == 0x7d ) || ( data == 0x7e ) )
	{
		*p++	= 0x7d;
		*p		= ( data - 0x7c );
		return 2;
	} else
	{
		*p = data;
		return 1;
	}
}

/*传递进长度，便于计算*/
static uint16_t jt808_pack_int( uint8_t * buf, uint8_t * fcs, uint32_t data, uint8_t width )
{
	uint16_t count = 0;
	switch( width )
	{
		case 1:
			count += jt808_pack_byte( buf + count, fcs, ( data & 0xff ) );
			break;
		case 2:
			count	+= jt808_pack_byte( buf + count, fcs, ( data >> 8 ) );
			count	+= jt808_pack_byte( buf + count, fcs, ( data & 0xff ) );
			break;
		case 4:
			count	+= jt808_pack_byte( buf + count, fcs, ( data >> 24 ) );
			count	+= jt808_pack_byte( buf + count, fcs, ( data >> 16 ) );
			count	+= jt808_pack_byte( buf + count, fcs, ( data >> 8 ) );
			count	+= jt808_pack_byte( buf + count, fcs, ( data & 0xff ) );
			break;
	}
	return count;
}

/*添加字符串***/
static uint16_t jt808_pack_string( uint8_t * buf, uint8_t * fcs, char * str )
{
	uint16_t	count	= 0;
	char		* p		= str;
	while( *p )
	{
		count += jt808_pack_byte( buf + count, fcs, *p++ );
	}
	return count;
}

/**添加数组**/
static uint16_t jt808_pack_array( uint8_t * buf, uint8_t * fcs, uint8_t * src, uint16_t len )
{
	uint16_t	count = 0;
	int			i;
	char		* p = src;
	for( i = 0; i < len; i++ )
	{
		count += jt808_pack_byte( buf + count, fcs, *p++ );
	}
	return count;
}

#endif


/*
   jt808终端发送信息
   并将相关信息注册到接收信息的处理线程中
   需要传递消息ID,和消息体，由jt808_send线程完成
    消息的填充
    发送和重发机制
    流水号
    已发信息的回收free
   传递进来的格式
   <msgid 2bytes><msg_len 2bytes><msgbody nbytes>

 */
static void jt808_send( void * parameter )
{
}

/*
发送后收到应答处理
*/
JT808_MSG_STATE jt808_tx_response( JT808_TX_NODEDATA * nodedata, uint8_t *pmsg )
{
	uint8_t		* msg = pmsg + 12;
	uint16_t	id;
	uint16_t	seq;
	uint8_t		res;

	seq = ( *msg << 8 ) | *( msg + 1 );
	id	= ( *( msg + 2 ) << 8 ) | *( msg + 3 );
	res = *( msg + 4 );

	rt_kprintf( "%d>CENT_ACK id=%04x seq=%04x res=%d\r\n", rt_tick_get( ), id, seq, res );
	switch( id )        // 判断对应终端消息的ID做区分处理
	{
		case 0x0200:    //	对应位置消息的应答
			rt_kprintf( "\r\nCentre ACK!\r\n" );
			break;
		case 0x0002:    //	心跳包的应答
			rt_kprintf( "\r\nCentre  Heart ACK!\r\n" );
			break;
		case 0x0101:    //	终端注销应答
			break;
		case 0x0102:    //	终端鉴权
			rt_kprintf( "\r\nCentre Auth ACK!\r\n" );
			break;
		case 0x0800:    // 多媒体事件信息上传
			break;
		case 0x0702:
			rt_kprintf( "\r\n  驾驶员信息上报---中心应答!  \r\n" );
			break;
		case 0x0701:
			rt_kprintf( "\r\n	电子运单上报---中心应答!  \r\n");
			break;
		default:
			rt_kprintf( "\r\nunknown id=%04x\r\n", id );
			break;
	}
	return ACK_OK;
}

/*
   消息发送超时
 */
static JT808_MSG_STATE jt808_tx_timeout( JT808_TX_NODEDATA * nodedata )
{
	rt_kprintf( "tx timeout\r\n" );
	return ACK_OK;
}

/*
   添加一个信息到发送列表中
   uint8_t linkno, 发送使用的socket号  1:808 2:iccar 3:user_define
   JT808_MSG_TYPE type, 类型 终端上报还是应答
   uint16_t id, 消息头中的消息ID
   uint16_t attr, 消息头中的消息体属性
   int32_t seq,  消息头中的流水号 -1:使用内部发送递增的  0-0XFFFF:指定流水号
   uint8_t *pinfo, 消息体
   对于多包发送的消息头中的消息包封装项字段，放到消息体中
 */
rt_err_t jt808_add_tx_data_gzy( uint8_t linkno,
                            JT808_MSG_TYPE type,
                            uint16_t id,
                            uint16_t attr,
                            int32_t seq,
                            JT808_MSG_STATE ( *cb_tx_timeout )( ),
                            JT808_MSG_STATE ( *cb_tx_response )( ),
                            uint8_t *pinfo )
{
	uint8_t				* pdata;
	JT808_TX_NODEDATA	* pnodedata;
	uint16_t			len;

	len = attr & 0xFFF;
	/**/

	pnodedata = rt_malloc( sizeof( JT808_TX_NODEDATA ) + sizeof( JT808_MSG_HEAD ) + len );
	if( pnodedata == RT_NULL )
	{
		return -RT_ERROR;
	}
	pnodedata->linkno	= linkno;
	pnodedata->type		= type;
	pnodedata->state	= IDLE;
	pnodedata->retry	= 0;
	if( attr & 0x2000 )
	{
		pnodedata->multipacket = 1;
	} else
	{
		pnodedata->multipacket = 0;
	}

	if( cb_tx_timeout == RT_NULL )
	{
		pnodedata->cb_tx_timeout = jt808_tx_timeout;
	} else
	{
		pnodedata->cb_tx_timeout = cb_tx_timeout;
	}
	if( cb_tx_response == RT_NULL )
	{
		pnodedata->cb_tx_response = jt808_tx_response;
	} else
	{
		pnodedata->cb_tx_response = cb_tx_timeout;
	}
/*在此可以存储在上报*/

/*在这添加上报的记录头*/
	pnodedata->msg_len = len + sizeof( JT808_MSG_HEAD );
	if( seq == -1 )
	{
		pnodedata->head_sn = tx_seq;
		tx_seq++;
	} else
	{
		pnodedata->head_sn = seq;
	}

	pnodedata->head_id = id;

	pdata		= pnodedata->tag_data;
	
	*pdata++	= id >> 8;
	*pdata++	= id & 0xff;
	*pdata++	= attr >> 8;
	*pdata++	= attr & 0xff;
	memcpy( pdata, mobile, 6 );
	pdata		+= 6;
	*pdata++	= pnodedata->head_sn >> 8;
	*pdata++	= pnodedata->head_sn & 0xff;
	memcpy( pdata, pinfo, len );    /*填充用户数据*/
	msglist_append( list_jt808_tx, pnodedata );

	return pnodedata->head_sn;      /*返回发送包序号*/
}

/*
   添加一个信息到发送列表中
   uint8_t linkno, 发送使用的socket号  1:808 2:iccar 3:user_define
   JT808_MSG_TYPE type, 类型 终端上报还是应答
   uint16_t id, 消息头中的消息ID
   uint16_t attr, 消息头中的消息体属性
   int32_t seq,  消息头中的流水号 -1:使用内部发送递增的  0-0XFFFF:指定流水号
   ( *cb_tx_timeout )( ),	用户超时回调函数
   ( *cb_tx_response )( ),	用户回调处理函数
   uint8_t *pinfo, 消息体
   *userpara 				用户参数
   对于多包发送的消息头中的消息包封装项字段，放到消息体中
 */
rt_err_t jt808_add_tx_data( uint8_t linkno,
                            JT808_MSG_TYPE type,
                            uint16_t id,
                            uint16_t len,
                            int32_t seq,
                            JT808_MSG_STATE ( *cb_tx_timeout )( ),
                            JT808_MSG_STATE ( *cb_tx_response )( ),
                            uint8_t *pinfo,
                            void  *userpara)
{
	uint8_t				* pdata;
	JT808_TX_NODEDATA	* pnodedata;
	//uint16_t			len;

	//len = attr & 0xFFF;
	/**/
	
	pnodedata = rt_malloc( sizeof( JT808_TX_NODEDATA )+sizeof(JT808_MSG_HEAD) + len + 4);
	if( pnodedata == RT_NULL )
	{
		return -RT_ERROR;
	}
	memset(pnodedata,0,sizeof(JT808_TX_NODEDATA));
	pnodedata->multipacket = 0;
	pnodedata->linkno	= linkno;
	pnodedata->type		= type;
	pnodedata->state	= IDLE;
	pnodedata->retry	= 0;
	pnodedata->packet_num = 1;
	pnodedata->packet_no  = 0;
	pnodedata->size		= len;
	pnodedata->msg_len	= len + 12;
	pnodedata->user_para = userpara;
	pnodedata->head_id 	= id;


	if( cb_tx_timeout == RT_NULL )
	{
		pnodedata->cb_tx_timeout = jt808_tx_timeout;
	} else
	{
		pnodedata->cb_tx_timeout = cb_tx_timeout;
	}
	if( cb_tx_response == RT_NULL )
	{
		pnodedata->cb_tx_response = jt808_tx_response;
	} else
	{
		pnodedata->cb_tx_response = cb_tx_response;
	}
/*在此可以存储在上报*/

/*在这添加上报的记录头*/
	pnodedata->msg_len = len+sizeof(JT808_MSG_HEAD);
	if( seq == -1 )
	{
		pnodedata->head_sn = tx_seq;
		tx_seq++;
	} else
	{
		pnodedata->head_sn = seq;
	}
	
	pnodedata->tag_data = (uint8_t *)pnodedata + sizeof(JT808_TX_NODEDATA);
	
	pdata		= pnodedata->tag_data;
	*pdata++	= id >> 8;
	*pdata++	= id & 0xff;
	*pdata++	= len >> 8;
	*pdata++	= len & 0xff;
	memcpy( pdata, mobile, 6 );
	pdata		+= 6;
	*pdata++	= pnodedata->head_sn >> 8;
	*pdata++	= pnodedata->head_sn & 0xff;
	memcpy( pdata, pinfo, len );  /*填充用户数据*/
	msglist_append( list_jt808_tx, pnodedata );

	return RT_EOK;
}


/*
   添加一个信息到发送列表中
   uint8_t linkno, 发送使用的socket号  1:808 2:iccar 3:user_define
   uint16_t id, 消息头中的消息ID
   int32_t seq,  消息头中的流水号 -1:使用内部发送递增的  0-0XFFFF:指定流水号
   ( *cb_tx_timeout )( ),	用户超时回调函数
   ( *cb_tx_response )( ),	用户回调处理函数
   *userpara 				用户参数
   对于多包发送的消息头中的消息包封装项字段，放到消息体中
 */
rt_err_t jt808_add_mult_tx_node( uint8_t linkno,
                            JT808_MSG_TYPE type,
                            uint16_t id,
                            uint32_t size,
                            int32_t seq,
                            JT808_MSG_STATE ( *cb_tx_timeout )( ),
                            JT808_MSG_STATE ( *cb_tx_response )( ),
                            void  *userpara )
{
	uint8_t				* pdata;
	JT808_TX_NODEDATA	* pnodedata;
	
	pnodedata = rt_malloc( sizeof( JT808_TX_NODEDATA ));
	if( pnodedata == RT_NULL )
	{
		rt_free(userpara);
		return -RT_ERROR;
	}

	pnodedata->multipacket = 1;
	pnodedata->linkno	= linkno;
	pnodedata->type		= type;
	pnodedata->state	= IDLE;
	pnodedata->retry	= 0;
	pnodedata->packet_num = (size/JT808_PACKAGE_MAX);
	if(size%JT808_PACKAGE_MAX)
		pnodedata->packet_num++;
	pnodedata->packet_no  = 0;
	pnodedata->size		= size;
	pnodedata->msg_len  = 0;
	pnodedata->user_para = userpara;
	pnodedata->head_id  = id;


/*在这添加上报的记录头*/
	if( seq == -1 )
	{
		pnodedata->head_sn = tx_seq;
		tx_seq++;
	} else
	{
		pnodedata->head_sn = seq;
	}
	

	if( cb_tx_timeout == RT_NULL )
	{
		pnodedata->cb_tx_timeout = jt808_tx_timeout;
	} else
	{
		pnodedata->cb_tx_timeout = cb_tx_timeout;
	}
	if( cb_tx_response == RT_NULL )
	{
		pnodedata->cb_tx_response = jt808_tx_response;
	} else
	{
		pnodedata->cb_tx_response = cb_tx_response;
	}
	
	msglist_append( list_jt808_tx, pnodedata );
	return RT_EOK;
}

/*
   添加一个信息到发送列表中
   uint8_t linkno, 发送使用的socket号  1:808 2:iccar 3:user_define
   JT808_MSG_TYPE type, 类型 终端上报还是应答
   uint16_t id, 消息头中的消息ID
   uint16_t attr, 消息头中的消息体属性
   int32_t seq,  消息头中的流水号 -1:使用内部发送递增的  0-0XFFFF:指定流水号
   ( *cb_tx_timeout )( ),	用户超时回调函数
   ( *cb_tx_response )( ),	用户回调处理函数
   uint8_t *pinfo, 消息体
   对于多包发送的消息头中的消息包封装项字段，放到消息体中
 */
uint16_t jt808_add_mult_tx_head(JT808_TX_NODEDATA *pnode)
{
	uint8_t				* pdata;
	uint16_t 			seq;
	uint16_t 			len;
	
	pdata = pnode->tag_data;
	len	= pnode->msg_len;
	
	*pdata++	= pnode->head_id >> 8;
	*pdata++	= pnode->head_id & 0xff;
	
	len |= 0x2000;
	*pdata++	= len >> 8 ;
	*pdata++	= len & 0xff;
	
	memcpy( pdata, mobile, 6 );
	pdata		+= 6;
	*pdata++	= pnode->head_sn >> 8;
	*pdata++	= pnode->head_sn & 0xff;
	*pdata++	= pnode->packet_num >> 8;
	*pdata++	= pnode->packet_num  & 0xff;
	*pdata++	= pnode->packet_no >> 8;
	*pdata++	= pnode->packet_no  & 0xff;
	return 16;
}


/*
   终端通用应答
 */
static rt_err_t jt808_tx_0x0001( uint8_t linkno, uint16_t seq, uint16_t id, uint8_t res )
{
	uint8_t				* pdata;
	JT808_TX_NODEDATA	* pnodedata;
	uint8_t				buf [256];
	uint8_t				* p;
	uint16_t			len;

	pnodedata = rt_malloc( sizeof( JT808_TX_NODEDATA ) );
	if( pnodedata == NULL )
	{
		return -RT_ERROR;
	}
	pnodedata->type				= TERMINAL_ACK;
	pnodedata->state			= IDLE;
	pnodedata->retry			= 0;
	pnodedata->cb_tx_timeout	= jt808_tx_timeout;
	pnodedata->cb_tx_response	= jt808_tx_response;

	pdata = rt_malloc( 5 + 12 );
	if( pdata == NULL )
	{
		rt_free( pnodedata );
		return -RT_ERROR;
	}

//	jt808_add_msg_head(pdata,0x0001,0x0005);
	p		= pdata + 12;
	*p++	= ( seq >> 8 );
	*p++	= ( seq & 0xff );
	*p++	= ( id >> 8 );
	*p++	= ( id & 0xff );
	*p		= res;

	pnodedata->msg_len = len;
	msglist_append( list_jt808_tx, pnodedata );
	tx_seq++;
}

/*
   平台通用应答,收到信息，停止发送
 */
static int handle_rx_0x8001( uint8_t linkno, uint8_t *pmsg )
{
	MsgListNode			* iter;
	JT808_TX_NODEDATA	* iterdata;
	JT808_MSG_STATE		res_ret;

	uint16_t			id;
	uint16_t			seq;
	uint8_t				res;
	
/*跳过消息头12byte*/
	seq = ( *( pmsg + 12 ) << 8 ) | *( pmsg + 13 );
	id	= ( *( pmsg + 14 ) << 8 ) | *( pmsg + 15 );
	res = *( pmsg + 16 );

	/*单条处理*/
	iter		= list_jt808_tx->first;
	iterdata	= (JT808_TX_NODEDATA*)iter->data;

	if( ( iterdata->head_id == id ) && ( iterdata->head_sn == seq ) )
	{
		if(iterdata->cb_tx_response!=RT_NULL)
		{
			iterdata->state = iterdata->cb_tx_response(iterdata,pmsg);
			iterdata->retry = 0;
		}
		else
		{
			iterdata->state = ACK_OK;
		}
	}
#if 0
	if(iterdata->multipacket==0)
	{
		if( ( iterdata->head_id == id ) && ( iterdata->head_sn == seq ) )
		{
			iterdata->cb_tx_response(iterdata, linkno, pmsg ); /*应答处理函数*/
			iterdata->state = ACK_OK;
		}
	}
#endif
}
int handle_8001( u8 tempstate)
{
#if 0
	MsgListNode 		* iter;
	JT808_TX_NODEDATA	* iterdata;
	JT808_MSG_STATE 	res_ret;

	/*单条处理*/
	iter		= list_jt808_tx->first;
	iterdata	= (JT808_TX_NODEDATA*)iter->data;
	if(tempstate)
		{
		iterdata->state = IDLE;
		iterdata->retry = 0;
		}
#endif
	MsgListNode			* iter;
	JT808_TX_NODEDATA	* iterdata;
	JT808_MSG_STATE		res_ret;
	uint8_t 			pmsg[128];
	uint16_t			body_len=0; /*消息体长度*/

	uint16_t			id;
	uint16_t			seq;
	uint8_t				res;
	
	/*单条处理*/
	iter		= list_jt808_tx->first;
	iterdata	= (JT808_TX_NODEDATA*)iter->data;

	///模拟数据
	pmsg[body_len++]	= 0x80;
	pmsg[body_len++]	= 0x01;
	pmsg[body_len++]	= 0x00;
	pmsg[body_len++]	= 0x05;
	body_len			= 12;
	pmsg[body_len++]	= iterdata->head_sn >>8;
	pmsg[body_len++]	= iterdata->head_sn & 0xFF;
	pmsg[body_len++]	= iterdata->head_id >>8;
	pmsg[body_len++]	= iterdata->head_id & 0xFF;
	pmsg[body_len++]	= tempstate;		///0 为成功，否则失败
/*跳过消息头12byte*/
	seq = ( *( pmsg + 12 ) << 8 ) | *( pmsg + 13 );
	id	= ( *( pmsg + 14 ) << 8 ) | *( pmsg + 15 );
	res = *( pmsg + 16 );


	if( ( iterdata->head_id == id ) && ( iterdata->head_sn == seq ) )
	{
	rt_kprintf("\r\n handle_8001_1");
		if(iterdata->cb_tx_response!=RT_NULL)
		{
			rt_kprintf("\r\n handle_8001_2");
			iterdata->state = iterdata->cb_tx_response(iterdata,pmsg);
			iterdata->retry = 0;
		}
		else
		{
			iterdata->state = ACK_OK;
		}
	}

}

FINSH_FUNCTION_EXPORT( handle_8001, handle_8001 );

/*补传分包请求*/
static int handle_rx_0x8003( uint8_t linkno, uint8_t *pmsg )
{
}

/* 监控中心对终端注册消息的应答*/
static int handle_rx_0x8100( uint8_t linkno, uint8_t *pmsg )
{
	MsgListNode			* iter;
	JT808_TX_NODEDATA	* iterdata;

	uint16_t			body_len; /*消息体长度*/
	uint16_t			ack_seq;
	uint8_t				res;
	uint8_t				* msg;

	body_len	= ( ( *( pmsg + 2 ) << 8 ) | ( *( pmsg + 3 ) ) ) & 0x3FF;
	msg			= pmsg + 12;

	ack_seq = ( *msg << 8 ) | *( msg + 1 );
	res		= *( msg + 2 );

	iter		= list_jt808_tx->first;
	iterdata	= iter->data;
	if( ( iterdata->head_id == 0x0100 ) && ( iterdata->head_sn == ack_seq ) )
	{
		if( res == 0 )
		{
			strncpy( term_param.register_code, msg + 3, body_len - 3 );
			iterdata->state = ACK_OK;
		}
	}
	return 1;
}

/*设置终端参数*/
static int handle_rx_0x8103( uint8_t linkno, uint8_t *pmsg )
{
	uint8_t		* p;
	uint8_t		res = 0;

	uint16_t	msg_len, count = 0;
	uint32_t	param_id;
	uint8_t		param_len;

	uint16_t	seq, id;

	if( *( pmsg + 2 ) >= 0x20 ) /*如果是多包的设置参数*/
	{
		rt_kprintf( "\r\n>%s multi packet no support!", __func__ );
		return 1;
	}

	id	= ( pmsg[0] << 8 ) | pmsg[1];
	seq = ( pmsg[10] << 8 ) | pmsg[11];

	msg_len = ( ( pmsg[2] << 8 ) | pmsg[3] ) & 0x3FF - 1;
	p		= pmsg + 13;

	/*使用数据长度,判断数据是否结束，没有使用参数总数*/
	while( count < msg_len )
	{
		param_id	= ( ( *p++ ) << 24 ) | ( ( *p++ ) << 16 ) | ( ( *p++ ) << 8 ) | ( *p++ );
		param_len	= *p++;
		count		+= ( 5 + param_len );
		res			|= param_put( param_id, param_len, p );
		if( res )
		{
			rt_kprintf( "\r\n%s>res=%d\r\n", __func__, __LINE__ );
			break;
		}
	}
	/*返回通用应答*/
	jt808_tx_0x0001( linkno, seq, id, res );
	return 1;
}

/*查询全部终端参数，有可能会超出单包最大字节*/
static int handle_rx_0x8104( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/*终端控制*/
static int handle_rx_0x8105( uint8_t linkno, uint8_t *pmsg )
{
	uint8_t cmd;
	uint8_t * cmd_arg;

	cmd = *( pmsg + 12 );
	switch( cmd )
	{
		case 1: /*无线升级*/
			break;
		case 2: /*终端控制链接指定服务器*/
			break;
		case 3: /*终端关机*/
			break;
		case 4: /*终端复位*/
			break;
		case 5: /*恢复出厂设置*/
			break;
		case 6: /*关闭数据通讯*/
			break;
		case 7: /*关闭所有无线通讯*/
			break;
	}
	return 1;
}

/*查询指定终端参数,返回应答0x0104*/
static int handle_rx_0x8106( uint8_t linkno, uint8_t *pmsg )
{
	int			i;
	uint8_t		*p;
	uint8_t		fcs = 0;
	uint8_t		value[8];
	uint32_t	id;
	uint16_t	len;
	uint16_t	pos;
	uint16_t	info_len	= 0;
	uint16_t	head_len	= 0;
	uint8_t		param_count, return_param_count;

	uint8_t		buf[1500];
#if 0
	pos					= 100;              /*先空出100byte*/
	param_count			= *( pmsg + 12 );   /*总的参数个数*/
	return_param_count	= 0;
	p					= pmsg + 13;
	/*填充要返回消息的数据，并记录长度*/
	for( i = 0; i < param_count; i++ )      /*如果有未知的id怎么办，忽略,这样参数个数就改变了*/
	{
		id	= *p++;
		id	|= ( *p++ ) << 8;
		id	|= ( *p++ ) << 16;
		id	|= ( *p++ ) << 24;
		len = param_get( id, value );       /*得到参数的长度，未转义*/
		if( len )
		{
			return_param_count++;           /*找到有效的id*/
			pos += jt808_pack_int( buf + pos, &fcs, id, 2 );
			pos + jt808_pack_int( buf + pos, &fcs, len, 1 );
			pos			+= jt808_pack_array( buf + pos, &fcs, value, len );
			info_len	+= ( len + 3 );     /*id+长度+数据*/
		}
	}

	head_len	= 1;                        /*空出开始的0x7e*/
	head_len	+= jt808_pack_int( buf + head_len, &fcs, 0x0104, 2 );
	head_len	+= jt808_pack_int( buf + head_len, &fcs, info_len + 3, 2 );
	head_len	+= jt808_pack_array( buf + head_len, &fcs, pmsg + 4, 6 );
	head_len	+= jt808_pack_int( buf + head_len, &fcs, tx_seq, 2 );

	head_len	+= jt808_pack_array( buf + head_len, &fcs, pmsg + 10, 2 );
	head_len	+= jt808_pack_int( buf + head_len, &fcs, return_param_count, 1 );

	memcpy( buf + head_len, buf + 100, pos - 100 ); /*拼接数据*/
	len = head_len + pos - 100;                     /*当前数据0x7e,<head><msg>*/

	len			+= jt808_pack_byte( buf + len, &fcs, fcs );
	buf [0]		= 0x7e;
	buf [len]	= 0x7e;

	jt808_add_tx_data( linkno, TERMINAL_ACK, 0x0104, buf, len + 1 );
#endif

	return 1;
}

/*查询终端属性,应答 0x0107*/
static int handle_rx_0x8107( uint8_t linkno, uint8_t *pmsg )
{
	uint8_t		buf[100];
	uint8_t		fcs			= 0;
	uint16_t	len			= 1;
	uint16_t	info_len	= 0;
	uint16_t	head_len	= 1;


/*
   len += jt808_pack_int( buf + len, &fcs, 0x0107, 2 );
   len += jt808_pack_int( buf + len, &fcs, 0x0107, 2 );
   len += jt808_pack_int( buf + len, &fcs, 0x0107, 2 );

   jt808_add_tx_data( linkno, TERMINAL_ACK, 0x0107, buf, len + 1 );
 */
	return 1;
}

/**/
static int handle_rx_0x8201( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8202( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8300( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8301( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8302( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8303( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8304( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8400( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8401( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8500( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8600( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8601( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8602( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8603( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8604( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8605( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8606( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8607( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/*行驶记录仪数据采集*/
static int handle_rx_0x8700( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/*行驶记录仪参数下传*/
static int handle_rx_0x8701( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8800( uint8_t linkno, uint8_t *pmsg )
{
	MsgListNode				* iter;
	JT808_TX_NODEDATA		* iterdata;

	uint16_t				body_len; /*消息体长度*/
	uint16_t				ack_seq;
	uint8_t					res;
	uint8_t					* msg;

	body_len	= ( ( *( pmsg + 2 ) << 8 ) | ( *( pmsg + 3 ) ) ) & 0x3FF;
	msg			= pmsg + 12;

	ack_seq = ( *msg << 8 ) | *( msg + 1 );
	res		= *( msg + 2 );

	iter		= list_jt808_tx->first;
	iterdata	= iter->data;
	if(( iterdata->head_id == 0x0801 )&&(iterdata->multipacket))	///判断是否调用回调函数
	{
		if(iterdata->cb_tx_response!=RT_NULL)
		{
			iterdata->state = iterdata->cb_tx_response(iterdata,pmsg);
			iterdata->retry	= 0;
		}
		else
		{
			iterdata->state = ACK_OK;
		}
	}
	return 1;
	return 1;
}

/**/
static int handle_8800( u32 media_id ,char* pmsg)
{
	MsgListNode				* iter;
	JT808_TX_NODEDATA		* iterdata;
	uint8_t 				tempbuf[128];

	uint16_t				body_len; /*消息体长度*/
	uint16_t				ack_seq;
	uint8_t					res;
	uint8_t					* msg;
	uint16_t				i,j;
	
	rt_kprintf("\r\n handle_8800_1");
	memset(tempbuf,0,sizeof(tempbuf));
	body_len=0;
	tempbuf[body_len++]	= 0x88;
	tempbuf[body_len++]	= 0x00;
	tempbuf[3]			= 4;
	body_len=12;
	tempbuf[body_len++]	= media_id>>24;
	tempbuf[body_len++]	= media_id>>16;
	tempbuf[body_len++]	= media_id>>8;
	tempbuf[body_len++]	= media_id;
	if(strlen(pmsg))
		{
		j=tempbuf[body_len++]=strlen(pmsg);
		tempbuf[3]++;
		for(i=0;i<j;i++)
			{
			tempbuf[body_len++] = 0;
			tempbuf[body_len++] = *pmsg-'0';
			pmsg++;
			tempbuf[3]+=2;
			}
		}
	iter		= list_jt808_tx->first;
	iterdata	= iter->data;
	if(( iterdata->head_id == 0x0801 )&&(iterdata->multipacket))	///判断是否调用回调函数
	{
		if(iterdata->cb_tx_response!=RT_NULL)
			{
			iterdata->state = iterdata->cb_tx_response(iterdata,tempbuf);
			iterdata->retry	= 0;
			rt_kprintf("\r\n handle_8800_2");
			}
	}
	return 1;
}
FINSH_FUNCTION_EXPORT( handle_8800, handle_8800 );

/*摄像头立即拍摄命令*/
static int handle_rx_0x8801( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/*
   多媒体信息检索
 */
static int handle_rx_0x8802( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8803( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8804( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8805( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8900( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8A00( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
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
static int handle_rx_default( uint8_t linkno, uint8_t *pmsg )
{
	rt_kprintf( "\r\nunknown!\r\n" );
	return 1;
}

#define DECL_JT808_RX_HANDLE( a )	{ a, handle_rx_ ## a }
#define DECL_JT808_TX_HANDLE( a )	{ a, handle_jt808_tx_ ## a }

HANDLE_JT808_RX_MSG handle_rx_msg[] =
{
	DECL_JT808_RX_HANDLE( 0x8001 ), //	通用应答
	DECL_JT808_RX_HANDLE( 0x8003 ), //	补传分包请求
	DECL_JT808_RX_HANDLE( 0x8100 ), //  监控中心对终端注册消息的应答
	DECL_JT808_RX_HANDLE( 0x8103 ), //	设置终端参数
	DECL_JT808_RX_HANDLE( 0x8104 ), //	查询终端参数
	DECL_JT808_RX_HANDLE( 0x8105 ), // 终端控制
	DECL_JT808_RX_HANDLE( 0x8106 ), // 查询指定终端参数
	DECL_JT808_RX_HANDLE( 0x8201 ), // 位置信息查询    位置信息查询消息体为空
	DECL_JT808_RX_HANDLE( 0x8202 ), // 临时位置跟踪控制
	DECL_JT808_RX_HANDLE( 0x8300 ), //	文本信息下发
	DECL_JT808_RX_HANDLE( 0x8301 ), //	事件设置
	DECL_JT808_RX_HANDLE( 0x8302 ), // 提问下发
	DECL_JT808_RX_HANDLE( 0x8303 ), //	信息点播菜单设置
	DECL_JT808_RX_HANDLE( 0x8304 ), //	信息服务
	DECL_JT808_RX_HANDLE( 0x8400 ), //	电话回拨
	DECL_JT808_RX_HANDLE( 0x8401 ), //	设置电话本
	DECL_JT808_RX_HANDLE( 0x8500 ), //	车辆控制
	DECL_JT808_RX_HANDLE( 0x8600 ), //	设置圆形区域
	DECL_JT808_RX_HANDLE( 0x8601 ), //	删除圆形区域
	DECL_JT808_RX_HANDLE( 0x8602 ), //	设置矩形区域
	DECL_JT808_RX_HANDLE( 0x8603 ), //	删除矩形区域
	DECL_JT808_RX_HANDLE( 0x8604 ), //	多边形区域
	DECL_JT808_RX_HANDLE( 0x8605 ), //	删除多边区域
	DECL_JT808_RX_HANDLE( 0x8606 ), //	设置路线
	DECL_JT808_RX_HANDLE( 0x8607 ), //	删除路线
	DECL_JT808_RX_HANDLE( 0x8700 ), //	行车记录仪数据采集命令
	DECL_JT808_RX_HANDLE( 0x8701 ), //	行驶记录仪参数下传命令
	DECL_JT808_RX_HANDLE( 0x8800 ), //	多媒体数据上传应答
	DECL_JT808_RX_HANDLE( 0x8801 ), //	摄像头立即拍照
	DECL_JT808_RX_HANDLE( 0x8802 ), //	存储多媒体数据检索
	DECL_JT808_RX_HANDLE( 0x8803 ), //	存储多媒体数据上传命令
	DECL_JT808_RX_HANDLE( 0x8804 ), //	录音开始命令
	DECL_JT808_RX_HANDLE( 0x8805 ), //	单条存储多媒体数据检索上传命令 ---- 补充协议要求
	DECL_JT808_RX_HANDLE( 0x8900 ), //	数据下行透传
	DECL_JT808_RX_HANDLE( 0x8A00 ), //	平台RSA公钥
};

/*jt808的socket管理

   维护链路。会有不同的原因
   上报状态的维护
   1.尚未登网
   2.中心连接，DNS,超时不应答
   3.禁止上报，关闭模块的区域
   4.当前正在进行空中更新，多媒体上报等不需要打断的工作

 */
/*
   这个回调主要是M66的链路关断时的通知，socket不是一个完整的线程
 */
void cb_socket_close( uint8_t cid)
{
	if( cid == 1 )
	{
		connect_state.server_state = CONNECT_CLOSE;
	}

	if( cid == 2 )
	{
		connect_state.auth_state = CONNECT_CLOSE;
	}
}

/*
   接收处理
   分析jt808格式的数据
   <linkno><长度2byte><标识0x7e><消息头><消息体><校验码><标识0x7e>

 */
uint16_t jt808_rx_proc( uint8_t * pinfo )
{
	uint8_t		* psrc;
	uint16_t	len;
	uint8_t		linkno;
	uint16_t	i, id;
	uint8_t		flag_find	= 0;
	uint8_t		fcs			= 0;
	uint16_t	ret;

	linkno	= pinfo [0];
	len		= ( pinfo [1] << 8 ) | pinfo [2];

/*去转义，还是直接在pinfo上操作*/
	len = jt808_decode_fcs( pinfo + 3, len );
	if( len == 0 )
	{
		rt_kprintf( ">jt808_decode_fcs error\r\n" );
		return 1;
	}
/*显示解码后的信息*/
	rt_kprintf( "\r\n>dump start(%d)\r\nhead>", len );
	psrc = pinfo + 3; /*跳过前面的len和0x7e*/
	rt_kprintf( "%02x%02x ", *( psrc + 0 ), *( psrc + 1 ) );
	rt_kprintf( "%02x%02x ", *( psrc + 2 ), *( psrc + 3 ) );
	for( i = 0; i < 6; i++ )
	{
		rt_kprintf( "%02x", *( psrc + 4 + i ) );
	}
	rt_kprintf( " %02x%02x ", *( psrc + 10 ), *( psrc + 11 ) );
	rt_kprintf( "\r\nbody>" );

	psrc = pinfo + 15;
	for( i = 0; i < ( len - 12 ); i++ )
	{
		if( i % 16 == 0 )
		{
			rt_kprintf( "\r\n" );
		}
		rt_kprintf( "%02x ", *psrc++ );
	}
	rt_kprintf( "\r\n>dump end\r\n" );


/*直接处理收到的信息，根据ID分发，直接分发消息*/

	psrc	= pinfo + 3;
	id		= ( *psrc << 8 ) | *( psrc + 1 );

	for( i = 0; i < sizeof( handle_rx_msg ) / sizeof( HANDLE_JT808_RX_MSG ); i++ )
	{
		if( id == handle_rx_msg [i].id )
		{
			handle_rx_msg [i].func( linkno, psrc );
			flag_find = 1;
		}
	}
	if( !flag_find )
	{
		handle_rx_default( linkno, psrc );
	}
}

/*
   处理每个要发送信息的状态
   现在允许并行处理吗?

   2013.06.08增加多包发送处理
 */

static MsgListRet jt808_tx_proc( MsgListNode * node )
{
	MsgListNode			* pnode		= ( MsgListNode* )node;
	JT808_TX_NODEDATA	* pnodedata = ( JT808_TX_NODEDATA* )( pnode->data );
	int					i;
	rt_err_t			ret;
	JT808_MSG_STATE		res_ret;

	if( node == RT_NULL )
	{
		return MSGLIST_RET_OK;
	}

	if( pnodedata->state == IDLE )                      /*空闲，发送信息或超时后没有数据*/
	{
		if( pnodedata->retry >= jt808_param.id_0x0003 ) /*超过了最大重传次数*/                                                                     /*已经达到重试次数*/
		{
			/*表示发送失败*/
			pnodedata->cb_tx_timeout( pnodedata );      /*调用发送失败处理函数*/
			return MSGLIST_RET_DELETE_NODE;
		}
		/*要判断是不是出于GSM_TCPIP状态,当前socket是否可用*/
		if( gsmstate( GSM_STATE_GET ) != GSM_TCPIP )
		{
			return MSGLIST_RET_OK;
		}
		if( connect_state.server_state != CONNECTED )
		{
			return MSGLIST_RET_OK;
		}

		if( pnodedata->multipacket == 0 )   /*单包发送*/
		{
			/*在这添加消息头?,动态分配如何解决*/
			ret = socket_write( pnodedata->linkno, pnodedata->tag_data, pnodedata->msg_len );
			if( ret == RT_EOK )             /*发送成功等待中心应答中*/
			{
				pnodedata->tick = rt_tick_get( );
				pnodedata->retry++;
				pnodedata->timeout	= pnodedata->retry * jt808_param.id_0x0002 * RT_TICK_PER_SECOND;
				pnodedata->state	= WAIT_ACK;
				rt_kprintf( "send retry=%d,timeout=%d\r\n", pnodedata->retry, pnodedata->timeout * 10 );
			}else /*发送数据没有等到模块返回的OK，立刻重发，还是等一段时间再发*/
			{
				pnodedata->retry++; /*置位再次发送*/
				total_send_error++;
				rt_kprintf( "total_send_error=%d\r\n", total_send_error );
			}
		}
		else							   /*多包发送*/
		{
			res_ret = pnodedata->cb_tx_response(pnodedata,NULL);
			if(IDLE == res_ret)
			{
				//rt_kprintf("\r\n发送数据:\r\n");
				//printer_data_hex(pnodedata->tag_data,pnodedata->msg_len);
				//ret=RT_EOK;
				ret = socket_write( pnodedata->linkno, pnodedata->tag_data, pnodedata->msg_len );
				
				pnodedata->retry = 1;
				if( ret == RT_EOK )             /*发送成功等待中心应答中*/
				{
					pnodedata->tick 	= rt_tick_get( );
					pnodedata->timeout	= jt808_param.id_0x0002 * RT_TICK_PER_SECOND;
					pnodedata->state	= WAIT_ACK;
					rt_kprintf( "send retry=%d,timeout=%d\r\n", pnodedata->retry, pnodedata->timeout * 10 );
				}else /*发送数据没有等到模块返回的OK，立刻重发，还是等一段时间再发*/
				{
					total_send_error++;
					rt_kprintf( "total_send_error=%d\r\n", total_send_error );
				}
			}
			else if(WAIT_ACK == res_ret)	///等待专用应答超时
			{
				pnodedata->tick 	= rt_tick_get( );
				pnodedata->state	= WAIT_ACK;
				pnodedata->retry++;
				pnodedata->timeout	= jt808_param.id_0x0002 * RT_TICK_PER_SECOND;
			}
			else
			{
				return MSGLIST_RET_DELETE_NODE; 
			}
		}

		return MSGLIST_RET_OK;
	}

	if( pnodedata->state == WAIT_ACK ) /*检查中心应答是否超时*/
	{
		if( rt_tick_get( ) - pnodedata->tick > pnodedata->timeout )
		{	
			pnodedata->state = IDLE;
		}
		return MSGLIST_RET_OK;
	}

	if( pnodedata->state == ACK_OK )
	{
		return MSGLIST_RET_DELETE_NODE;
	}

	return MSGLIST_RET_OK;
}

/*
   处理每个要发送信息的状态
   现在允许并行处理吗?

   2013.06.08增加多包发送处理
 */

static MsgListRet jt808_tx_proc_test( MsgListNode * node )
{
	MsgListNode			* pnode		= ( MsgListNode* )node;
	JT808_TX_NODEDATA	* pnodedata = ( JT808_TX_NODEDATA* )( pnode->data );
	int					i;
	rt_err_t			ret;
	JT808_MSG_STATE		res_ret;

	jt808_param.id_0x0002	= 3;
	jt808_param.id_0x0003	= 3;

	if( node == RT_NULL )
	{
		return MSGLIST_RET_OK;
	}

	if( pnodedata->state == IDLE )                      /*空闲，发送信息或超时后没有数据*/
	{
		if( pnodedata->retry >= jt808_param.id_0x0003 ) /*超过了最大重传次数*/                                                                     /*已经达到重试次数*/
		{
			/*表示发送失败*/
			pnodedata->cb_tx_timeout( pnodedata );      /*调用发送失败处理函数*/
			return MSGLIST_RET_DELETE_NODE;
		}
		#if 0
		/*要判断是不是出于GSM_TCPIP状态,当前socket是否可用*/
		if( gsmstate( GSM_STATE_GET ) != GSM_TCPIP )
		{
			return MSGLIST_RET_OK;
		}
		if( connect_state.server_state != CONNECTED )
		{
			return MSGLIST_RET_OK;
		}
		#endif

		if( pnodedata->multipacket == 0 )   /*单包发送*/
		{
			/*在这添加消息头?,动态分配如何解决*/
			ret = socket_write( pnodedata->linkno, pnodedata->tag_data, pnodedata->msg_len );
			if( ret == RT_EOK )             /*发送成功等待中心应答中*/
			{
				pnodedata->tick = rt_tick_get( );
				pnodedata->retry++;
				pnodedata->timeout	= pnodedata->retry * jt808_param.id_0x0002 * RT_TICK_PER_SECOND;
				pnodedata->state	= WAIT_ACK;
				rt_kprintf( "send retry=%d,timeout=%d\r\n", pnodedata->retry, pnodedata->timeout * 10 );
			}else /*发送数据没有等到模块返回的OK，立刻重发，还是等一段时间再发*/
			{
				pnodedata->retry++; /*置位再次发送*/
				total_send_error++;
				rt_kprintf( "total_send_error=%d\r\n", total_send_error );
			}
		}
		else							   /*多包发送*/
		{
			res_ret = pnodedata->cb_tx_response(pnodedata,NULL);
			if(IDLE == res_ret)
			{
				//rt_kprintf("\r\n发送数据:\r\n");
				//printer_data_hex(pnodedata->tag_data,pnodedata->msg_len);
				//ret=RT_EOK;
				ret = socket_write( pnodedata->linkno, pnodedata->tag_data, pnodedata->msg_len );
				
				pnodedata->retry = 1;
				if( ret == RT_EOK )             /*发送成功等待中心应答中*/
				{
					pnodedata->tick 	= rt_tick_get( );
					pnodedata->timeout	= jt808_param.id_0x0002 * RT_TICK_PER_SECOND;
					pnodedata->state	= WAIT_ACK;
					rt_kprintf( "send retry=%d,timeout=%d\r\n", pnodedata->retry, pnodedata->timeout * 10 );
				}else /*发送数据没有等到模块返回的OK，立刻重发，还是等一段时间再发*/
				{
					total_send_error++;
					rt_kprintf( "total_send_error=%d\r\n", total_send_error );
				}
			}
			else if(WAIT_ACK == res_ret)	///等待专用应答超时
			{
				pnodedata->tick 	= rt_tick_get( );
				pnodedata->state	= WAIT_ACK;
				pnodedata->retry++;
				pnodedata->timeout	= jt808_param.id_0x0002 * RT_TICK_PER_SECOND;
			}
			else
			{
				return MSGLIST_RET_DELETE_NODE; 
			}
		}

		return MSGLIST_RET_OK;
	}

	if( pnodedata->state == WAIT_ACK ) /*检查中心应答是否超时*/
	{
		if( rt_tick_get( ) - pnodedata->tick > pnodedata->timeout )
		{	
			pnodedata->state = IDLE;
		}
		return MSGLIST_RET_OK;
	}

	if( pnodedata->state == ACK_OK )
	{
		return MSGLIST_RET_DELETE_NODE;
	}

	return MSGLIST_RET_OK;
}



/*808连接处理*/
static void jt808_socket_proc( void )
{
	T_GSM_STATE			state;
	static rt_tick_t	server_heartbeat_tick	= 0;
	static rt_tick_t	auth_heartbeat_tick		= 0;

/*检查是否允许gsm工作*/
	if( connect_state.disable_connect )
	{
		return;
	}

/*检查GSM状态*/
	state = gsmstate( GSM_STATE_GET );
	if( state == GSM_IDLE )
	{
		gsmstate( GSM_POWERON );                /*开机登网*/
		return;
	}
/*控制登网*/
	if( state == GSM_AT )                       /*这里要判断用那个apn user psw 登网*/
	{
		if( connect_state.server_index % 2 )    /*用备用服务器*/
		{
			ctl_gprs( jt808_param.id_0x0014, \
			          jt808_param.id_0x0015, \
			          jt808_param.id_0x0016, \
			          1 );
		}else /*用主服务器*/
		{
			ctl_gprs( jt808_param.id_0x0010, \
			          jt808_param.id_0x0011, \
			          jt808_param.id_0x0012, \
			          1 );
		}
		return;
	}
/*控制建立连接*/
	if( state == GSM_TCPIP )                                /*已经在线了*/
	{
		if( connect_state.server_state == CONNECT_IDLE )
		{
			if( connect_state.server_index % 2 )            /*连备用服务器*/
			{
				ctl_socket( 1, 't', jt808_param.id_0x0017, jt808_param.id_0x0018, 1 );
			}else /*连主服务器*/
			{
				ctl_socket( 1, 't', jt808_param.id_0x0013, jt808_param.id_0x0018, 1 );
			}
			connect_state.server_state = CONNECT_PEER;      /*此时gsm_state处于 GSM_SOCKET_PROC，连完后返回 GSM_TCPIP*/
			return;
		}

		if( connect_state.server_state == CONNECT_PEER )    /*正在连接到服务器*/
		{
			if( socketstate( SOCKET_STATE ) == SOCKET_READY )
			{
				connect_state.server_state = CONNECTED;
			}else /*没有连接成功,切换服务器*/
			{
				connect_state.server_index++;
				connect_state.server_state = CONNECT_IDLE;
			}
		}

		if( connect_state.server_state == CONNECTED ) /*链路维护心跳包*/
		{
			if( server_heartbeat_tick )
			{
				/*要发送心跳包*/
				if( ( rt_tick_get( ) - server_heartbeat_tick ) >= ( jt808_param.id_0x0001 * RT_TICK_PER_SECOND ) )
				{
				}
			}else
			{
				/*重新拨号，发送鉴权命令或注册命令*/
				rt_kprintf( "auth\r\n" );
				//jt808_tx(0x0102,"012345",6);  /*这个简化的指令好像不能执行，基本函数*/
				jt808_add_tx_data( 1, TERMINAL_CMD, 0x0102, 6, -1, RT_NULL, RT_NULL, "012345",RT_NULL );
				server_heartbeat_tick = rt_tick_get( ); /*首次用保留当前时刻*/
			}
			return;                                     /*直接返回，不连ICCARD*/
		}

		
		if( connect_state.server_state == CONNECT_CLOSE ) /*链接关闭，区分主动还是被动关闭*/
		{
			
			
		}



		

		/*连接IC卡服务器*/

		if( connect_state.auth_state == CONNECT_IDLE )  /*没有连接*/
		{
			if( connect_state.auth_index % 2 )          /*连备用服务器*/
			{
				ctl_socket( 2, 't', jt808_param.id_0x001A, jt808_param.id_0x001B, 1 );
			}else /*连主服务器*/
			{
				ctl_socket( 2, 't', jt808_param.id_0x001D, jt808_param.id_0x001B, 1 );
			}
			connect_state.auth_state = CONNECT_PEER;
			return;
		}

		if( connect_state.auth_state == CONNECT_PEER ) /*正在连接到服务器*/
		{
			if( socketstate( 0 ) == SOCKET_READY )
			{
				connect_state.auth_state = CONNECTED;
			}else /*没有连接成功,切换服务器*/
			{
				//connect_state.auth_index++;
				//connect_state.auth_state=CONNECT_IDLE;
				//if(connect_state.auth_index>6)
				//{

				//}
			}
		}
	}
}

/*
   tts语音播报的处理

   是通过
   %TTS: 0 判断tts状态(怀疑并不是每次都有输出)
   还是AT%TTS? 查询状态
 */
void jt808_tts_proc( void )
{
	rt_err_t	ret;
	rt_size_t	len;
	uint8_t		*pinfo, *p;
	uint8_t		c;
	T_GSM_STATE oldstate;

	char		buf[20];
	char		tbl[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
/*gsm在处理其他命令*/
	oldstate = gsmstate( GSM_STATE_GET );
	if( oldstate != GSM_TCPIP )
	{
		if( oldstate != GSM_AT )
		{
			return;
		}
	}

/*是否有信息要播报*/
	ret = rt_mb_recv( &mb_tts, (rt_uint32_t*)&pinfo, 0 );
	if( ret != RT_EOK )
	{
		return;
	}

	gsmstate( GSM_AT_SEND );

	GPIO_ResetBits( GPIOD, GPIO_Pin_9 ); /*开功放*/

	sprintf( buf, "AT%%TTS=2,3,5,\"" );

	rt_device_write( dev_gsm, 0, buf, strlen( buf ) );

	rt_kprintf( "%s", buf );

	len = ( *pinfo << 8 ) | ( *( pinfo + 1 ) );
	p	= pinfo + 2;
	while( len-- )
	{
		c		= *p++;
		buf[0]	= tbl[c >> 4];
		buf[1]	= tbl[c & 0x0f];
		rt_device_write( dev_gsm, 0, buf, 2 );
		rt_kprintf( "%c%c", buf[0], buf[1] );
	}
	buf[0]	= '"';
	buf[1]	= 0x0d;
	buf[2]	= 0x0a;
	buf[3]	= 0;
	rt_device_write( dev_gsm, 0, buf, 3 );
	rt_kprintf( "%s", buf );
/*不判断，在gsmrx_cb中处理*/
	rt_free( pinfo );
	ret = gsm_send( "", RT_NULL, "%TTS: 0", RESP_TYPE_STR, RT_TICK_PER_SECOND * 35, 1 );
	GPIO_SetBits( GPIOD, GPIO_Pin_9 ); /*关功放*/
	gsmstate( oldstate );
}

/*
   at命令处理，收到OK或超时退出
 */
void jt808_at_tx_proc( void )
{
	rt_err_t	ret;
	rt_size_t	len;
	uint8_t		*pinfo, *p;
	uint8_t		c;
	T_GSM_STATE oldstate;

/*gsm在处理其他命令*/
	oldstate = gsmstate( GSM_STATE_GET );
	if( oldstate != GSM_TCPIP )
	{
		if( oldstate != GSM_AT )
		{
			return;
		}
	}

/*是否有信息要发送*/
	ret = rt_mb_recv( &mb_at_tx, (rt_uint32_t*)&pinfo, 0 );
	if( ret != RT_EOK )
	{
		return;
	}

	gsmstate( GSM_AT_SEND );

	len = ( *pinfo << 8 ) | ( *( pinfo + 1 ) );
	p	= pinfo + 2;
	ret = gsm_send( p, RT_NULL, "OK", RESP_TYPE_STR, RT_TICK_PER_SECOND * 5, 1 );
	rt_kprintf( "at_tx=%d\r\n", ret );
	rt_free( pinfo );
	gsmstate( oldstate );
}

/*
   连接状态维护
   jt808协议处理

 */
ALIGN( RT_ALIGN_SIZE )
static char thread_jt808_stack [4096];
struct rt_thread thread_jt808;

/***/
static void rt_thread_entry_jt808( void * parameter )
{
	rt_err_t			ret;
	int					i;
	uint8_t				* pstr;

	MsgListNode			* iter;
	JT808_TX_NODEDATA	* pnodedata;

	int					j = 0xaabbccdd;

	jt808_gps_init( );

	rt_kprintf( "\r\n1.id0=%08x\r\n", param_get_int( 0x0000 ) );

	param_put_int( 0x000, j );
	rt_kprintf( "\r\nwrite j=%08x read=%08x\r\n", j, param_get_int( 0x0000 ) );

	param_put( 0x000, 4, (uint8_t*)&j );
	rt_kprintf( "\r\nid0=%08x\r\n", param_get_int( 0x0000 ) );

/*读取参数，并配置*/
	//param_load( );

	list_jt808_tx	= msglist_create( );
	list_jt808_rx	= msglist_create( );

	while( 1 )
	{
/*接收gprs信息*/
		ret = rt_mb_recv( &mb_gprsrx, ( rt_uint32_t* )&pstr, 5 );
		if( ret == RT_EOK )
		{
			jt808_rx_proc( pstr );
			rt_free( pstr );
		}

		jt808_socket_proc( );   /*jt808 socket处理*/

		jt808_tts_proc( );      /*tts处理*/

		jt808_at_tx_proc( );    /*at命令处理*/

/*短信处理*/
		SMS_Process( );

/*发送信息逐条处理*/
		iter = list_jt808_tx->first;

		if( jt808_tx_proc( iter ) == MSGLIST_RET_DELETE_NODE )  /*删除该节点*/
		{
			//rt_kprintf( "%d>%s,%d\r\n", rt_tick_get( ), __func__, __LINE__ );
			pnodedata = ( JT808_TX_NODEDATA* )( iter->data );
			
			rt_kprintf("\r\n 删除节点,head_id=%X",pnodedata->head_id);
			if( pnodedata->multipacket )
				{
				rt_free(pnodedata->user_para);
				rt_free(pnodedata->tag_data);
				pnodedata->user_para = RT_NULL;
				pnodedata->tag_data  = RT_NULL;
				}
			rt_free( pnodedata );                               /*删除节点数据*/
			list_jt808_tx->first = iter->next;                  /*指向下一个*/
			list_jt808_tx->first->prev = RT_NULL;
			rt_free( iter );
		}
		rt_thread_delay( RT_TICK_PER_SECOND / 20 );
	}

	msglist_destroy( list_jt808_tx );
}


/*********************************************************************************
*函数名称:void BkpSram_init(void)
*功能描述:backup sram 初始化
*输	入:	none
*输	出:	none
*返 回 值:void
*作	者:白养民
*创建日期:2013-06-18
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
void BkpSram_init(void)
{
	u16 uwIndex,uwErrorIndex=0;
	
	/* Enable the PWR APB1 Clock Interface */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

	/* Allow access to BKP Domain */
	PWR_BackupAccessCmd(ENABLE);
	
    /* Enable BKPSRAM Clock */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_BKPSRAM, ENABLE);
	
	/* Enable the Backup SRAM low power Regulator to retain it's content in VBAT mode */
	PWR_BackupRegulatorCmd(ENABLE);

	/* Wait until the Backup SRAM low power Regulator is ready */
	while(PWR_GetFlagStatus(PWR_FLAG_BRR) == RESET)
	{
	}
	//rt_kprintf("\r\n BkpSram_init OK!");
}


/*********************************************************************************
*函数名称:u8 BkpSram_write(u32 addr,u8 *data, u16 len)
*功能描述:backup sram 数据写入
*输	入:	addr	:写入的地址
		data	:写入的数据指针
		len		:写入的长度
*输	出:	none
*返 回 值:u8	:	0:表示操作失败，	1:表示操作成功
*作	者:白养民
*创建日期:2013-06-18
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
u8 BkpSram_write(u32 addr,u8 *data, u16 len)
{
	u32 i;
	//addr &= 0xFFFC;
	//*(__IO uint32_t *) (BKPSRAM_BASE + addr) = data;
	for(i=0;i<len;i++)
		{
		if(addr<0x1000)
			{
			*(__IO uint8_t *) (BKPSRAM_BASE + addr) = *data++;
			}
		else
			{
			return 0;
			}
		++addr;
		}
	return 1;
}

/*********************************************************************************
*函数名称:u16 bkpSram_read(u32 addr,u8 *data, u16 len)
*功能描述:backup sram 数据读取
*输	入:	addr	:读取的地址
		data	:读取的数据指针
		len		:读取的长度
*输	出:	none
*返 回 值:u16	:表示实际读取的长度
*作	者:白养民
*创建日期:2013-06-18
*---------------------------------------------------------------------------------
*修 改 人:
*修改日期:
*修改描述:
*********************************************************************************/
u16 BkpSram_read(u32 addr,u8 *data, u16 len)
{
	u32 i;
	//addr &= 0xFFFC;
	//data = *(__IO uint32_t *) (BKPSRAM_BASE + addr);
	for(i=0;i<len;i++)
		{
		if(addr<0x1000)
			{
			*data++ = *(__IO uint8_t *) (BKPSRAM_BASE + addr);
			}
		else
			{
			break;
			}
		++addr;
		}
	return i;
}

void BkpSram_wr(u32 addr,char *psrc)
{
	char pstr[128];
	memset(pstr,0,sizeof(pstr));
	memcpy(pstr,psrc,strlen(psrc));
	BkpSram_write(addr,pstr,strlen(pstr)+1);
}
FINSH_FUNCTION_EXPORT( BkpSram_wr, write from backup sram );


void BkpSram_rd(u32 addr)
{
	char pstr[128];
	BkpSram_read(addr,pstr,sizeof(pstr));
	rt_kprintf("\r\n str=%s\r\n",pstr);
}
FINSH_FUNCTION_EXPORT( BkpSram_rd, read from backup sram );


/*jt808处理线程初始化*/
void jt808_init( void )
{
	sms_init( );
	BkpSram_init( );

	vdr_init();

	dev_gsm = rt_device_find( "gsm" );
	rt_mb_init( &mb_gprsrx, "mb_gprs", &mb_gprsrx_pool, MB_GPRSDATA_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );

	rt_mb_init( &mb_tts, "mb_tts", &mb_tts_pool, MB_TTS_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );

	rt_mb_init( &mb_at_tx, "mb_at_tx", &mb_at_tx_pool, MB_AT_TX_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );

	rt_thread_init( &thread_jt808,
	                "jt808",
	                rt_thread_entry_jt808,
	                RT_NULL,
	                &thread_jt808_stack [0],
	                sizeof( thread_jt808_stack ), 10, 5 );
	rt_thread_startup( &thread_jt808 );
}

/*gprs接收处理,收到数据要尽快处理*/
rt_err_t gprs_rx( uint8_t linkno, uint8_t * pinfo, uint16_t length )
{
	uint8_t * pmsg;
	pmsg = rt_malloc( length + 3 );                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            





                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               /*包含长度信息*/
	if( pmsg != RT_NULL )
	{
		pmsg [0]	= linkno;
		pmsg [1]	= length >> 8;
		pmsg [2]	= length & 0xff;
		memcpy( pmsg + 3, pinfo, length );
		rt_mb_send( &mb_gprsrx, ( rt_uint32_t )pmsg );
		return 0;
	}
	return 1;
}

#if 0


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static rt_err_t demo_jt808_tx( void )
{
	uint8_t				* pdata;
	JT808_TX_NODEDATA	* pnodedata;
	uint8_t				buf [256];
	uint16_t			len;
	uint8_t				fcs = 0;
	int					i;
/*准备要发送的数据*/
	pnodedata = rt_malloc( sizeof( JT808_TX_NODEDATA ) );
	if( pnodedata == NULL )
	{
		return -1;
	}
	pnodedata->type				= TERMINAL_CMD;
	pnodedata->state			= IDLE;
	pnodedata->retry			= 0;
	pnodedata->cb_tx_timeout	= jt808_tx_timeout;
	pnodedata->cb_tx_response	= jt808_tx_response;

	len = jt808_pack( buf, "%w%w%6s%w%w%w%5s%20s%7s%b%s",
	                  0x0100,
	                  37 + strlen( jt808_param.id_0x0083 ),
	                  term_param.mobile,
	                  tx_seq,
	                  jt808_param.id_0x0081,
	                  jt808_param.id_0x0082,
	                  term_param.producer_id,
	                  term_param.model,
	                  term_param.terminal_id,
	                  jt808_param.id_0x0084,
	                  jt808_param.id_0x0083 );

	rt_kprintf( "\r\n*********************\r\n" );
	for( i = 0; i < len; i++ )
	{
		rt_kprintf( "%02x ", buf [i] );
	}
	rt_kprintf( "\r\n*********************\r\n" );

	len = 1;
	len += jt808_pack_int( buf + len, &fcs, 0x0100, 2 );
	len += jt808_pack_int( buf + len, &fcs, 37 + strlen( jt808_param.id_0x0083 ), 2 );
	len += jt808_pack_array( buf + len, &fcs, term_param.mobile, 6 );
	len += jt808_pack_int( buf + len, &fcs, tx_seq, 2 );

	len			+= jt808_pack_int( buf + len, &fcs, jt808_param.id_0x0081, 2 );
	len			+= jt808_pack_int( buf + len, &fcs, jt808_param.id_0x0082, 2 );
	len			+= jt808_pack_array( buf + len, &fcs, term_param.producer_id, 5 );
	len			+= jt808_pack_array( buf + len, &fcs, term_param.model, 20 );
	len			+= jt808_pack_array( buf + len, &fcs, term_param.terminal_id, 7 );
	len			+= jt808_pack_int( buf + len, &fcs, jt808_param.id_0x0084, 1 );
	len			+= jt808_pack_string( buf + len, &fcs, jt808_param.id_0x0083 );
	len			+= jt808_pack_byte( buf + len, &fcs, fcs );
	buf [0]		= 0x7e;
	buf [len]	= 0x7e;
	pdata		= rt_malloc( len + 1 );
	if( pdata == NULL )
	{
		rt_free( pnodedata );
		return;
	}
	rt_kprintf( "\r\n--------------------\r\n" );
	for( i = 0; i < len + 1; i++ )
	{
		rt_kprintf( "%02x ", buf [i] );
	}
	rt_kprintf( "\r\n--------------------\r\n" );
	memcpy( pdata, buf, len + 1 );
	pnodedata->msg_len	= len + 1;
	pnodedata->pmsg		= pdata;
	pnodedata->head_sn	= tx_seq;
	pnodedata->head_id	= 0x0100;
	msglist_append( list_jt808_tx, pnodedata );
	tx_seq++;
}

FINSH_FUNCTION_EXPORT_ALIAS( demo_jt808_tx, jt808_tx, jt808_tx test );

#endif


/*
   收到tts信息并发送
   返回0:OK
    1:分配RAM错误
 */
rt_size_t tts_write( char* info )
{
	uint8_t		*pmsg;
	uint16_t	count;
	count = strlen( info );

	/*直接发送到Mailbox中,内部处理*/
	pmsg = rt_malloc( count + 2 );
	if( pmsg != RT_NULL )
	{
		*pmsg			= count >> 8;
		*( pmsg + 1 )	= count & 0xff;
		memcpy( pmsg + 2, info, count );
		rt_mb_send( &mb_tts, (rt_uint32_t)pmsg );
		return 0;
	}
	return 1;
}

FINSH_FUNCTION_EXPORT( tts_write, tts send );


/*
   发送AT命令
   如何保证不干扰,其他的执行，传入等待的时间

 */
rt_size_t at( char *sinfo )
{
	uint8_t		*pmsg;
	uint16_t	count;
	count = strlen( sinfo );

	/*直接发送到Mailbox中,内部处理*/
	pmsg = rt_malloc( count + 3 );
	if( pmsg != RT_NULL )
	{
		*pmsg			= count >> 8;
		*( pmsg + 1 )	= count & 0xff;
		memcpy( pmsg + 2, sinfo, count );
		*( pmsg + count + 2 ) = 0;
		rt_mb_send( &mb_at_tx, (rt_uint32_t)pmsg );
		return 0;
	}
	return 1;
}

FINSH_FUNCTION_EXPORT( at, write gsm );


/*
   重启设备
   填充重启原因
 */
void reset( uint32_t reason )
{
/*没有发送的数据要保存*/

/*关闭连接*/

/*日志记录时刻重启原因*/

	rt_kprintf( "\r\n%08d reset>reason=%08x", rt_tick_get( ), reason );
/*执行重启*/
	rt_thread_delay( RT_TICK_PER_SECOND*5 );
	NVIC_SystemReset( );
}

FINSH_FUNCTION_EXPORT( reset, restart device );

/************************************** The End Of File **************************************/


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
#include "jt808_area.h"
#include "jt808_misc.h"
#include "jt808_camera.h"
#include "vdr.h"

#pragma diag_error 223

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

typedef enum
{
	STOP_REPORT = 0,
	REGISTER,
	AUTH,
	REPORT,
	WAIT
}JT808_STATE;

JT808_STATE jt808_state = REGISTER;


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
MsgList					* list_jt808_rx;

static rt_device_t		dev_gsm;

struct _connect_state	connect_state = { 0, CONNECT_IDLE, 0, CONNECT_NONE, 0 };

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
   发送后收到应答处理
 */
static JT808_MSG_STATE jt808_tx_response( JT808_TX_NODEDATA * nodedata, uint8_t *pmsg )
{
	uint8_t		* msg = pmsg + 12;
	uint16_t	id;
	uint16_t	seq;
	uint8_t		res;

	seq = ( *msg << 8 ) | *( msg + 1 );
	id	= ( *( msg + 2 ) << 8 ) | *( msg + 3 );
	res = *( msg + 4 );

	//rt_kprintf( "%d>CENT_ACK id=%04x seq=%04x res=%d\r\n", rt_tick_get( ), id, seq, res );
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
			jt808_state = REPORT;
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
	JT808_TX_NODEDATA* pnodedata = nodedata;
	pnodedata->retry++;
	if( pnodedata->retry > pnodedata->max_retry )
	{
		/*处理,保存*/
		return WAIT_DELETE;
	}
	return IDLE;                                                /*等待再次发送*/
}

/*
   分配一个指定大小的node
 */
JT808_TX_NODEDATA * node_begin( uint8_t linkno,
                                JT808_MSG_TYPE fMultiPacket,    /*是否为多包*/
                                uint16_t id,
                                int32_t seq,
                                uint16_t datasize )
{
	JT808_TX_NODEDATA * pnodedata;

	if( fMultiPacket > SINGLE_ACK )
	{
		pnodedata = rt_malloc( sizeof( JT808_TX_NODEDATA ) + sizeof( JT808_MSG_HEAD_EX ) + datasize );
	} else
	{
		pnodedata = rt_malloc( sizeof( JT808_TX_NODEDATA ) + sizeof( JT808_MSG_HEAD ) + datasize );
	}
	if( pnodedata == RT_NULL )
	{
		return RT_NULL;
	}
	//memset( pnodedata, 0, sizeof( JT808_TX_NODEDATA ) ); ///绝对不能少，否则系统出错
	pnodedata->multipacket	= fMultiPacket;
	pnodedata->linkno		= linkno;
	pnodedata->state		= IDLE;
	pnodedata->head_id		= id;
	pnodedata->retry		= 0;
	if( fMultiPacket > SINGLE_ACK )
	{
		pnodedata->max_retry	= 1;
		pnodedata->timeout		= RT_TICK_PER_SECOND * 3;
	} else
	{
		pnodedata->max_retry	= 3;
		pnodedata->timeout		= RT_TICK_PER_SECOND * 5;
	}

	pnodedata->packet_num	= 1;
	pnodedata->packet_no	= 0;
	pnodedata->size			= datasize;

	if( seq == -1 )
	{
		pnodedata->head_sn = tx_seq;
		tx_seq++;
	} else
	{
		pnodedata->head_sn = seq;
	}
	return pnodedata;
}

/*
   向里面填充数据,形成有效的数据
 */
JT808_TX_NODEDATA * node_data( JT808_TX_NODEDATA *pnodedata,
                               uint8_t* pinfo, uint16_t len,
                               JT808_MSG_STATE ( *cb_tx_timeout )( ),
                               JT808_MSG_STATE ( *cb_tx_response )( ),
                               void  *userpara )
{
	uint8_t* pdata;
	pdata = pnodedata->tag_data;

	pdata[0]	= pnodedata->head_id >> 8;
	pdata[1]	= pnodedata->head_id & 0xff;
	pdata[2]	= ( len >> 8 );
	pdata[3]	= len & 0xff;
	pdata[10]	= pnodedata->head_sn >> 8;
	pdata[11]	= pnodedata->head_sn & 0xff;

	memcpy( pdata + 4, mobile, 6 );
	if( pnodedata->multipacket > SINGLE_ACK )   /*多包数据*/
	{
		pdata[2] += 0x20;
		memcpy( pdata + 16, pinfo, len );       /*填充用户数据*/
		pnodedata->msg_len = len + 16;
	} else
	{
		memcpy( pdata + 12, pinfo, len );       /*填充用户数据*/
		pnodedata->msg_len = len + 12;
	}
	pnodedata->user_para = userpara;

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
}

/*修整发送数据的信息长度*/
void node_datalen( JT808_TX_NODEDATA* pnodedata, uint16_t datalen )
{
	uint8_t* pdata_head = pnodedata->tag_data;

	pdata_head[2]		= datalen >> 8;
	pdata_head[3]		= datalen & 0xFF;
	pnodedata->msg_len	= datalen + 12;         /*缺省是单包*/
	if( pnodedata->multipacket > SINGLE_ACK )   /*多包数据*/
	{
		pdata_head[12]	= pnodedata->packet_num >> 8;
		pdata_head[13]	= pnodedata->packet_num & 0xFF;
		pnodedata->packet_no++;
		pdata_head[14]		= pnodedata->packet_no >> 8;
		pdata_head[15]		= pnodedata->packet_no & 0xFF;
		pdata_head[2]		|= 0x20;            /*多包发送*/
		pnodedata->msg_len	+= 4;
	}
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
uint8_t* node_msg_body( JT808_TX_NODEDATA* pnodedata )
{
	if( pnodedata->multipacket > SINGLE_ACK )
	{
		return pnodedata->tag_data + 16;
	} else
	{
		return pnodedata->tag_data + 12;
	}
}

/*添加到发送列表**/
void node_end( JT808_TX_NODEDATA* pnodedata )
{
	msglist_append( list_jt808_tx, pnodedata );
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
rt_err_t jt808_add_tx( uint8_t linkno,
                       JT808_MSG_TYPE fMultiPacket,         /*是否为多包*/
                       uint16_t id,
                       int32_t seq,
                       JT808_MSG_STATE ( *cb_tx_timeout )( ),
                       JT808_MSG_STATE ( *cb_tx_response )( ),
                       uint16_t len,                        /*信息长度*/
                       uint8_t *pinfo,
                       void  *userpara )

{
	JT808_TX_NODEDATA* pnodedata;

	pnodedata = node_begin( linkno, fMultiPacket, id, seq, len );
	node_data( pnodedata, pinfo, len, cb_tx_timeout, cb_tx_response, userpara );
	msglist_append( list_jt808_tx, pnodedata );
}

/*
   终端通用应答
 */
rt_err_t jt808_tx_0x0001( uint16_t seq, uint16_t id, uint8_t res )
{
	uint8_t				* pdata;
	JT808_TX_NODEDATA	* pnodedata;
	uint8_t				* p;
	uint16_t			len;
	uint8_t				buf[5];

	pnodedata = node_begin( 1, SINGLE_ACK, 0x0001, -1, 5 );
	if( pnodedata == NULL )
	{
		return RT_ENOMEM;
	}
	buf[0]	= ( seq >> 8 );
	buf[1]	= ( seq & 0xff );
	buf[2]	= ( id >> 8 );
	buf[3]	= ( id & 0xff );
	buf[4]	= res;
	node_data( pnodedata, buf, 5, jt808_tx_timeout, jt808_tx_response, RT_NULL );
	node_end( pnodedata );
}

/*
   平台通用应答,收到信息，停止发送
 */
static int handle_rx_0x8001( uint8_t linkno, uint8_t *pmsg )
{
	MsgListNode			* iter;
	JT808_TX_NODEDATA	* iterdata;

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
		iterdata->cb_tx_response( iterdata, pmsg ); /*应答处理函数*/
		iterdata->state = ACK_OK;
	}
	return 1;
}

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
			strncpy( jt808_param.id_0xF003, msg + 3, body_len - 3 );
			iterdata->state = ACK_OK;
			jt808_state		= AUTH;
		}
	}
	return 1;
}

/*设置终端参数*/
static int handle_rx_0x8103( uint8_t linkno, uint8_t *pmsg )
{
	uint8_t		* p;
	uint8_t		res = 0;
	uint32_t	param_id;
	uint8_t		param_len;
	uint8_t		param_count;
	uint16_t	offset;
	uint16_t	seq, id;

	if( *( pmsg + 2 ) >= 0x20 ) /*如果是多包的设置参数*/
	{
		rt_kprintf( "\r\n>%s multi packet no support!", __func__ );
		jt808_tx_0x0001( seq, id, 1 );
		return 1;
	}

	id	= ( pmsg[0] << 8 ) | pmsg[1];
	seq = ( pmsg[10] << 8 ) | pmsg[11];

	param_count = pmsg[12];
	offset		= 13;
	for( param_count = 0; param_count < pmsg[12]; param_count++ )
	{
		p			= pmsg + offset;
		param_id	= ( p[0] << 24 ) | ( p[1] << 16 ) | ( p[2] << 8 ) | ( p[3] );
		param_len	= p[4];
		res			|= param_put( param_id, param_len, &p[5] );
		offset		+= ( param_len + 5 );
		rt_kprintf( "\r\n0x8103>id=%x res=%d \r\n", param_id, res );
	}

	if( res ) /*有错误*/
	{
		jt808_tx_0x0001( seq, id, 1 );
	}else
	{
		jt808_tx_0x0001( seq, id, 0 );
		param_save( );
	}
	return 1;
}

/*查询全部终端参数，有可能会超出单包最大字节*/
static int handle_rx_0x8104( uint8_t linkno, uint8_t *pmsg )
{
	jt808_param_0x8104( pmsg );
	return 1;
}

/*终端控制*/
static int handle_rx_0x8105( uint8_t linkno, uint8_t *pmsg )
{
	uint8_t		cmd;
	uint8_t		* cmd_arg;
	uint16_t	seq = ( pmsg[10] << 8 ) | pmsg[11];

	cmd = *( pmsg + 12 );
	switch( cmd )
	{
		case 1:                         /*无线升级*/
			break;
		case 2:                         /*终端控制链接指定服务器*/
			break;
		case 3:                         /*终端关机*/
			break;
		case 4:                         /*终端复位*/
			break;
		case 5:                         /*恢复出厂设置*/
			break;
		case 6:                         /*关闭数据通讯*/
			break;
		case 7:                         /*关闭所有无线通讯*/
			break;
	}
	jt808_tx_0x0001( seq, 0x8105, 3 );  /*直接返回不支持*/
	return 1;
}

/*查询指定终端参数,返回应答0x0104*/
static int handle_rx_0x8106( uint8_t linkno, uint8_t *pmsg )
{
	jt808_param_0x8106( pmsg );
	return 1;
}

/*查询终端属性,应答 0x0107*/
static int handle_rx_0x8107( uint8_t linkno, uint8_t *pmsg )
{
	uint8_t				buf[100];
	uint8_t				len1, len2;
	JT808_TX_NODEDATA	*pnodedata;

	pnodedata = node_begin( 1, SINGLE_ACK, 0x0107, -1, 100 );
	if( pnodedata == RT_NULL )
	{
		return 0;
	}

	buf[0]	= jt808_param.id_0xF004 >> 8;
	buf[1]	= jt808_param.id_0xF004 & 0xFF;
	memcpy( buf + 2, jt808_param.id_0xF000, 5 );    /*制造商ID*/
	memcpy( buf + 7, jt808_param.id_0xF001, 20 );   /*终端型号*/
	memcpy( buf + 27, jt808_param.id_0xF002, 7 );   /*终端ID*/
	memcpy( buf + 34, "1234567890", 10 );           /*终端ICCID*/
	len1	= strlen( jt808_param.id_0xF011 );
	buf[44] = len1;
	memcpy( buf + 45, jt808_param.id_0xF011, len1 );
	len2			= strlen( jt808_param.id_0xF010 );
	buf[45 + len1]	= len2;
	memcpy( buf + 46 + len1, jt808_param.id_0xF011, len2 );
	buf[46 + len1 + len2]		= 0x03;
	buf[46 + len1 + len2 + 1]	= 0x01;

	node_data( pnodedata, buf, ( 46 + len1 + len2 + 2 ), jt808_tx_timeout, jt808_tx_response, RT_NULL );
	node_end( pnodedata );
	return 1;
}

/*位置信息查询*/
static int handle_rx_0x8201( uint8_t linkno, uint8_t *pmsg )
{
	uint8_t buf[40];
	buf[0]	= pmsg[10];
	buf[1]	= pmsg[11];
	memcpy( buf + 2, (uint8_t*)&gps_baseinfo, 28 );
	jt808_tx_ack( 0x0201, buf, 30 );

	return 1;
}

/*临时位置跟踪控制*/
static int handle_rx_0x8202( uint8_t linkno, uint8_t *pmsg )
{
	uint16_t interval;
	interval = ( pmsg[12] << 8 ) | pmsg[13];
	if( interval == 0 )
	{
		jt808_8202_track_duration = 0; /*停止跟踪*/
	}else
	{
		jt808_8202_track_interval	= interval;
		jt808_8202_track_duration	= ( pmsg[14] << 24 ) | ( pmsg[15] << 16 ) | ( pmsg[16] << 8 ) | ( pmsg[17] );
	}
	return 1;
}

/*人工确认报警信息*/
static int handle_rx_0x8203( uint8_t linkno, uint8_t *pmsg )
{
	jt808_8203_manual_ack_seq	= ( pmsg[12] << 8 ) | pmsg[13];
	jt808_8203_manual_ack_value = ( pmsg[14] << 24 ) | ( pmsg[15] << 16 ) | ( pmsg[16] << 8 ) | ( pmsg[17] );
	return 1;
}

/*文本信息下发*/
static int handle_rx_0x8300( uint8_t linkno, uint8_t *pmsg )
{
	jt808_misc_0x8300( pmsg );
	return 1;
}

/*事件设置*/
static int handle_rx_0x8301( uint8_t linkno, uint8_t *pmsg )
{
	jt808_misc_0x8301( pmsg );
	return 1;
}

/*提问下发*/
static int handle_rx_0x8302( uint8_t linkno, uint8_t *pmsg )
{
	jt808_misc_0x8302( pmsg );
	return 1;
}

/*信息点播菜单设置*/
static int handle_rx_0x8303( uint8_t linkno, uint8_t *pmsg )
{
	jt808_misc_0x8303( pmsg );
	return 1;
}

/*信息服务*/
static int handle_rx_0x8304( uint8_t linkno, uint8_t *pmsg )
{
	jt808_misc_0x8304( pmsg );

	return 1;
}

/*电话回拨*/
static int handle_rx_0x8400( uint8_t linkno, uint8_t *pmsg )
{
	jt808_misc_0x8400( pmsg );
	return 1;
}

/*设置电话本*/
static int handle_rx_0x8401( uint8_t linkno, uint8_t *pmsg )
{
	jt808_misc_0x8401( pmsg );
	return 1;
}

/*车辆控制*/
static int handle_rx_0x8500( uint8_t linkno, uint8_t *pmsg )
{
	jt808_misc_0x8500( pmsg );

	return 1;
}

/*设置圆形区域*/
static int handle_rx_0x8600( uint8_t linkno, uint8_t *pmsg )
{
	area_jt808_0x8600( linkno, pmsg );
	return 1;
}

/*删除圆形区域*/
static int handle_rx_0x8601( uint8_t linkno, uint8_t *pmsg )
{
	area_jt808_0x8601( linkno, pmsg );

	return 1;
}

/*设置矩形区域*/
static int handle_rx_0x8602( uint8_t linkno, uint8_t *pmsg )
{
	area_jt808_0x8602( linkno, pmsg );

	return 1;
}

/*删除矩形区域*/
static int handle_rx_0x8603( uint8_t linkno, uint8_t *pmsg )
{
	area_jt808_0x8603( linkno, pmsg );

	return 1;
}

/*设置多边形区域*/
static int handle_rx_0x8604( uint8_t linkno, uint8_t *pmsg )
{
	area_jt808_0x8604( linkno, pmsg );

	return 1;
}

/*删除多边形区域*/
static int handle_rx_0x8605( uint8_t linkno, uint8_t *pmsg )
{
	area_jt808_0x8605( linkno, pmsg );

	return 1;
}

/*设置路线*/
static int handle_rx_0x8606( uint8_t linkno, uint8_t *pmsg )
{
	area_jt808_0x8606( linkno, pmsg );

	return 1;
}

/*删除路线*/
static int handle_rx_0x8607( uint8_t linkno, uint8_t *pmsg )
{
	area_jt808_0x8607( linkno, pmsg );

	return 1;
}

/*行驶记录仪数据采集*/

static int handle_rx_0x8700( uint8_t linkno, uint8_t *pmsg )
{
	vdr_rx_8700( pmsg );
	return 1;
}

/*行驶记录仪参数下传*/
static int handle_rx_0x8701( uint8_t linkno, uint8_t *pmsg )
{
	vdr_rx_8701( pmsg );
	return 1;
}

/*
   多媒体数据上传应答
   会有不同的消息通过此接口
 */
static int handle_rx_0x8800( uint8_t linkno, uint8_t *pmsg )
{
	MsgListNode			* iter;
	JT808_TX_NODEDATA	* iterdata;
	uint32_t			media_id;

	/*跳过消息头12byte*/
	media_id	= ( pmsg[12] << 24 ) | ( pmsg[13] << 16 ) | ( pmsg[14] << 8 ) | ( pmsg[15] );
	iter		= list_jt808_tx->first;
	while( iter != RT_NULL )
	{
		iterdata = (JT808_TX_NODEDATA*)iter->data;
		if( iterdata->head_id == media_id )
		{
			iterdata->cb_tx_response( iterdata, pmsg ); /*应答处理函数*/
			iterdata->state = ACK_OK;
			break;
		}else
		{
			iter = iter->next;
		}
	}

	return 1;
}

/*摄像头立即拍摄命令*/
static int handle_rx_0x8801( uint8_t linkno, uint8_t *pmsg )
{
	Cam_jt808_0x8801( linkno, pmsg );
	return 1;
}

/*
   多媒体信息检索
 */
static int handle_rx_0x8802( uint8_t linkno, uint8_t *pmsg )
{
	Cam_jt808_0x8802( linkno, pmsg );

	return 1;
}

/**/
static int handle_rx_0x8803( uint8_t linkno, uint8_t *pmsg )
{
	Cam_jt808_0x8803( linkno, pmsg );

	return 1;
}

/*录音开始*/
static int handle_rx_0x8804( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/*单条存储多媒体数据检索上传*/
static int handle_rx_0x8805( uint8_t linkno, uint8_t *pmsg )
{
	Cam_jt808_0x8805( linkno, pmsg );
	return 1;
}

/*数据下行透传*/
static int handle_rx_0x8900( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/*平台RSA公钥*/
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
	DECL_JT808_RX_HANDLE( 0x8106 ), /*查询终端属性,应答 0x0107*/
	DECL_JT808_RX_HANDLE( 0x8201 ), // 位置信息查询    位置信息查询消息体为空
	DECL_JT808_RX_HANDLE( 0x8202 ), // 临时位置跟踪控制
	DECL_JT808_RX_HANDLE( 0x8203 ), /*人工确认报警信息*/
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
void cb_socket_close( uint8_t cid )
{
	if( cid == 1 )
	{
		connect_state.server_state = CONNECT_CLOSE;
		rt_kprintf( "%d>linkno %d close\r\n", rt_tick_get( ), cid );
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

   20130625 会有粘包的情况

 */
uint16_t jt808_rx_proc( uint8_t * pinfo )
{
	uint8_t		* psrc, *pdst, *pdata;
	uint16_t	total_len, len;
	uint8_t		linkno;
	uint16_t	i, id;
	uint8_t		flag_find	= 0;
	uint8_t		fcs			= 0;
	uint16_t	ret;
	uint16_t	count;
	uint8_t		fstuff = 0;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          /*是否字节填充*/

	linkno		= pinfo [0];
	total_len	= ( pinfo [1] << 8 ) | pinfo [2];

	psrc	= pinfo + 3;
	pdst	= pinfo + 3;

	count = 0;

/*处理粘包*/
	while( total_len )
	{
		if( *psrc == 0x7e )         /*包头包尾标志*/
		{
			if( count )             /*有数据*/
			{
				if( fcs == 0 )      /*数据正确*/
				{
					*psrc	= 0;    /*20120711 置为字符串结束标志*/
					id		= ( ( *pdata ) << 8 ) | *( pdata + 1 );
					for( i = 0; i < sizeof( handle_rx_msg ) / sizeof( HANDLE_JT808_RX_MSG ); i++ )
					{
						if( id == handle_rx_msg [i].id )
						{
							handle_rx_msg [i].func( linkno, pdata );
							flag_find = 1;
						}
					}
					if( !flag_find )
					{
						handle_rx_default( linkno, pdata );
						flag_find = 0;
					}
				}else
				{
					rt_kprintf( "count=%d,fcs err=%d\r\n", count, fcs );
				}
			}
			fcs		= 0;
			pdata	= psrc;         /*指向数据头,0x7E的位置*/
			pdst	= psrc;
		}else if( *psrc == 0x7d )   /*是转义字符等待处理下一个*/
		{
			fstuff = 0x7c;
		} else
		{
			*pdst	= *psrc + fstuff;
			fstuff	= 0;
			count++;
			fcs ^= *pdst;
			pdst++;
		}
		psrc++;
		total_len--;
	}
}

/*
   处理每个要发送信息的状态
   现在允许并行处理吗?

   2013.06.08增加多包发送处理
 */
#if 0
static MsgListRet jt808_tx_proc( MsgListNode * node )
{
	MsgListNode			* pnode		= ( MsgListNode* )node;
	JT808_TX_NODEDATA	* pnodedata = ( JT808_TX_NODEDATA* )( pnode->data );
	int					i;
	rt_err_t			ret;

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

		ret = socket_write( pnodedata->linkno, pnodedata->tag_data, pnodedata->msg_len );
		if( ret == RT_EOK ) /*发送成功等待中心应答中*/
		{
			pnodedata->tick = rt_tick_get( );
			pnodedata->retry++;
			pnodedata->timeout	= pnodedata->retry * jt808_param.id_0x0002 * RT_TICK_PER_SECOND;
			pnodedata->state	= WAIT_ACK;
			rt_kprintf( "send retry=%d,timeout=%d\r\n", pnodedata->retry, pnodedata->timeout * 10 );
		}else /*发送数据没有等到模块返回的OK，立刻重发，还是等一段时间再发*/
		{
			pnodedata->retry++;         /*置位再次发送*/
			total_send_error++;
			rt_kprintf( "total_send_error=%d\r\n", total_send_error );
		}
		return MSGLIST_RET_OK;
	}

	if( pnodedata->state == WAIT_ACK )  /*检查中心应答是否超时*/
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

#endif


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
static JT808_MSG_STATE jt808_tx_proc( MsgListNode * node )
{
	MsgListNode			* pnode		= ( MsgListNode* )node;
	JT808_TX_NODEDATA	* pnodedata = ( JT808_TX_NODEDATA* )( pnode->data );
	int					i;
	rt_err_t			ret;

	if( node == RT_NULL )
	{
		return IDLE;
	}

	if( pnodedata->state == IDLE ) /*空闲，发送信息或超时后没有数据*/
	{
#if 1
		/*要判断是不是出于GSM_TCPIP状态,当前socket是否可用*/
		if( gsmstate( GSM_STATE_GET ) != GSM_TCPIP )
		{
			return IDLE;
		}
		if( connect_state.server_state != CONNECTED )
		{
			return IDLE;
		}

		ret = socket_write( pnodedata->linkno, pnodedata->tag_data, pnodedata->msg_len );
#else
		do
		{
			uint8_t* pdata = pnodedata->tag_data;
			rt_kprintf( "\r\n>DUMP BEGIN\r\n" );
			for( i = 0; i < pnodedata->msg_len; i++ )
			{
				if( ( i % 16 ) == 0 )
				{
					rt_kprintf( "\r\n" );
				}
				rt_kprintf( "%02x ", *pdata++ );
			}
			rt_kprintf( "\r\n>DUMP END\r\n" );
		}
		while( 0 );
		ret = RT_EOK;
#endif
		if( ret != RT_EOK )                                                                             /*发送数据没有等到模块返回的OK，立刻重发，还是等一段时间再发*/
		{
			total_send_error++;
			if( total_send_error++ > 3 )                                                                /*m66 发送失败,如何处理*/
			{
				gsmstate( GSM_POWEROFF );
			}
			rt_kprintf( "total_send_error=%d\r\n", total_send_error );
		}

		pnodedata->timeout_tick = rt_tick_get( ) + ( pnodedata->retry + 1 ) * pnodedata->timeout - 30;  /*减30是为了修正*/
		pnodedata->state		= WAIT_ACK;
		rt_kprintf( "%d>send retry=%d,timeout=%d\r\n", rt_tick_get( ), pnodedata->retry, pnodedata->timeout * 10 );
		return IDLE;
	}

	if( pnodedata->state == WAIT_ACK )                                                                  /*检查中心应答是否超时*/
	{
		if( rt_tick_get( ) >= pnodedata->timeout_tick )
		{
			return pnodedata->cb_tx_timeout( pnodedata );
		}
		return WAIT_ACK;
	}

	if( pnodedata->state == ACK_OK )
	{
		return WAIT_DELETE;
	}

	return IDLE;
}

/*
   808连接处理
   是以gsm状态迁移的,如果已层次状态机的模型，
   是不是从顶到底的状态处理会更好些。

   jt808_state   注册，鉴权 ，正常上报，停报
   socket_state
   gsm_state

 */
static void jt808_socket_proc( void )
{
	T_GSM_STATE			state;
	static rt_tick_t	server_heartbeat_tick	= 0;
	static rt_tick_t	auth_heartbeat_tick		= 0;
	uint8_t				buf[64];


/*
   检查是否允许gsm工作
   中间的时候关闭连接
 */
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

		if( connect_state.server_state == CONNECTED )               /*链路维护心跳包*/
		{
			/*判断当前链接是否异常*/
			if( socketstate( SOCKET_STATE ) == CONNECT_CLOSE )      /*链接被挂断，是主动挂断还是网络原因*/
			{
				connect_state.server_state = CONNECT_IDLE;          /*还是在cb_socket_close中判断*/
				return;
			}

			switch( jt808_state )
			{
				case REGISTER:
					buf[0]	= jt808_param.id_0x0081 >> 8;           /*省域*/
					buf[1]	= jt808_param.id_0x0081 & 0xff;
					buf[2]	= jt808_param.id_0x0082 >> 8;           /*市域*/
					buf[3]	= jt808_param.id_0x0082 & 0xff;
					memcpy( buf + 4, jt808_param.id_0xF000, 5 );    /*制造商ID*/
					memcpy( buf + 9, jt808_param.id_0xF001, 20 );   /*终端型号*/
					memcpy( buf + 29, jt808_param.id_0xF002, 7 );   /*终端ID*/
					buf[36] = jt808_param.id_0xF004;
					strcpy( buf + 37, jt808_param.id_0xF005 );      /*车辆表示或VIN*/
					jt808_tx( 0x0100, buf, 37 + strlen( jt808_param.id_0xF005 ) );
					jt808_state = WAIT;
					break;
				case AUTH:
					jt808_tx( 0x0102, jt808_param.id_0xF003, strlen( jt808_param.id_0xF003 ) );
					jt808_state = WAIT;
					break;
				case REPORT:
					if( server_heartbeat_tick )
					{
						/*要发送心跳包*/
						if( ( rt_tick_get( ) - server_heartbeat_tick ) >= ( jt808_param.id_0x0001 * RT_TICK_PER_SECOND ) )
						{
						}
					}
					server_heartbeat_tick = rt_tick_get( ); /*首次用保留当前时刻*/
					break;
			}

			return;                                         /*直接返回，不连ICCARD*/
		}

		if( connect_state.server_state == CONNECT_CLOSE )   /*链接关闭，区分主动还是被动关闭*/
		{
			connect_state.server_state = CONNECT_IDLE;      /*重新连接*/
		}

		/*连接IC卡服务器*/

		if( connect_state.auth_state == CONNECT_IDLE )      /*没有连接*/
		{
			if( connect_state.auth_index % 2 )              /*连备用服务器*/
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

	jt808_misc_init( );
	jt808_gps_init( );

	list_jt808_tx	= msglist_create( );
	list_jt808_rx	= msglist_create( );
/*初始化其他信息*/
	if( strlen( jt808_param.id_0xF003 ) ) /*是否已有鉴权码*/
	{
		jt808_state = AUTH;
	}

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

		if( jt808_tx_proc( iter ) == WAIT_DELETE )  /*删除该节点*/
		{
			//rt_kprintf( "%d>%s,%d\r\n", rt_tick_get( ), __func__, __LINE__ );
			pnodedata = ( JT808_TX_NODEDATA* )( iter->data );
			rt_free( pnodedata->user_para );
			rt_free( pnodedata );                   /*删除节点数据*/
			list_jt808_tx->first = iter->next;      /*指向下一个*/
			rt_free( iter );
		}
		rt_thread_delay( RT_TICK_PER_SECOND / 20 );
	}

	msglist_destroy( list_jt808_tx );
}

#define BKSRAM

#ifdef BKSRAM

/**/
void bkpsram_init( void )
{
	u16 uwIndex, uwErrorIndex = 0;

	/* Enable the PWR APB1 Clock Interface */
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_PWR, ENABLE );

	/* Allow access to BKP Domain */
	PWR_BackupAccessCmd( ENABLE );

	/* Enable BKPSRAM Clock */
	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_BKPSRAM, ENABLE );

	/* Enable the Backup SRAM low power Regulator to retain it's content in VBAT mode */
	PWR_BackupRegulatorCmd( ENABLE );

	/* Wait until the Backup SRAM low power Regulator is ready */
	while( PWR_GetFlagStatus( PWR_FLAG_BRR ) == RESET )
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
u8 bkpsram_write( u32 addr, u8 * data, u16 len )
{
	u32 i;
	//addr &= 0xFFFC;
	//*(__IO uint32_t *) (BKPSRAM_BASE + addr) = data;
	for( i = 0; i < len; i++ )
	{
		if( addr < 0x1000 )
		{
			*(__IO uint8_t*)( BKPSRAM_BASE + addr ) = *data++;
		}else
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
u16 bkpsram_read( u32 addr, u8 * data, u16 len )
{
	u32 i;
	//addr &= 0xFFFC;
	//data = *(__IO uint32_t *) (BKPSRAM_BASE + addr);
	for( i = 0; i < len; i++ )
	{
		if( addr < 0x1000 )
		{
			*data++ = *(__IO uint8_t*)( BKPSRAM_BASE + addr );
		}else
		{
			break;
		}
		++addr;
	}
	return i;
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
void bkpsram_wr( u32 addr, char *psrc )
{
	char pstr[128];
	memset( pstr, 0, sizeof( pstr ) );
	memcpy( pstr, psrc, strlen( psrc ) );
	bkpsram_write( addr, pstr, strlen( pstr ) + 1 );
}

FINSH_FUNCTION_EXPORT( bkpsram_wr, write from backup sram );

/**/
void bkpsram_rd( u32 addr )
{
	char pstr[128];
	bkpsram_read( addr, pstr, sizeof( pstr ) );
	rt_kprintf( "\r\n str=%s\r\n", pstr );
}

FINSH_FUNCTION_EXPORT( bkpsram_rd, read from backup sram );

#endif

/*jt808处理线程初始化*/
void jt808_init( void )
{
	/*读取参数，并配置,这个时候应该没有操作flash的*/
	param_load( );
#ifdef BKSRAM
	bkpsram_init( );
#endif

	sms_init( );
	vdr_init( );

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
	pmsg = rt_malloc( length + 3 );                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            /*包含长度信息*/
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

/*
   收到tts信息并发送
   返回0:OK
    1:分配RAM错误
 */
rt_size_t tts_write( char* info, uint16_t len )
{
	uint8_t *pmsg;
	/*直接发送到Mailbox中,内部处理*/
	pmsg = rt_malloc( len + 2 );
	if( pmsg != RT_NULL )
	{
		*pmsg			= len >> 8;
		*( pmsg + 1 )	= len & 0xff;
		memcpy( pmsg + 2, info, len );
		rt_mb_send( &mb_tts, (rt_uint32_t)pmsg );
		return 0;
	}
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
rt_err_t tts( char *s )
{
	tts_write( s, strlen( s ) );
}

FINSH_FUNCTION_EXPORT( tts, tts send );


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
	rt_thread_delay( RT_TICK_PER_SECOND * 5 );
	NVIC_SystemReset( );
}

FINSH_FUNCTION_EXPORT( reset, restart device );

#define DUMP_PRINT( value, format ) rt_kprintf( # value "="format, value )


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void dump_node( JT808_TX_NODEDATA *pnodedata )
{
	DUMP_PRINT( pnodedata->head_id, "%02x" );
}

/************************************** The End Of File **************************************/


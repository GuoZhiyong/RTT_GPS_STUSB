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

typedef enum
{
	STOP_REPORT = 0,                    /*停报*/
	REGISTER,                           /*注册*/
	AUTH,                               /*鉴权*/
	REPORT,                             /*上报*/
	WAIT                                /*等待*/
}JT808_STATE;

JT808_STATE		jt808_state = REGISTER;

static uint16_t tx_seq = 0;             /*发送序号*/

static uint16_t total_send_error = 0;   /*总的发送出错计数如果达到一定的次数要重启M66*/

/*发送信息列表*/
MsgList* list_jt808_tx;

/*接收信息列表*/
MsgList				* list_jt808_rx;

static rt_tick_t	tick_server_heartbeat	= 0;
static rt_tick_t	tick_auth_heartbeat		= 0;


/*
   发送后收到应答处理
 */
static JT808_MSG_STATE jt808_tx_response( JT808_TX_NODEDATA * nodedata, uint8_t *pmsg )
{
	uint8_t		* msg = pmsg + 12;
	uint16_t	ack_id;
//	uint16_t	ack_seq;
	uint8_t		ack_res;

//	ack_seq = ( msg[0] << 8 ) | msg[1];
	ack_id	= ( msg[2] << 8 ) | msg[3];
	ack_res = *( msg + 4 );


	switch( ack_id )            // 判断对应终端消息的ID做区分处理
	{
		case 0x0002:            //	心跳包的应答
			//rt_kprintf( "\nCentre  Heart ACK!\n" );
			break;
		case 0x0101:            //	终端注销应答
			break;
		case 0x0102:            //	终端鉴权
			if( ack_res == 0 )  /*成功*/
			{
				jt808_state = REPORT;
			} else
			{
				jt808_state = REGISTER;
			}
			break;
		case 0x0800: // 多媒体事件信息上传
			break;
		case 0x0702:
			//rt_kprintf( "\n驾驶员信息上报---中心应答!" );
			break;
		case 0x0701:
			//rt_kprintf( "电子运单上报---中心应答!" );
			break;
		default:
			//rt_kprintf( "\nunknown id=%04x", ack_id );
			break;
	}
	return ACK_OK;
}

/*
   消息发送超时
 */

static JT808_MSG_STATE jt808_tx_timeout( JT808_TX_NODEDATA * nodedata )
{
	rt_kprintf( "\nsend %04x timeout\n", nodedata->head_id );
	return ACK_TIMEOUT;
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
static void convert_deviceid_to_mobile( uint8_t* pout )
{
	uint8_t *pdst = pout;
	*pdst++ = ( ( jt808_param.id_0xF006[0] - 0x30 ) << 4 ) | ( jt808_param.id_0xF006[1] - 0x30 );
	*pdst++ = ( ( jt808_param.id_0xF006[2] - 0x30 ) << 4 ) | ( jt808_param.id_0xF006[3] - 0x30 );
	*pdst++ = ( ( jt808_param.id_0xF006[4] - 0x30 ) << 4 ) | ( jt808_param.id_0xF006[5] - 0x30 );
	*pdst++ = ( ( jt808_param.id_0xF006[6] - 0x30 ) << 4 ) | ( jt808_param.id_0xF006[7] - 0x30 );
	*pdst++ = ( ( jt808_param.id_0xF006[8] - 0x30 ) << 4 ) | ( jt808_param.id_0xF006[9] - 0x30 );
	*pdst	= ( ( jt808_param.id_0xF006[10] - 0x30 ) << 4 ) | ( jt808_param.id_0xF006[11] - 0x30 );
}

/*
   分配一个指定大小的node
 */
JT808_TX_NODEDATA * node_begin( uint8_t linkno,
                                JT808_MSG_TYPE msgtype,    /*是否为多包*/
                                uint16_t id,
                                int32_t seq,
                                uint16_t datasize )
{
	JT808_TX_NODEDATA * pnodedata;

	if( msgtype > SINGLE_ACK )
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
	//rt_kprintf( "\n%d>分配(id:%04x size:%d) %p", rt_tick_get( ), id, datasize,pnodedata );
	memset( pnodedata, 0, sizeof( JT808_TX_NODEDATA ) );    ///绝对不能少，否则系统出错
	pnodedata->linkno	= linkno;
	pnodedata->state	= IDLE;
	pnodedata->head_id	= id;
	pnodedata->retry	= 0;
	pnodedata->type		= msgtype;
	if( msgtype > SINGLE_CMD )                              /*多包和单包应答都只发一次*/
	{
		pnodedata->max_retry	= 1;
		pnodedata->timeout		= RT_TICK_PER_SECOND * 10;
	} else
	{
		pnodedata->max_retry	= 3;
		pnodedata->timeout		= RT_TICK_PER_SECOND * 10;
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
                               uint8_t* pinfo, uint16_t len )
{
	uint8_t * pdata;

	pdata = pnodedata->tag_data;

	pdata[0]	= pnodedata->head_id >> 8;
	pdata[1]	= pnodedata->head_id & 0xff;
	pdata[2]	= ( len >> 8 );
	pdata[3]	= len & 0xff;
	pdata[10]	= pnodedata->head_sn >> 8;
	pdata[11]	= pnodedata->head_sn & 0xff;

	convert_deviceid_to_mobile( pdata + 4 );
	if( pnodedata->type >= MULTI_CMD )      /*多包数据*/
	{
		pdata[2] += 0x20;
		memcpy( pdata + 16, pinfo, len );   /*填充用户数据*/
		pnodedata->msg_len = len + 16;
	} else
	{
		memcpy( pdata + 12, pinfo, len );   /*填充用户数据*/
		pnodedata->msg_len = len + 12;
	}
	return pnodedata;
}

/*修整发送数据的信息长度*/
void node_datalen( JT808_TX_NODEDATA* pnodedata, uint16_t datalen )
{
	uint8_t* pdata_head = pnodedata->tag_data;

	pdata_head[2]		= datalen >> 8;
	pdata_head[3]		= datalen & 0xFF;
	pnodedata->msg_len	= datalen + 12; /*缺省是单包*/
	if( pnodedata->type > SINGLE_ACK )  /*多包数据*/
	{
		pdata_head[12]	= pnodedata->packet_num >> 8;
		pdata_head[13]	= pnodedata->packet_num & 0xFF;
		pnodedata->packet_no++;
		pdata_head[14]		= pnodedata->packet_no >> 8;
		pdata_head[15]		= pnodedata->packet_no & 0xFF;
		pdata_head[2]		|= 0x20;    /*多包发送*/
		pnodedata->msg_len	+= 4;
	}
}

/**/
uint8_t* node_msg_body( JT808_TX_NODEDATA* pnodedata )
{
	if( pnodedata->type > SINGLE_ACK )
	{
		return pnodedata->tag_data + 16;
	} else
	{
		return pnodedata->tag_data + 12;
	}
}

/*添加到发送列表**/
void node_end( JT808_MSG_TYPE msgtype,
               JT808_TX_NODEDATA* pnodedata,
               JT808_MSG_STATE ( *cb_tx_timeout )( ),
               JT808_MSG_STATE ( *cb_tx_response )( ),
               void  *userpara )
{
	pnodedata->user_para = userpara;

	convert_deviceid_to_mobile( pnodedata->tag_data + 4 );

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

	if( msgtype == SINGLE_FIRST )
	{
		msglist_prepend( list_jt808_tx, pnodedata );
	} else
	{
		msglist_append( list_jt808_tx, pnodedata );
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
void jt808_add_tx( uint8_t linkno,
                   JT808_MSG_TYPE msgtype,              /*是否为多包*/
                   uint16_t id,
                   int32_t seq,
                   JT808_MSG_STATE ( *cb_tx_timeout )( ),
                   JT808_MSG_STATE ( *cb_tx_response )( ),
                   uint16_t len,                        /*信息长度*/
                   uint8_t *pinfo,
                   void  *userpara )

{
	JT808_TX_NODEDATA* pnodedata;

	pnodedata = node_begin( linkno, msgtype, id, seq, len );
	if( pnodedata == RT_NULL )
	{
		return;
	}
	node_data( pnodedata, pinfo, len );
	node_end( msgtype, pnodedata, cb_tx_timeout, cb_tx_response, userpara );
}

/*
   终端通用应答
   只发送1次，发完后删除
 */
rt_err_t jt808_tx_0x0001( uint16_t seq, uint16_t id, uint8_t res )
{
	JT808_TX_NODEDATA	* pnodedata;
	uint8_t				buf[10];
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
	node_data( pnodedata, buf, 5 );
	node_end( SINGLE_FIRST, pnodedata, jt808_tx_timeout, jt808_tx_response, RT_NULL );
	return RT_EOK;
}

/*
   平台通用应答,收到信息，停止发送
 */
static int handle_rx_0x8001( uint8_t linkno, uint8_t *pmsg )
{
	MsgListNode			* iter;
	JT808_TX_NODEDATA	* iterdata;

	uint16_t	ack_id;
	uint16_t	ack_seq;
//	uint8_t		ack_res;
/*跳过消息头12byte*/
	ack_seq = ( *( pmsg + 12 ) << 8 ) | *( pmsg + 13 );
	ack_id	= ( *( pmsg + 14 ) << 8 ) | *( pmsg + 15 );
	rt_kprintf( "\n%d>ACK %04x:%04x:%d", rt_tick_get( ), ack_id, ack_seq, pmsg[16] );

	/*单条处理*/
	iter = list_jt808_tx->first;
	while( iter != RT_NULL )                                                /*增加遍历，有可能插入*/
	{
		iterdata = (JT808_TX_NODEDATA*)iter->data;
		if( ( iterdata->head_id == ack_id ) && ( iterdata->head_sn == ack_seq ) )
		{
			iterdata->state = iterdata->cb_tx_response( iterdata, pmsg );   /*应答处理函数*/
			return 1;
		}
		iter = iter->next;
	}
	return 1;
}

/*补传分包请求*/
static int handle_rx_0x8003( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
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

	iter = list_jt808_tx->first;
	while( iter != RT_NULL )
	{
		iterdata = iter->data;
		if( ( iterdata->head_id == 0x0100 ) && ( iterdata->head_sn == ack_seq ) )
		{
			if( res == 0 )
			{
				strncpy( jt808_param.id_0xF003, (char*)msg + 3, body_len - 3 );
				param_save( );
				iterdata->state = ACK_OK;
				jt808_state		= AUTH;
				return 1;
			}
		}
		iter = iter->next;
	}
	return 1;
}

/*设置终端参数*/
static int handle_rx_0x8103( uint8_t linkno, uint8_t *pmsg )
{
	jt808_param_0x8103( pmsg );
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
#if 0
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
#endif
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
	jt808_tx_ack( 0x0107, buf, ( 46 + len1 + len2 + 2 ) );
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
		jt808_8202_track_counter	= 0;
	}
	jt808_tx_0x0001( ( pmsg[10] << 8 ) | pmsg[11], 0x8202, 0 );
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
//	uint32_t			media_id;

	/*跳过消息头12byte*/

	iter = list_jt808_tx->first;
	while( iter != RT_NULL )
	{
		iterdata = (JT808_TX_NODEDATA*)iter->data;
		if( iterdata->head_id == 0x0801 )
		{
			iterdata->cb_tx_response( iterdata, pmsg ); /*应答处理函数*/
			return 1;
		}
		iter = iter->next;
	}
#if 0
	media_id = ( pmsg[12] << 24 ) | ( pmsg[13] << 16 ) | ( pmsg[14] << 8 ) | ( pmsg[15] );

	while( iter != RT_NULL )
	{
		iterdata = (JT808_TX_NODEDATA*)iter->data;
		if( iterdata->head_id == media_id )             /*这里不对*/
		{
			iterdata->cb_tx_response( iterdata, pmsg ); /*应答处理函数*/
			iterdata->state = ACK_OK;
			break;
		}else
		{
			iter = iter->next;
		}
	}
#endif
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
	rt_kprintf( "\nunknown!\n" );
	return 1;
}

#define DECL_JT808_RX_HANDLE( a ) { a, handle_rx_ ## a }
//#define DECL_JT808_TX_HANDLE( a )	{ a, handle_jt808_tx_ ## a }

HANDLE_JT808_RX_MSG handle_rx_msg[] =
{
	DECL_JT808_RX_HANDLE( 0x8001 ), //	通用应答
	DECL_JT808_RX_HANDLE( 0x8003 ), //	补传分包请求
	DECL_JT808_RX_HANDLE( 0x8100 ), //  监控中心对终端注册消息的应答
	DECL_JT808_RX_HANDLE( 0x8103 ), //	设置终端参数
	DECL_JT808_RX_HANDLE( 0x8104 ), //	查询终端参数
	DECL_JT808_RX_HANDLE( 0x8105 ), // 终端控制
	DECL_JT808_RX_HANDLE( 0x8106 ), // 查询指定终端参数
	DECL_JT808_RX_HANDLE( 0x8107 ), /*查询终端属性,应答 0x0107*/
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
   1.2.3.5.6
 */
void cb_socket_close( uint8_t cid )
{
	//gsm_socket[linkno-1].state = cid;
	//rt_kprintf( "\n%d>linkno %id close:%d", rt_tick_get( ),linkno, cid );
	if( cid == 1 )
	{
		if( gsm_socket[0].state == CONNECT_CLOSED ) /*主动关闭的*/
		{
			gsm_socket[0].state = CONNECT_NONE;
		}else
		{
			gsm_socket[0].index++;
			gsm_socket[0].state = CONNECT_IDLE;     /*还是在cb_socket_close中判断*/
			jt808_state			= AUTH;             /*重新连接要重新鉴权*/
		}
	}
	if( cid == 2 )
	{
		if( gsm_socket[1].state == CONNECT_CLOSED ) /*主动关闭的*/
		{
			gsm_socket[1].state = CONNECT_NONE;
		}else
		{
			gsm_socket[1].state = CONNECT_IDLE;     /*还是在cb_socket_close中判断*/
		}
	}
	if( cid == 3 )
	{
		if( gsm_socket[2].state == CONNECT_CLOSED ) /*主动关闭的*/
		{
			gsm_socket[2].state = CONNECT_NONE;
		}else
		{
			gsm_socket[2].state = CONNECT_IDLE;     /*还是在cb_socket_close中判断*/
		}
	}
	if( cid == 5 )                                  /*全部连接挂断*/
	{
	}
	pcurr_socket = RT_NULL;
}

/*
   接收处理
   分析jt808格式的数据
   <linkno><长度2byte><标识0x7e><消息头><消息体><校验码><标识0x7e>

   20130625 会有粘包的情况

 */
void jt808_rx_proc( uint8_t * pinfo )
{
	uint8_t		* psrc, *pdst, *pdata;
	uint16_t	total_len;
	uint8_t		linkno;
	uint16_t	i, id;
	uint8_t		flag_find	= 0;
	uint8_t		fcs			= 0;
	uint16_t	count;
	uint8_t		fstuff = 0;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /*是否字节填充*/

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
					rt_kprintf( "\ncount=%d,fcs err=%d", count, fcs );
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

/*发送控制*/
static void jt808_tx_proc( MsgListNode * node )
{
	MsgListNode			* pnode		= ( MsgListNode* )node;
	JT808_TX_NODEDATA	* pnodedata = ( JT808_TX_NODEDATA* )( pnode->data );
	rt_err_t			ret;

	if( pnodedata->state == IDLE ) /*空闲，发送信息或超时后没有数据*/
	{
		/*要判断是不是出于GSM_TCPIP状态,当前socket是否可用*/
		if( gsmstate( GSM_STATE_GET ) != GSM_TCPIP )
		{
			return;
		}
		if( gsm_socket[0].state != CONNECTED )
		{
			return;
		}
		gsmstate( GSM_AT_SEND );
		rt_kprintf( "\n%d socket>", rt_tick_get( ) );
		ret = socket_write( pnodedata->linkno, pnodedata->tag_data, pnodedata->msg_len );

		if( ret != RT_EOK ) /*gsm<ERROR:41 ERROR:35	发送数据没有等到模块返回的OK，立刻重发，还是等一段时间再发*/
		{
			gsm_socket[0].state = CONNECT_IDLE;
			gsmstate( GSM_POWEROFF );
			return;
		}
		gsmstate( GSM_TCPIP );
		tick_server_heartbeat = rt_tick_get( );

		if( pnodedata->type == SINGLE_ACK )                                                                 /*应答信息，只发一遍，发完删除即可*/
		{
			pnodedata->state = ACK_OK;                                                                      /*发完就可以删除了*/
		}else
		{
			pnodedata->timeout_tick = rt_tick_get( ) + ( pnodedata->retry + 1 ) * pnodedata->timeout -10;  /*减10是为了修正*/
			pnodedata->state		= WAIT_ACK;
			rt_kprintf( "\n%d>SEND %04x:%04x (%d/%d:%dms)",
			            rt_tick_get( ),
			            pnodedata->head_id,
			            pnodedata->head_sn,
			            pnodedata->retry + 1,
			            pnodedata->max_retry,
			            ( pnodedata->retry + 1 ) * pnodedata->timeout * 10 );
		}
	}

	if( pnodedata->state == WAIT_ACK )                                      /*检查中心应答是否超时*/
	{
		if( rt_tick_get( ) >= pnodedata->timeout_tick )
		{
			pnodedata->retry++;
			if( pnodedata->retry >= pnodedata->max_retry )
			{
				pnodedata->state = pnodedata->cb_tx_timeout( pnodedata );   /*20130912 不够简练,已经判断超时了,对于多媒体信息有用*/
			}else
			{
				pnodedata->state = IDLE;                                    /*等待下次发送*/
			}
		}
	}

	if( ( pnodedata->state == ACK_TIMEOUT ) || ( pnodedata->state == ACK_OK ) )
	{
		//rt_kprintf( "\n%d>free node(%04x) %p", rt_tick_get( ), pnodedata->head_id, pnodedata );
		rt_free( pnodedata->user_para );
		rt_free( pnodedata );               /*删除节点数据*/
		list_jt808_tx->first = node->next;  /*指向下一个*/
		rt_free( node );
	}
}

/*
   M66规定三个socket 0..2 对应的linkno为1..3
   其中linkno
   1 上报公共货运平台
   2 上报河北或天津的平台--有可能会同时上报
   3 上报IC卡中心或更新服务器操作

 */
GSM_SOCKET	gsm_socket[3];

GSM_SOCKET	*pcurr_socket = RT_NULL;

//GSM_SOCKET socket_master;
//GSM_SOCKET socket_slave;
//GSM_SOCKET socket_iccard;

/*处理公共货运平台*/
static void socket_master_proc( void )
{
	uint8_t buf[64];
	if( gsm_socket[0].state == CONNECT_NONE )   /*不连接*/
	{
		return;
	}
	if( gsm_socket[0].state == CONNECT_IDLE )
	{
		if( gsm_socket[0].index % 2 )           /*连备用服务器*/
		{
			strcpy( gsm_socket[0].ipstr, jt808_param.id_0x0017 );
			gsm_socket[0].port = jt808_param.id_0x0018;
		}else /*连主服务器*/
		{
			strcpy( gsm_socket[0].ipstr, jt808_param.id_0x0013 );
			gsm_socket[0].port = jt808_param.id_0x0018;
		}
		gsm_socket[0].state = CONNECT_PEER;                         /*此时gsm_state处于 GSM_SOCKET_PROC，连完后返回 GSM_TCPIP*/
		pcurr_socket		= &gsm_socket[0];
		gsmstate( GSM_SOCKET_PROC );
		jt808_state = AUTH;                                         /*重新连接要鉴权开始*/
		return;
	}

	if( gsm_socket[0].state == CONNECT_ERROR )                      /*没有连接成功,切换服务器*/
	{
		gsm_socket[0].index++;
		gsm_socket[0].state = CONNECT_IDLE;
	}

	if( gsm_socket[0].state == CONNECTED )                          /*链路维护心跳包*/
	{
		pcurr_socket = RT_NULL;
		switch( jt808_state )
		{
			case REGISTER:
				buf[0]	= jt808_param.id_0x0081 >> 8;               /*省域*/
				buf[1]	= jt808_param.id_0x0081 & 0xff;
				buf[2]	= jt808_param.id_0x0082 >> 8;               /*市域*/
				buf[3]	= jt808_param.id_0x0082 & 0xff;
				memcpy( buf + 4, jt808_param.id_0xF000, 5 );        /*制造商ID*/
				memcpy( buf + 9, jt808_param.id_0xF001, 20 );       /*终端型号*/
				memcpy( buf + 29, jt808_param.id_0xF002, 7 );       /*终端ID*/
				buf[36] = jt808_param.id_0x0084;
				strcpy( (char*)buf + 37, jt808_param.id_0x0083 );   /*车辆表示或VIN*/
				jt808_add_tx( 1,
				              SINGLE_FIRST,
				              0x0100,
				              -1, RT_NULL, RT_NULL,
				              37 + strlen( jt808_param.id_0x0083 ), buf, RT_NULL );
				jt808_state = WAIT;
				break;
			case AUTH:
				jt808_add_tx( 1,
				              SINGLE_FIRST,
				              0x0102,
				              -1, RT_NULL, RT_NULL,
				              strlen( jt808_param.id_0xF003 ),
				              (uint8_t*)( jt808_param.id_0xF003 ), RT_NULL );
				jt808_state = WAIT;
				jt808_report_init( ); /*重新初始化一下上报*/
				break;
			case REPORT:
				if( tick_server_heartbeat )
				{
					/*要发送心跳包*/
					if( ( rt_tick_get( ) - tick_server_heartbeat ) >= ( jt808_param.id_0x0001 * RT_TICK_PER_SECOND ) )
					{
						//jt808_tx_ack( 0x0002, buf, 0 );
						tick_server_heartbeat = rt_tick_get( ); /*首次用保留当前时刻*/
					}
				}else
				{
					tick_server_heartbeat = rt_tick_get( );     /*首次用保留当前时刻*/
				}
				break;
		}
	}
}

/*处理运营商平台*/
static void socket_slave_proc( void )
{
}

/*
   处理IC卡或远程升级平台
   是否需要一上电就连接，还是按需连接

 */
static void socket_iccard_iap_proc( void )
{
	if( gsm_socket[2].state == CONNECT_IDLE )
	{
		if( gsm_socket[2].index != 2 )      /*不连接oiap*/
		{
			if( gsm_socket[2].index % 2 )   /*连备用服务器*/
			{
				strcpy( gsm_socket[2].ipstr, jt808_param.id_0x001A );
				gsm_socket[2].port = jt808_param.id_0x001B;
			}else /*连主服务器*/
			{
				strcpy( gsm_socket[2].ipstr, jt808_param.id_0x001D );
				gsm_socket[2].port = jt808_param.id_0x001B;
			}
		}
		gsm_socket[2].state = CONNECT_PEER;
		pcurr_socket		= &gsm_socket[2];
		gsmstate( GSM_SOCKET_PROC );
	}

	if( gsm_socket[2].state == CONNECT_ERROR ) /*没有连接成功,切换服务器*/
	{
		//gsm_socket[2].index++;
		//gsm_socket[2].state = CONNECT_IDLE;
		rt_kprintf( "\nsocket2 连接错误" );
	}
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
	T_GSM_STATE state;

/*检查GSM状态*/
	state = gsmstate( GSM_STATE_GET );
	if( state == GSM_IDLE )
	{
		gsmstate( GSM_POWERON );        /*开机登网*/
		return;
	}
/*控制登网*/
	if( state == GSM_AT )               /*这里要判断用那个apn user psw 登网*/
	{
		if( gsm_socket[0].index % 2 )   /*用备用服务器*/
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
/*控制建立连接,有可能会修改gsmstate状态*/
	if( gsmstate( GSM_STATE_GET ) == GSM_TCPIP )        /*已经在线了，没有处理其他socket*/
	{
		socket_master_proc( );
	}
	if( gsmstate( GSM_STATE_GET ) == GSM_TCPIP )        /*已经在线了，没有处理其他socket*/
	{
		socket_slave_proc( );
	}
	if( gsmstate( GSM_STATE_GET ) == GSM_TCPIP )        /*已经在线了，没有处理其他socket*/
	{
		socket_iccard_iap_proc( );
	}
}

/*
   连接状态维护
   jt808协议处理

 */
ALIGN( RT_ALIGN_SIZE )
//static char thread_jt808_stack [2048] CCM_RT_STACK;
static char thread_jt808_stack [2048] __attribute__((section("CCM_RT_STACK")));
struct rt_thread thread_jt808;

/***/
static void rt_thread_entry_jt808( void * parameter )
{
	rt_err_t			ret;
	uint8_t				* pstr;

	MsgListNode			* iter;
	JT808_TX_NODEDATA	* pnodedata;

	GPIO_SetBits( GPIOD, GPIO_Pin_9 );      /*关功放*/
	jt808_misc_init( );
	jt808_gps_init( );

	list_jt808_tx	= msglist_create( );
	list_jt808_rx	= msglist_create( );
/*初始化其他信息*/
	if( strlen( jt808_param.id_0xF003 ) )   /*是否已有鉴权码*/
	{
		jt808_state = AUTH;
	}
	gsm_socket[0].state		= CONNECT_IDLE; /*允许gsm_socket[0]连接*/
	gsm_socket[0].index		= 0;
	gsm_socket[0].linkno	= 1;

	gsm_socket[1].linkno	= 2;
	gsm_socket[2].linkno	= 3;
	while( 1 )
	{
/*接收gprs信息,要做分发 重启还是断网怎么办?升级中不上报数据*/
		//ret = rt_mb_recv( &mb_gprsrx, ( rt_uint32_t* )&pstr, 5 );
		ret = rt_mb_recv( &mb_gprsrx, ( rt_uint32_t* )&pstr, 0 );
		if( ret == RT_EOK )
		{
			jt808_rx_proc( pstr );
			rt_free( pstr );
		}

		jt808_socket_proc( );       /*jt808 socket处理*/

/*发送信息逐条处理*/
		iter = list_jt808_tx->first;
		if( iter == RT_NULL )       /*没有要发送的数据*/
		{
			jt808_report_get( );    /*检查有没有要发送的数据*/
		}else /*处理发送节点状态*/
		{
			jt808_tx_proc( iter );
		}
		rt_thread_delay( RT_TICK_PER_SECOND / 20 );
	}

	msglist_destroy( list_jt808_tx );
}

/*jt808处理线程初始化*/
void jt808_init( void )
{
	vdr_init( );
	rt_mb_init( &mb_gprsrx, "mb_gprs", &mb_gprsrx_pool, MB_GPRSDATA_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );
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
	pmsg = rt_malloc( length + 3 );                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /*包含长度信息*/
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
   重启设备
   填充重启原因
 */
void reset( unsigned int reason )
{
	uint32_t i = 0x7FFFFFF;
/*没有发送的数据要保存*/

/*关闭连接*/

/*日志记录时刻重启原因*/

	rt_kprintf( "\n%d reset>reason=%08x", rt_tick_get( ), reason );
/*执行重启*/
	//rt_thread_delay( RT_TICK_PER_SECOND * 3 );
	while( i-- )
	{
		;
	}

	NVIC_SystemReset( );
}

FINSH_FUNCTION_EXPORT( reset, restart device );

/*恢复出厂设置*/
void factory( void )
{
	uint16_t	i;
	uint32_t	addr = 51 * 4096;
	rt_enter_critical( );
	for( i = 51; i < 1024; i++ )
	{
		sst25_erase_4k( addr );
		addr += 4096;
	}
	NVIC_SystemReset( );
}

FINSH_FUNCTION_EXPORT( factory, reset to factory );


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
uint8_t list_node( void )
{
	MsgListNode			* iter;
	JT808_TX_NODEDATA	* pnodedata;
	uint8_t				count = 0;

	iter = list_jt808_tx->first;
	while( iter != NULL )
	{
		pnodedata = ( JT808_TX_NODEDATA* )( iter->data );
		rt_kprintf( "\nid=%04x\tseq=%04x len=%d", pnodedata->head_id, pnodedata->head_sn, pnodedata->msg_len );
		iter = iter->next;
		count++;
	}
	return count;
}

FINSH_FUNCTION_EXPORT( list_node, list node );

/************************************** The End Of File **************************************/


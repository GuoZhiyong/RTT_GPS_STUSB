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
#ifndef _H_JT808_H_
#define _H_JT808_H_

#include <stm32f4xx.h>
#include <rtthread.h>

#include "msglist.h"
#include "m66.h"

#define NEED_TODO	0



#define   MsgQ_Timeout		3
#define JT808_PACKAGE_MAX	512


/*
   存储区域分配,采用绝对地址,以4K(0x1000)为一个扇区
 */

#define ADDR_PARAM 0x000000000


/*for new use*/

#define FLASH_SEM_DELAY	2



#define BYTESWAP2( val )    \
    ( ( ( ( val ) & 0xff ) << 8 ) |   \
      ( ( ( val ) & 0xff00 ) >> 8 ) )

#define BYTESWAP4( val )    \
    ( ( ( ( val ) & 0xff ) << 24 ) |   \
      ( ( ( val ) & 0xff00 ) << 8 ) |  \
      ( ( ( val ) & 0xff0000 ) >> 8 ) |  \
      ( ( ( val ) & 0xff000000 ) >> 24 ) )

#define HEX2BCD( x )	( ( ( x ) / 10 ) << 4 | ( ( x ) % 10 ) )
#define BCD2HEX( x )	( ( ( ( x ) >> 4 ) * 10 ) + ( ( x ) & 0x0f ) )

typedef uint32_t MYTIME;

#define MYDATETIME( year, month, day, hour, minute, sec ) \
    ( (uint32_t)( ( year ) << 26 ) | \
      (uint32_t)( ( month ) << 22 ) | \
      (uint32_t)( ( day ) << 17 ) | \
      (uint32_t)( ( hour ) << 12 ) | \
      (uint32_t)( ( minute ) << 6 ) | ( sec ) )
#define YEAR( datetime )	( ( datetime >> 26 ) & 0x3F )
#define MONTH( datetime )	( ( datetime >> 22 ) & 0xF )
#define DAY( datetime )		( ( datetime >> 17 ) & 0x1F )
#define HOUR( datetime )	( ( datetime >> 12 ) & 0x1F )
#define MINUTE( datetime )	( ( datetime >> 6 ) & 0x3F )
#define SEC( datetime )		( datetime & 0x3F )


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
__inline MYTIME buf_to_time( uint8_t *p )
{
	uint32_t ret;
	ret = (uint32_t)( ( *p++ ) << 26 );
	ret |= (uint32_t)( ( *p++ ) << 22 );
	ret |= (uint32_t)( ( *p++ ) << 17 );
	ret |= (uint32_t)( ( *p++ ) << 12 );
	ret |= (uint32_t)( ( *p++ ) << 6 );
	ret |= ( *p );
	return ret;
}

typedef struct
{
	int		id;
	short	attr;
	int		latitute;   /*以度位单位的纬度值乘以10的6次方，精确到百万分之一度*/
	int		longitute;
	int		radius;     /*单位为米m，路段为该拐点到下一拐点*/
	char	start[6];
	char	end[6];
	short	speed;
	char	interval;   /*持续时间,秒*/
}GPS_AREA_CIRCLE;

#if 0
typedef enum
{
	T_NODEF = 1,
	T_BYTE,
	T_WORD,
	T_DWORD,
	T_STRING,
}PARAM_TYPE;

/*终端参数类型*/
typedef  struct
{
	uint8_t		id;
	PARAM_TYPE	type;
	void		* pvalue;
}PARAM;

/*终端参数类型*/
typedef __packed struct
{
	PARAM_TYPE	type;
	void		* pvalue;
}PARAM_BODY;
#endif
 

typedef enum
{
	IDLE = 1,                   /*空闲等待发送*/
	WAIT_ACK,                   /*等待ACK中*/
	ACK_OK,                     /*已收到ACK应答*/
	WAIT_DELETE,                /*等待删除*/
} JT808_MSG_STATE;


/*
   typedef enum
   {
   TERMINAL_CMD = 1,
   TERMINAL_ACK,
   CENTER_CMD,
   CENTER_ACK
   }JT808_MSG_TYPE;
 */

typedef enum
{
	SINGLE_CMD = 0,
	SINGLE_ACK,
	MULTI_CMD,
	MULTI_ACK
}JT808_MSG_TYPE;

typedef __packed struct
{
	uint16_t	id;
	uint16_t	attr;
	uint8_t		mobile[6];
	uint16_t	seq;
}JT808_MSG_HEAD;

typedef __packed struct
{
	uint16_t	id;
	uint16_t	attr;
	uint8_t		mobile[6];
	uint16_t	seq;
	uint16_t	packet_num;
	uint16_t	packet_no;
}JT808_MSG_HEAD_EX;

#define JT808HEAD_ID( head )	( ( *( head + 0 ) << 8 ) | ( *( head + 1 ) ) )
#define JT808HEAD_ATTR( head )	( ( *( head + 2 ) << 8 ) | ( *( head + 3 ) ) )
#define JT808HEAD_LEN( head )	( ( *( head + 2 ) << 8 ) | ( *( head + 3 ) ) ) & 0x3FF
#define JT808HEAD_SEQ( head )	( ( *( head + 10 ) << 8 ) | ( *( head + 11 ) ) )


typedef enum
{
	CONNECT_NONE	= 0,            /*不连接*/
	CONNECT_IDLE	= 1,            /*空闲，准备连接*/
	CONNECT_PEER,                   /*正在连接到对端*/
	CONNECTED,                      /*连接成功*/
	CONNECT_ERROR,                  /*连接错误*/
	CONNECT_CLOSE,                  /*连接关闭，区分是主动还是被动*/
}CONN_STATE;

struct _connect_state
{
	uint32_t	disable_connect;    /*禁止链接标志，协议控制 0:允许链接*/
	CONN_STATE	server_state;
	uint8_t		server_index;
	CONN_STATE	auth_state;
	uint8_t		auth_index;
};

extern struct _connect_state connect_state ;




/*
   存储jt808发送的相关信息
 */
#if 0
typedef __packed struct _jt808_tx_msg_nodedata_old
{
/*发送机制相关*/
	uint8_t			linkno;     /*传输使用的link,包括了协议和远端socket*/
	JT808_MSG_TYPE	type;       /*发送消息的类型*/
	JT808_MSG_STATE state;      /*发送状态*/
	uint32_t		retry;      /*重传次数,递增，递减找不到*/
	uint32_t		max_retry;  /*最大重传次数*/
	uint32_t		timeout;    /*超时时间*/
	uint32_t		tick;       /*发送时间*/
/*接收的处理判断相关*/
	void ( *cb_tx_timeout )( __packed struct _jt808_tx_msg_nodedata *pnodedata );
	void ( *cb_tx_response )( uint8_t linkno, uint8_t *pmsg );
	uint16_t	head_id;        /*消息ID*/
	uint16_t	head_sn;        /*消息流水号*/
/*真实的发送数据*/
	uint16_t	msg_len;        /*消息长度*/
	uint8_t		*pmsg;          /*发送消息体,真实的要发送的数据格式，经过转义和FCS后的<7e>为标志*/
}JT808_TX_MSG_NODEDATA_OLD;
#endif

#if 0
typedef __packed struct _jt808_tx_nodedata
{
/*发送机制相关*/
	uint8_t			linkno;                                                 /*传输使用的link,包括了协议和远端socket*/
	uint8_t			multipacket;                                            /*是不是多包发送*/
	JT808_MSG_TYPE	type;                                                   /*发送消息的类型*/
	JT808_MSG_STATE state;                                                  /*发送状态*/
	uint32_t		retry;                                                  /*重传次数,递增，递减找不到*/
	uint32_t		max_retry;                                              /*最大重传次数*/
	uint32_t		timeout;                                                /*超时时间*/
	uint32_t		tick;                                                   /*发送时间*/
/*接收的处理判断相关*/
	void ( *cb_tx_timeout )( __packed struct _jt808_tx_nodedata * thiz );   /*发送超时的处理函数*/
	void ( *cb_tx_response )( uint8_t linkno, uint8_t *pmsg );              /*收到中心应答的处理函数*/
	uint16_t	head_id;                                                    /*消息ID*/
	uint16_t	head_sn;                                                    /*消息流水号*/

/*单包真实的发送数据-消息体*/
	uint16_t	msg_len;                                                    /*消息长度*/
	uint8_t		*pmsg;                                                      /*原始信息,需要在发送时转义,因为多包发送时得到的是原始信息。
	                                                                                                     ，包括808转义和M66的HEX转义，这样，减少RAM使用*/
/*多包发送的处理*/


	/*
	   提供一个void * 交由用户自己控制
	   uint8_t		stage;                                                                  //阶段
	   uint16_t	packet_num;                                                             //总包数
	   uint16_t	packet_no;                                                              //当前包数
	   uint32_t	size;                                                                   //总得数据大小
	   uint32_t	media_id;
	   uint16_t	seq;
	   提供一个发送列表数组，按序号表示发送的状态，同时便于将来补传
	   packet_num指定数组大小，packet_no指示当前要发送的ID
	 */
	void *user_data;
	int ( *get_data )( __packed struct _jt808_tx_nodedata * thiz );         /*获取要发送的信息*/
}JT808_TX_NODEDATA;
#endif

#if 0
typedef __packed struct _jt808_tx_nodedata
{
/*发送机制相关*/
	uint8_t			linkno;                                                                             /*传输使用的link,包括了协议和远端socket*/
	uint8_t			multipacket;                                                                        /*是不是多包发送*/
	JT808_MSG_TYPE	type;                                                                               /*发送消息的类型*/
	JT808_MSG_STATE state;                                                                              /*发送状态*/
	uint32_t		retry;                                                                              /*重传次数,递增，递减找不到*/
	uint32_t		max_retry;                                                                          /*最大重传次数*/
	uint32_t		timeout;                                                                            /*超时时间*/
	uint32_t		timeout_tick;                                                                       /*达到超时的tick值*/
	uint16_t		user_size;                                                                          /*分配给用户的大小*/
/*接收的处理判断相关*/
	JT808_MSG_STATE ( *cb_tx_timeout )( __packed struct _jt808_tx_nodedata * thiz );                    /*发送超时的处理函数*/
	JT808_MSG_STATE ( *cb_tx_response )( __packed struct _jt808_tx_nodedata * thiz, uint8_t *pmsg );    /*收到中心应答的处理函数*/
	uint16_t	head_id;                                                                                /*消息ID*/
	uint16_t	head_sn;                                                                                /*消息流水号*/
/*用户数据*/
	uint16_t	msg_len;                                                                                /*消息长度*/
	uint8_t		tag_data[1];                                                                            /*变长数组*/
}JT808_TX_NODEDATA;

#endif

typedef __packed struct _jt808_tx_nodedata
{
/*发送机制相关*/
	uint8_t			linkno;                                                                             /*传输使用的link,包括了协议和远端socket*/
	uint8_t			multipacket;                                                                        /*是不是多包发送*/
	JT808_MSG_TYPE	type;                                                                               /*发送消息的类型*/
	JT808_MSG_STATE state;                                                                              /*发送状态*/
	uint32_t		retry;                                                                              /*重传次数,递增，递减找不到*/
	uint32_t		max_retry;                                                                          /*最大重传次数*/
	uint32_t		timeout;                                                                            /*超时时间*/
	uint32_t		timeout_tick;                                                                       /*发送时间*/
/*接收的处理判断相关*/
	JT808_MSG_STATE ( *cb_tx_timeout )( __packed struct _jt808_tx_nodedata * thiz );                    /*发送超时的处理函数*/
	JT808_MSG_STATE ( *cb_tx_response )( __packed struct _jt808_tx_nodedata * thiz, uint8_t *pmsg );    /*收到中心应答的处理函数*/
	uint16_t	head_id;                                                                                /*消息ID*/
	uint16_t	head_sn;                                                                                /*消息流水号*/

	uint16_t	packet_num;                                                                             /*多包总包数*/
	uint16_t	packet_no;                                                                              /*多包当前包数*/
	uint32_t	size;                                                                                   /*消息体总得数据大小*/
	uint16_t	msg_len;                                                                                /*单包消息长度*/
/*多包发送的处理*/
	void	*user_para;                                                                                 /*cb_tx_response函数需要的关键原始数据参数，通过该参数和回调函数关联起来*/
	uint8_t tag_data[];                                                                                 /*指向数据的指针*/
}JT808_TX_NODEDATA;

extern uint8_t mobile[6];

rt_err_t gprs_rx( uint8_t linkno, uint8_t *pinfo, uint16_t length );


JT808_TX_NODEDATA * node_begin( uint8_t linkno,
                                JT808_MSG_TYPE fMultiPacket,    /*是否为多包*/
                                uint16_t id,
                                int32_t seq,
                                uint16_t datasize );


JT808_TX_NODEDATA * node_data( JT808_TX_NODEDATA * pnodedata,
                               uint8_t * pinfo, uint16_t len,
                               JT808_MSG_STATE ( *cb_tx_timeout )( ),
                               JT808_MSG_STATE ( *cb_tx_response )( ),
                               void  *userpara );

void node_end( JT808_TX_NODEDATA* pnodedata );


rt_err_t jt808_add_tx( uint8_t linkno,
                       JT808_MSG_TYPE fMultiPacket, /*是否为多包*/
                       uint16_t id,
                       int32_t seq,
                       JT808_MSG_STATE ( *cb_tx_timeout )( ),
                       JT808_MSG_STATE ( *cb_tx_response )( ),
                       uint16_t info_len,           /*信息长度*/
                       uint8_t * pinfo,
                       void  *userpara );

void jt808_add_tx_end( JT808_TX_NODEDATA* pnodedata );


#define jt808_tx( id, info, len ) jt808_add_tx( 1, SINGLE_CMD, id, -1, RT_NULL, RT_NULL, len, info, RT_NULL )

#define jt808_tx_ack( id, info, len ) jt808_add_tx( 1, SINGLE_ACK, id, -1, RT_NULL, RT_NULL, len, info, RT_NULL )

#endif

/************************************** The End Of File **************************************/

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
#ifndef _H_JT808_H_
#define _H_JT808_H_

#include <stm32f4xx.h>
#include <rtthread.h>

#include "msglist.h"
#include "m66.h"
#include "sst25.h"
#include "jt808_area.h"
#include "jt808_util.h"
#include "jt808_vehicle.h"

#define NEED_TODO	0



#define   MsgQ_Timeout		3
#define JT808_PACKAGE_MAX	512




/*for new use*/

#define FLASH_SEM_DELAY	5




typedef struct
{
	int		id;
	short	attr;
	int		latitute;   /*�Զ�λ��λ��γ��ֵ����10��6�η�����ȷ�������֮һ��*/
	int		longitute;
	int		radius;     /*��λΪ��m��·��Ϊ�ùյ㵽��һ�յ�*/
	char	start[6];
	char	end[6];
	short	speed;
	char	interval;   /*����ʱ��,��*/
}GPS_AREA_CIRCLE;

 

typedef enum
{
	IDLE = 1,                   /*���еȴ�����*/
	WAIT_ACK,                   /*�ȴ�ACK��*/
	ACK_OK,                     /*���յ�ACKӦ��*/
	WAIT_DELETE,                /*�ȴ�ɾ��*/
} JT808_MSG_STATE;


typedef enum
{
	SINGLE_FIRST=0,
	SINGLE_CMD = 1,
	SINGLE_ACK=2,
	MULTI_CMD=3,
	MULTI_ACK=4
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
	CONNECT_NONE	= 0,            /*������*/
	CONNECT_IDLE	= 1,            /*���У�׼������*/
	CONNECT_PEER,                   /*�������ӵ��Զ�*/
	CONNECTED,                      /*���ӳɹ�*/
	CONNECT_ERROR,                  /*���Ӵ���*/
	CONNECT_CLOSE,                  /*���ӹرգ��������������Ǳ���*/
}CONN_STATE;

struct _connect_state
{
	uint32_t	disable_connect;    /*��ֹ���ӱ�־��Э����� 0:��������*/
	CONN_STATE	server_state;
	uint8_t		server_index;
	CONN_STATE	auth_state;
	uint8_t		auth_index;
};

extern struct _connect_state connect_state ;




/*
   �洢jt808���͵������Ϣ
 */

typedef __packed struct _jt808_tx_nodedata
{
/*���ͻ������*/
	uint8_t			linkno;                                                                             /*����ʹ�õ�link,������Э���Զ��socket*/
//	uint8_t			multipacket;                                                                        /*�ǲ��Ƕ������*/
	JT808_MSG_TYPE	type;                                                                               /*������Ϣ������*/
	JT808_MSG_STATE state;                                                                              /*����״̬*/
	uint32_t		retry;                                                                              /*�ش�����,�������ݼ��Ҳ���*/
	uint32_t		max_retry;                                                                          /*����ش�����*/
	uint32_t		timeout;                                                                            /*��ʱʱ��*/
	uint32_t		timeout_tick;                                                                       /*����ʱ��*/
/*���յĴ����ж����*/
	JT808_MSG_STATE ( *cb_tx_timeout )( __packed struct _jt808_tx_nodedata * thiz );                    /*���ͳ�ʱ�Ĵ�����*/
	JT808_MSG_STATE ( *cb_tx_response )( __packed struct _jt808_tx_nodedata * thiz, uint8_t *pmsg );    /*�յ�����Ӧ��Ĵ�����*/
	uint16_t	head_id;                                                                                /*��ϢID*/
	uint16_t	head_sn;                                                                                /*��Ϣ��ˮ��*/

	uint16_t	packet_num;                                                                             /*����ܰ���*/
	uint16_t	packet_no;                                                                              /*�����ǰ����*/
	uint32_t	size;                                                                                   /*��Ϣ���ܵ����ݴ�С*/
	uint16_t	msg_len;                                                                                /*������Ϣ����*/
/*������͵Ĵ���*/
	void	*user_para;                                                                                 /*cb_tx_response������Ҫ�Ĺؼ�ԭʼ���ݲ�����ͨ���ò����ͻص�������������*/
	uint8_t tag_data[];                                                                                 /*ָ�����ݵ�ָ��*/
}JT808_TX_NODEDATA;

void jt808_init( void );
rt_err_t gprs_rx( uint8_t linkno, uint8_t *pinfo, uint16_t length );
rt_size_t tts_write( char* info,uint16_t len );
rt_size_t at( char *sinfo );



JT808_TX_NODEDATA * node_begin( uint8_t linkno,
                                JT808_MSG_TYPE fMultiPacket,    /*�Ƿ�Ϊ���*/
                                uint16_t id,
                                int32_t seq,
                                uint16_t datasize );


JT808_TX_NODEDATA * node_data( JT808_TX_NODEDATA * pnodedata,
                               uint8_t * pinfo, uint16_t len);

void node_end( JT808_TX_NODEDATA* pnodedata,
               JT808_MSG_STATE ( *cb_tx_timeout )( ),
               JT808_MSG_STATE ( *cb_tx_response )( ),
               void  *userpara );


void jt808_add_tx( uint8_t linkno,
                       JT808_MSG_TYPE msgtype, /*�Ƿ�Ϊ���*/
                       uint16_t id,
                       int32_t seq,
                       JT808_MSG_STATE ( *cb_tx_timeout )( ),
                       JT808_MSG_STATE ( *cb_tx_response )( ),
                       uint16_t info_len,           /*��Ϣ����*/
                       uint8_t * pinfo,
                       void  *userpara );


/*ͨ��Ӧ��*/
rt_err_t jt808_tx_0x0001(uint16_t seq, uint16_t id, uint8_t res );


#define jt808_tx( id, info, len ) jt808_add_tx( 1, SINGLE_CMD, id, -1, RT_NULL, RT_NULL, len, info, RT_NULL )

/*ר��Ӧ��*/
#define jt808_tx_ack( id, info, len ) jt808_add_tx( 1, SINGLE_ACK, id, -1, RT_NULL, RT_NULL, len, info, RT_NULL )


#endif

/************************************** The End Of File **************************************/

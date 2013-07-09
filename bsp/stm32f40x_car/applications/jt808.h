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

#define NEED_TODO	0



#define   MsgQ_Timeout		3
#define JT808_PACKAGE_MAX	512


/*
   �洢�������,���þ��Ե�ַ,��4K(0x1000)Ϊһ������
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
	int		latitute;   /*�Զ�λ��λ��γ��ֵ����10��6�η�����ȷ�������֮һ��*/
	int		longitute;
	int		radius;     /*��λΪ��m��·��Ϊ�ùյ㵽��һ�յ�*/
	char	start[6];
	char	end[6];
	short	speed;
	char	interval;   /*����ʱ��,��*/
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

/*�ն˲�������*/
typedef  struct
{
	uint8_t		id;
	PARAM_TYPE	type;
	void		* pvalue;
}PARAM;

/*�ն˲�������*/
typedef __packed struct
{
	PARAM_TYPE	type;
	void		* pvalue;
}PARAM_BODY;
#endif
 

typedef enum
{
	IDLE = 1,                   /*���еȴ�����*/
	WAIT_ACK,                   /*�ȴ�ACK��*/
	ACK_OK,                     /*���յ�ACKӦ��*/
	WAIT_DELETE,                /*�ȴ�ɾ��*/
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
#if 0
typedef __packed struct _jt808_tx_msg_nodedata_old
{
/*���ͻ������*/
	uint8_t			linkno;     /*����ʹ�õ�link,������Э���Զ��socket*/
	JT808_MSG_TYPE	type;       /*������Ϣ������*/
	JT808_MSG_STATE state;      /*����״̬*/
	uint32_t		retry;      /*�ش�����,�������ݼ��Ҳ���*/
	uint32_t		max_retry;  /*����ش�����*/
	uint32_t		timeout;    /*��ʱʱ��*/
	uint32_t		tick;       /*����ʱ��*/
/*���յĴ����ж����*/
	void ( *cb_tx_timeout )( __packed struct _jt808_tx_msg_nodedata *pnodedata );
	void ( *cb_tx_response )( uint8_t linkno, uint8_t *pmsg );
	uint16_t	head_id;        /*��ϢID*/
	uint16_t	head_sn;        /*��Ϣ��ˮ��*/
/*��ʵ�ķ�������*/
	uint16_t	msg_len;        /*��Ϣ����*/
	uint8_t		*pmsg;          /*������Ϣ��,��ʵ��Ҫ���͵����ݸ�ʽ������ת���FCS���<7e>Ϊ��־*/
}JT808_TX_MSG_NODEDATA_OLD;
#endif

#if 0
typedef __packed struct _jt808_tx_nodedata
{
/*���ͻ������*/
	uint8_t			linkno;                                                 /*����ʹ�õ�link,������Э���Զ��socket*/
	uint8_t			multipacket;                                            /*�ǲ��Ƕ������*/
	JT808_MSG_TYPE	type;                                                   /*������Ϣ������*/
	JT808_MSG_STATE state;                                                  /*����״̬*/
	uint32_t		retry;                                                  /*�ش�����,�������ݼ��Ҳ���*/
	uint32_t		max_retry;                                              /*����ش�����*/
	uint32_t		timeout;                                                /*��ʱʱ��*/
	uint32_t		tick;                                                   /*����ʱ��*/
/*���յĴ����ж����*/
	void ( *cb_tx_timeout )( __packed struct _jt808_tx_nodedata * thiz );   /*���ͳ�ʱ�Ĵ�����*/
	void ( *cb_tx_response )( uint8_t linkno, uint8_t *pmsg );              /*�յ�����Ӧ��Ĵ�����*/
	uint16_t	head_id;                                                    /*��ϢID*/
	uint16_t	head_sn;                                                    /*��Ϣ��ˮ��*/

/*������ʵ�ķ�������-��Ϣ��*/
	uint16_t	msg_len;                                                    /*��Ϣ����*/
	uint8_t		*pmsg;                                                      /*ԭʼ��Ϣ,��Ҫ�ڷ���ʱת��,��Ϊ�������ʱ�õ�����ԭʼ��Ϣ��
	                                                                                                     ������808ת���M66��HEXת�壬����������RAMʹ��*/
/*������͵Ĵ���*/


	/*
	   �ṩһ��void * �����û��Լ�����
	   uint8_t		stage;                                                                  //�׶�
	   uint16_t	packet_num;                                                             //�ܰ���
	   uint16_t	packet_no;                                                              //��ǰ����
	   uint32_t	size;                                                                   //�ܵ����ݴ�С
	   uint32_t	media_id;
	   uint16_t	seq;
	   �ṩһ�������б����飬����ű�ʾ���͵�״̬��ͬʱ���ڽ�������
	   packet_numָ�������С��packet_noָʾ��ǰҪ���͵�ID
	 */
	void *user_data;
	int ( *get_data )( __packed struct _jt808_tx_nodedata * thiz );         /*��ȡҪ���͵���Ϣ*/
}JT808_TX_NODEDATA;
#endif

#if 0
typedef __packed struct _jt808_tx_nodedata
{
/*���ͻ������*/
	uint8_t			linkno;                                                                             /*����ʹ�õ�link,������Э���Զ��socket*/
	uint8_t			multipacket;                                                                        /*�ǲ��Ƕ������*/
	JT808_MSG_TYPE	type;                                                                               /*������Ϣ������*/
	JT808_MSG_STATE state;                                                                              /*����״̬*/
	uint32_t		retry;                                                                              /*�ش�����,�������ݼ��Ҳ���*/
	uint32_t		max_retry;                                                                          /*����ش�����*/
	uint32_t		timeout;                                                                            /*��ʱʱ��*/
	uint32_t		timeout_tick;                                                                       /*�ﵽ��ʱ��tickֵ*/
	uint16_t		user_size;                                                                          /*������û��Ĵ�С*/
/*���յĴ����ж����*/
	JT808_MSG_STATE ( *cb_tx_timeout )( __packed struct _jt808_tx_nodedata * thiz );                    /*���ͳ�ʱ�Ĵ�����*/
	JT808_MSG_STATE ( *cb_tx_response )( __packed struct _jt808_tx_nodedata * thiz, uint8_t *pmsg );    /*�յ�����Ӧ��Ĵ�����*/
	uint16_t	head_id;                                                                                /*��ϢID*/
	uint16_t	head_sn;                                                                                /*��Ϣ��ˮ��*/
/*�û�����*/
	uint16_t	msg_len;                                                                                /*��Ϣ����*/
	uint8_t		tag_data[1];                                                                            /*�䳤����*/
}JT808_TX_NODEDATA;

#endif

typedef __packed struct _jt808_tx_nodedata
{
/*���ͻ������*/
	uint8_t			linkno;                                                                             /*����ʹ�õ�link,������Э���Զ��socket*/
	uint8_t			multipacket;                                                                        /*�ǲ��Ƕ������*/
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

extern uint8_t mobile[6];

rt_err_t gprs_rx( uint8_t linkno, uint8_t *pinfo, uint16_t length );


JT808_TX_NODEDATA * node_begin( uint8_t linkno,
                                JT808_MSG_TYPE fMultiPacket,    /*�Ƿ�Ϊ���*/
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
                       JT808_MSG_TYPE fMultiPacket, /*�Ƿ�Ϊ���*/
                       uint16_t id,
                       int32_t seq,
                       JT808_MSG_STATE ( *cb_tx_timeout )( ),
                       JT808_MSG_STATE ( *cb_tx_response )( ),
                       uint16_t info_len,           /*��Ϣ����*/
                       uint8_t * pinfo,
                       void  *userpara );

void jt808_add_tx_end( JT808_TX_NODEDATA* pnodedata );


#define jt808_tx( id, info, len ) jt808_add_tx( 1, SINGLE_CMD, id, -1, RT_NULL, RT_NULL, len, info, RT_NULL )

#define jt808_tx_ack( id, info, len ) jt808_add_tx( 1, SINGLE_ACK, id, -1, RT_NULL, RT_NULL, len, info, RT_NULL )

#endif

/************************************** The End Of File **************************************/

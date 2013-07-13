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

/*gprs�յ���Ϣ������*/
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
   AT�����ʹ�õ�mailbox
   �� VOICE TTS SMS TTSʹ��
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

static uint16_t				tx_seq = 0;             /*�������*/

static uint16_t				total_send_error = 0;   /*�ܵķ��ͳ����������ﵽһ���Ĵ���Ҫ����M66*/

/*������Ϣ�б�*/
MsgList* list_jt808_tx;

/*������Ϣ�б�*/
MsgList					* list_jt808_rx;

static rt_device_t		dev_gsm;

struct _connect_state	connect_state = { 0, CONNECT_IDLE, 0, CONNECT_NONE, 0 };

#if 0

T_SOCKET_STATE	socket_jt808_state	= SOCKET_IDLE;
uint16_t		socket_jt808_index	= 0;

T_SOCKET_STATE	socket_iccard_state = SOCKET_IDLE;
uint16_t		socket_iccard_index = 0;


/*
   ͬʱ׼���ÿ��õ��ĸ����ӣ�����Ҫ��ѡ����,����Ϊ
   ʵ���в�����ͬʱ�Զ�����ӽ�����ֻ�����η���������
   ��808������
   ����808������
   ��IC����Ȩ������
   ����IC����Ȩ������

 */
GSM_SOCKET gsm_socket[MAX_GSM_SOCKET];
#endif


/*
   ���ͺ��յ�Ӧ����
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
	switch( id )        // �ж϶�Ӧ�ն���Ϣ��ID�����ִ���
	{
		case 0x0200:    //	��Ӧλ����Ϣ��Ӧ��
			rt_kprintf( "\r\nCentre ACK!\r\n" );
			break;
		case 0x0002:    //	��������Ӧ��
			rt_kprintf( "\r\nCentre  Heart ACK!\r\n" );
			break;
		case 0x0101:    //	�ն�ע��Ӧ��
			break;
		case 0x0102:    //	�ն˼�Ȩ
			rt_kprintf( "\r\nCentre Auth ACK!\r\n" );
			jt808_state = REPORT;
			break;
		case 0x0800:    // ��ý���¼���Ϣ�ϴ�
			break;
		case 0x0702:
			rt_kprintf( "\r\n  ��ʻԱ��Ϣ�ϱ�---����Ӧ��!  \r\n" );
			break;
		case 0x0701:
			rt_kprintf( "\r\n	�����˵��ϱ�---����Ӧ��!  \r\n");
			break;
		default:
			rt_kprintf( "\r\nunknown id=%04x\r\n", id );
			break;
	}
	return ACK_OK;
}

/*
   ��Ϣ���ͳ�ʱ
 */

static JT808_MSG_STATE jt808_tx_timeout( JT808_TX_NODEDATA * nodedata )
{
	JT808_TX_NODEDATA* pnodedata = nodedata;
	pnodedata->retry++;
	if( pnodedata->retry > pnodedata->max_retry )
	{
		/*����,����*/
		return WAIT_DELETE;
	}
	return IDLE;                                                /*�ȴ��ٴη���*/
}

/*
   ����һ��ָ����С��node
 */
JT808_TX_NODEDATA * node_begin( uint8_t linkno,
                                JT808_MSG_TYPE fMultiPacket,    /*�Ƿ�Ϊ���*/
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
	//memset( pnodedata, 0, sizeof( JT808_TX_NODEDATA ) ); ///���Բ����٣�����ϵͳ����
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
   �������������,�γ���Ч������
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
	if( pnodedata->multipacket > SINGLE_ACK )   /*�������*/
	{
		pdata[2] += 0x20;
		memcpy( pdata + 16, pinfo, len );       /*����û�����*/
		pnodedata->msg_len = len + 16;
	} else
	{
		memcpy( pdata + 12, pinfo, len );       /*����û�����*/
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

/*�����������ݵ���Ϣ����*/
void node_datalen( JT808_TX_NODEDATA* pnodedata, uint16_t datalen )
{
	uint8_t* pdata_head = pnodedata->tag_data;

	pdata_head[2]		= datalen >> 8;
	pdata_head[3]		= datalen & 0xFF;
	pnodedata->msg_len	= datalen + 12;         /*ȱʡ�ǵ���*/
	if( pnodedata->multipacket > SINGLE_ACK )   /*�������*/
	{
		pdata_head[12]	= pnodedata->packet_num >> 8;
		pdata_head[13]	= pnodedata->packet_num & 0xFF;
		pnodedata->packet_no++;
		pdata_head[14]		= pnodedata->packet_no >> 8;
		pdata_head[15]		= pnodedata->packet_no & 0xFF;
		pdata_head[2]		|= 0x20;            /*�������*/
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

/*��ӵ������б�**/
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
                       JT808_MSG_TYPE fMultiPacket,         /*�Ƿ�Ϊ���*/
                       uint16_t id,
                       int32_t seq,
                       JT808_MSG_STATE ( *cb_tx_timeout )( ),
                       JT808_MSG_STATE ( *cb_tx_response )( ),
                       uint16_t len,                        /*��Ϣ����*/
                       uint8_t *pinfo,
                       void  *userpara )

{
	JT808_TX_NODEDATA* pnodedata;

	pnodedata = node_begin( linkno, fMultiPacket, id, seq, len );
	node_data( pnodedata, pinfo, len, cb_tx_timeout, cb_tx_response, userpara );
	msglist_append( list_jt808_tx, pnodedata );
}

/*
   �ն�ͨ��Ӧ��
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
   ƽ̨ͨ��Ӧ��,�յ���Ϣ��ֹͣ����
 */
static int handle_rx_0x8001( uint8_t linkno, uint8_t *pmsg )
{
	MsgListNode			* iter;
	JT808_TX_NODEDATA	* iterdata;

	uint16_t			id;
	uint16_t			seq;
	uint8_t				res;
/*������Ϣͷ12byte*/
	seq = ( *( pmsg + 12 ) << 8 ) | *( pmsg + 13 );
	id	= ( *( pmsg + 14 ) << 8 ) | *( pmsg + 15 );
	res = *( pmsg + 16 );

	/*��������*/
	iter		= list_jt808_tx->first;
	iterdata	= (JT808_TX_NODEDATA*)iter->data;
	if( ( iterdata->head_id == id ) && ( iterdata->head_sn == seq ) )
	{
		iterdata->cb_tx_response( iterdata, pmsg ); /*Ӧ������*/
		iterdata->state = ACK_OK;
	}
	return 1;
}

/*�����ְ�����*/
static int handle_rx_0x8003( uint8_t linkno, uint8_t *pmsg )
{
}

/* ������Ķ��ն�ע����Ϣ��Ӧ��*/
static int handle_rx_0x8100( uint8_t linkno, uint8_t *pmsg )
{
	MsgListNode			* iter;
	JT808_TX_NODEDATA	* iterdata;
	uint16_t			body_len; /*��Ϣ�峤��*/
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

/*�����ն˲���*/
static int handle_rx_0x8103( uint8_t linkno, uint8_t *pmsg )
{
	uint8_t		* p;
	uint8_t		res = 0;
	uint32_t	param_id;
	uint8_t		param_len;
	uint8_t		param_count;
	uint16_t	offset;
	uint16_t	seq, id;

	if( *( pmsg + 2 ) >= 0x20 ) /*����Ƕ�������ò���*/
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

	if( res ) /*�д���*/
	{
		jt808_tx_0x0001( seq, id, 1 );
	}else
	{
		jt808_tx_0x0001( seq, id, 0 );
		param_save( );
	}
	return 1;
}

/*��ѯȫ���ն˲������п��ܻᳬ����������ֽ�*/
static int handle_rx_0x8104( uint8_t linkno, uint8_t *pmsg )
{
	jt808_param_0x8104( pmsg );
	return 1;
}

/*�ն˿���*/
static int handle_rx_0x8105( uint8_t linkno, uint8_t *pmsg )
{
	uint8_t		cmd;
	uint8_t		* cmd_arg;
	uint16_t	seq = ( pmsg[10] << 8 ) | pmsg[11];

	cmd = *( pmsg + 12 );
	switch( cmd )
	{
		case 1:                         /*��������*/
			break;
		case 2:                         /*�ն˿�������ָ��������*/
			break;
		case 3:                         /*�ն˹ػ�*/
			break;
		case 4:                         /*�ն˸�λ*/
			break;
		case 5:                         /*�ָ���������*/
			break;
		case 6:                         /*�ر�����ͨѶ*/
			break;
		case 7:                         /*�ر���������ͨѶ*/
			break;
	}
	jt808_tx_0x0001( seq, 0x8105, 3 );  /*ֱ�ӷ��ز�֧��*/
	return 1;
}

/*��ѯָ���ն˲���,����Ӧ��0x0104*/
static int handle_rx_0x8106( uint8_t linkno, uint8_t *pmsg )
{
	jt808_param_0x8106( pmsg );
	return 1;
}

/*��ѯ�ն�����,Ӧ�� 0x0107*/
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
	memcpy( buf + 2, jt808_param.id_0xF000, 5 );    /*������ID*/
	memcpy( buf + 7, jt808_param.id_0xF001, 20 );   /*�ն��ͺ�*/
	memcpy( buf + 27, jt808_param.id_0xF002, 7 );   /*�ն�ID*/
	memcpy( buf + 34, "1234567890", 10 );           /*�ն�ICCID*/
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

/*λ����Ϣ��ѯ*/
static int handle_rx_0x8201( uint8_t linkno, uint8_t *pmsg )
{
	uint8_t buf[40];
	buf[0]	= pmsg[10];
	buf[1]	= pmsg[11];
	memcpy( buf + 2, (uint8_t*)&gps_baseinfo, 28 );
	jt808_tx_ack( 0x0201, buf, 30 );

	return 1;
}

/*��ʱλ�ø��ٿ���*/
static int handle_rx_0x8202( uint8_t linkno, uint8_t *pmsg )
{
	uint16_t interval;
	interval = ( pmsg[12] << 8 ) | pmsg[13];
	if( interval == 0 )
	{
		jt808_8202_track_duration = 0; /*ֹͣ����*/
	}else
	{
		jt808_8202_track_interval	= interval;
		jt808_8202_track_duration	= ( pmsg[14] << 24 ) | ( pmsg[15] << 16 ) | ( pmsg[16] << 8 ) | ( pmsg[17] );
	}
	return 1;
}

/*�˹�ȷ�ϱ�����Ϣ*/
static int handle_rx_0x8203( uint8_t linkno, uint8_t *pmsg )
{
	jt808_8203_manual_ack_seq	= ( pmsg[12] << 8 ) | pmsg[13];
	jt808_8203_manual_ack_value = ( pmsg[14] << 24 ) | ( pmsg[15] << 16 ) | ( pmsg[16] << 8 ) | ( pmsg[17] );
	return 1;
}

/*�ı���Ϣ�·�*/
static int handle_rx_0x8300( uint8_t linkno, uint8_t *pmsg )
{
	jt808_misc_0x8300( pmsg );
	return 1;
}

/*�¼�����*/
static int handle_rx_0x8301( uint8_t linkno, uint8_t *pmsg )
{
	jt808_misc_0x8301( pmsg );
	return 1;
}

/*�����·�*/
static int handle_rx_0x8302( uint8_t linkno, uint8_t *pmsg )
{
	jt808_misc_0x8302( pmsg );
	return 1;
}

/*��Ϣ�㲥�˵�����*/
static int handle_rx_0x8303( uint8_t linkno, uint8_t *pmsg )
{
	jt808_misc_0x8303( pmsg );
	return 1;
}

/*��Ϣ����*/
static int handle_rx_0x8304( uint8_t linkno, uint8_t *pmsg )
{
	jt808_misc_0x8304( pmsg );

	return 1;
}

/*�绰�ز�*/
static int handle_rx_0x8400( uint8_t linkno, uint8_t *pmsg )
{
	jt808_misc_0x8400( pmsg );
	return 1;
}

/*���õ绰��*/
static int handle_rx_0x8401( uint8_t linkno, uint8_t *pmsg )
{
	jt808_misc_0x8401( pmsg );
	return 1;
}

/*��������*/
static int handle_rx_0x8500( uint8_t linkno, uint8_t *pmsg )
{
	jt808_misc_0x8500( pmsg );

	return 1;
}

/*����Բ������*/
static int handle_rx_0x8600( uint8_t linkno, uint8_t *pmsg )
{
	area_jt808_0x8600( linkno, pmsg );
	return 1;
}

/*ɾ��Բ������*/
static int handle_rx_0x8601( uint8_t linkno, uint8_t *pmsg )
{
	area_jt808_0x8601( linkno, pmsg );

	return 1;
}

/*���þ�������*/
static int handle_rx_0x8602( uint8_t linkno, uint8_t *pmsg )
{
	area_jt808_0x8602( linkno, pmsg );

	return 1;
}

/*ɾ����������*/
static int handle_rx_0x8603( uint8_t linkno, uint8_t *pmsg )
{
	area_jt808_0x8603( linkno, pmsg );

	return 1;
}

/*���ö��������*/
static int handle_rx_0x8604( uint8_t linkno, uint8_t *pmsg )
{
	area_jt808_0x8604( linkno, pmsg );

	return 1;
}

/*ɾ�����������*/
static int handle_rx_0x8605( uint8_t linkno, uint8_t *pmsg )
{
	area_jt808_0x8605( linkno, pmsg );

	return 1;
}

/*����·��*/
static int handle_rx_0x8606( uint8_t linkno, uint8_t *pmsg )
{
	area_jt808_0x8606( linkno, pmsg );

	return 1;
}

/*ɾ��·��*/
static int handle_rx_0x8607( uint8_t linkno, uint8_t *pmsg )
{
	area_jt808_0x8607( linkno, pmsg );

	return 1;
}

/*��ʻ��¼�����ݲɼ�*/

static int handle_rx_0x8700( uint8_t linkno, uint8_t *pmsg )
{
	vdr_rx_8700( pmsg );
	return 1;
}

/*��ʻ��¼�ǲ����´�*/
static int handle_rx_0x8701( uint8_t linkno, uint8_t *pmsg )
{
	vdr_rx_8701( pmsg );
	return 1;
}

/*
   ��ý�������ϴ�Ӧ��
   ���в�ͬ����Ϣͨ���˽ӿ�
 */
static int handle_rx_0x8800( uint8_t linkno, uint8_t *pmsg )
{
	MsgListNode			* iter;
	JT808_TX_NODEDATA	* iterdata;
	uint32_t			media_id;

	/*������Ϣͷ12byte*/
	media_id	= ( pmsg[12] << 24 ) | ( pmsg[13] << 16 ) | ( pmsg[14] << 8 ) | ( pmsg[15] );
	iter		= list_jt808_tx->first;
	while( iter != RT_NULL )
	{
		iterdata = (JT808_TX_NODEDATA*)iter->data;
		if( iterdata->head_id == media_id )
		{
			iterdata->cb_tx_response( iterdata, pmsg ); /*Ӧ������*/
			iterdata->state = ACK_OK;
			break;
		}else
		{
			iter = iter->next;
		}
	}

	return 1;
}

/*����ͷ������������*/
static int handle_rx_0x8801( uint8_t linkno, uint8_t *pmsg )
{
	Cam_jt808_0x8801( linkno, pmsg );
	return 1;
}

/*
   ��ý����Ϣ����
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

/*¼����ʼ*/
static int handle_rx_0x8804( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/*�����洢��ý�����ݼ����ϴ�*/
static int handle_rx_0x8805( uint8_t linkno, uint8_t *pmsg )
{
	Cam_jt808_0x8805( linkno, pmsg );
	return 1;
}

/*��������͸��*/
static int handle_rx_0x8900( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/*ƽ̨RSA��Կ*/
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
	DECL_JT808_RX_HANDLE( 0x8001 ), //	ͨ��Ӧ��
	DECL_JT808_RX_HANDLE( 0x8003 ), //	�����ְ�����
	DECL_JT808_RX_HANDLE( 0x8100 ), //  ������Ķ��ն�ע����Ϣ��Ӧ��
	DECL_JT808_RX_HANDLE( 0x8103 ), //	�����ն˲���
	DECL_JT808_RX_HANDLE( 0x8104 ), //	��ѯ�ն˲���
	DECL_JT808_RX_HANDLE( 0x8105 ), // �ն˿���
	DECL_JT808_RX_HANDLE( 0x8106 ), // ��ѯָ���ն˲���
	DECL_JT808_RX_HANDLE( 0x8106 ), /*��ѯ�ն�����,Ӧ�� 0x0107*/
	DECL_JT808_RX_HANDLE( 0x8201 ), // λ����Ϣ��ѯ    λ����Ϣ��ѯ��Ϣ��Ϊ��
	DECL_JT808_RX_HANDLE( 0x8202 ), // ��ʱλ�ø��ٿ���
	DECL_JT808_RX_HANDLE( 0x8203 ), /*�˹�ȷ�ϱ�����Ϣ*/
	DECL_JT808_RX_HANDLE( 0x8300 ), //	�ı���Ϣ�·�
	DECL_JT808_RX_HANDLE( 0x8301 ), //	�¼�����
	DECL_JT808_RX_HANDLE( 0x8302 ), // �����·�
	DECL_JT808_RX_HANDLE( 0x8303 ), //	��Ϣ�㲥�˵�����
	DECL_JT808_RX_HANDLE( 0x8304 ), //	��Ϣ����
	DECL_JT808_RX_HANDLE( 0x8400 ), //	�绰�ز�
	DECL_JT808_RX_HANDLE( 0x8401 ), //	���õ绰��
	DECL_JT808_RX_HANDLE( 0x8500 ), //	��������
	DECL_JT808_RX_HANDLE( 0x8600 ), //	����Բ������
	DECL_JT808_RX_HANDLE( 0x8601 ), //	ɾ��Բ������
	DECL_JT808_RX_HANDLE( 0x8602 ), //	���þ�������
	DECL_JT808_RX_HANDLE( 0x8603 ), //	ɾ����������
	DECL_JT808_RX_HANDLE( 0x8604 ), //	���������
	DECL_JT808_RX_HANDLE( 0x8605 ), //	ɾ���������
	DECL_JT808_RX_HANDLE( 0x8606 ), //	����·��
	DECL_JT808_RX_HANDLE( 0x8607 ), //	ɾ��·��
	DECL_JT808_RX_HANDLE( 0x8700 ), //	�г���¼�����ݲɼ�����
	DECL_JT808_RX_HANDLE( 0x8701 ), //	��ʻ��¼�ǲ����´�����
	DECL_JT808_RX_HANDLE( 0x8800 ), //	��ý�������ϴ�Ӧ��
	DECL_JT808_RX_HANDLE( 0x8801 ), //	����ͷ��������
	DECL_JT808_RX_HANDLE( 0x8802 ), //	�洢��ý�����ݼ���
	DECL_JT808_RX_HANDLE( 0x8803 ), //	�洢��ý�������ϴ�����
	DECL_JT808_RX_HANDLE( 0x8804 ), //	¼����ʼ����
	DECL_JT808_RX_HANDLE( 0x8805 ), //	�����洢��ý�����ݼ����ϴ����� ---- ����Э��Ҫ��
	DECL_JT808_RX_HANDLE( 0x8900 ), //	��������͸��
	DECL_JT808_RX_HANDLE( 0x8A00 ), //	ƽ̨RSA��Կ
};


/*jt808��socket����

   ά����·�����в�ͬ��ԭ��
   �ϱ�״̬��ά��
   1.��δ����
   2.�������ӣ�DNS,��ʱ��Ӧ��
   3.��ֹ�ϱ����ر�ģ�������
   4.��ǰ���ڽ��п��и��£���ý���ϱ��Ȳ���Ҫ��ϵĹ���

 */


/*
   ����ص���Ҫ��M66����·�ض�ʱ��֪ͨ��socket����һ���������߳�
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
   ���մ���
   ����jt808��ʽ������
   <linkno><����2byte><��ʶ0x7e><��Ϣͷ><��Ϣ��><У����><��ʶ0x7e>

   20130625 ����ճ�������

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
	uint8_t		fstuff = 0;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          /*�Ƿ��ֽ����*/

	linkno		= pinfo [0];
	total_len	= ( pinfo [1] << 8 ) | pinfo [2];

	psrc	= pinfo + 3;
	pdst	= pinfo + 3;

	count = 0;

/*����ճ��*/
	while( total_len )
	{
		if( *psrc == 0x7e )         /*��ͷ��β��־*/
		{
			if( count )             /*������*/
			{
				if( fcs == 0 )      /*������ȷ*/
				{
					*psrc	= 0;    /*20120711 ��Ϊ�ַ���������־*/
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
			pdata	= psrc;         /*ָ������ͷ,0x7E��λ��*/
			pdst	= psrc;
		}else if( *psrc == 0x7d )   /*��ת���ַ��ȴ�������һ��*/
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
   ����ÿ��Ҫ������Ϣ��״̬
   ���������д�����?

   2013.06.08���Ӷ�����ʹ���
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

	if( pnodedata->state == IDLE )                      /*���У�������Ϣ��ʱ��û������*/
	{
		if( pnodedata->retry >= jt808_param.id_0x0003 ) /*����������ش�����*/                                                                     /*�Ѿ��ﵽ���Դ���*/
		{
			/*��ʾ����ʧ��*/
			pnodedata->cb_tx_timeout( pnodedata );      /*���÷���ʧ�ܴ�����*/
			return MSGLIST_RET_DELETE_NODE;
		}
		/*Ҫ�ж��ǲ��ǳ���GSM_TCPIP״̬,��ǰsocket�Ƿ����*/
		if( gsmstate( GSM_STATE_GET ) != GSM_TCPIP )
		{
			return MSGLIST_RET_OK;
		}
		if( connect_state.server_state != CONNECTED )
		{
			return MSGLIST_RET_OK;
		}

		ret = socket_write( pnodedata->linkno, pnodedata->tag_data, pnodedata->msg_len );
		if( ret == RT_EOK ) /*���ͳɹ��ȴ�����Ӧ����*/
		{
			pnodedata->tick = rt_tick_get( );
			pnodedata->retry++;
			pnodedata->timeout	= pnodedata->retry * jt808_param.id_0x0002 * RT_TICK_PER_SECOND;
			pnodedata->state	= WAIT_ACK;
			rt_kprintf( "send retry=%d,timeout=%d\r\n", pnodedata->retry, pnodedata->timeout * 10 );
		}else /*��������û�еȵ�ģ�鷵�ص�OK�������ط������ǵ�һ��ʱ���ٷ�*/
		{
			pnodedata->retry++;         /*��λ�ٴη���*/
			total_send_error++;
			rt_kprintf( "total_send_error=%d\r\n", total_send_error );
		}
		return MSGLIST_RET_OK;
	}

	if( pnodedata->state == WAIT_ACK )  /*�������Ӧ���Ƿ�ʱ*/
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

	if( pnodedata->state == IDLE ) /*���У�������Ϣ��ʱ��û������*/
	{
#if 1
		/*Ҫ�ж��ǲ��ǳ���GSM_TCPIP״̬,��ǰsocket�Ƿ����*/
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
		if( ret != RT_EOK )                                                                             /*��������û�еȵ�ģ�鷵�ص�OK�������ط������ǵ�һ��ʱ���ٷ�*/
		{
			total_send_error++;
			if( total_send_error++ > 3 )                                                                /*m66 ����ʧ��,��δ���*/
			{
				gsmstate( GSM_POWEROFF );
			}
			rt_kprintf( "total_send_error=%d\r\n", total_send_error );
		}

		pnodedata->timeout_tick = rt_tick_get( ) + ( pnodedata->retry + 1 ) * pnodedata->timeout - 30;  /*��30��Ϊ������*/
		pnodedata->state		= WAIT_ACK;
		rt_kprintf( "%d>send retry=%d,timeout=%d\r\n", rt_tick_get( ), pnodedata->retry, pnodedata->timeout * 10 );
		return IDLE;
	}

	if( pnodedata->state == WAIT_ACK )                                                                  /*�������Ӧ���Ƿ�ʱ*/
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
   808���Ӵ���
   ����gsm״̬Ǩ�Ƶ�,����Ѳ��״̬����ģ�ͣ�
   �ǲ��ǴӶ����׵�״̬��������Щ��

   jt808_state   ע�ᣬ��Ȩ �������ϱ���ͣ��
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
   ����Ƿ�����gsm����
   �м��ʱ��ر�����
 */
	if( connect_state.disable_connect )
	{
		return;
	}

/*���GSM״̬*/
	state = gsmstate( GSM_STATE_GET );
	if( state == GSM_IDLE )
	{
		gsmstate( GSM_POWERON );                /*��������*/
		return;
	}
/*���Ƶ���*/
	if( state == GSM_AT )                       /*����Ҫ�ж����Ǹ�apn user psw ����*/
	{
		if( connect_state.server_index % 2 )    /*�ñ��÷�����*/
		{
			ctl_gprs( jt808_param.id_0x0014, \
			          jt808_param.id_0x0015, \
			          jt808_param.id_0x0016, \
			          1 );
		}else /*����������*/
		{
			ctl_gprs( jt808_param.id_0x0010, \
			          jt808_param.id_0x0011, \
			          jt808_param.id_0x0012, \
			          1 );
		}
		return;
	}
/*���ƽ�������*/
	if( state == GSM_TCPIP )                                /*�Ѿ�������*/
	{
		if( connect_state.server_state == CONNECT_IDLE )
		{
			if( connect_state.server_index % 2 )            /*�����÷�����*/
			{
				ctl_socket( 1, 't', jt808_param.id_0x0017, jt808_param.id_0x0018, 1 );
			}else /*����������*/
			{
				ctl_socket( 1, 't', jt808_param.id_0x0013, jt808_param.id_0x0018, 1 );
			}
			connect_state.server_state = CONNECT_PEER;      /*��ʱgsm_state���� GSM_SOCKET_PROC������󷵻� GSM_TCPIP*/
			return;
		}

		if( connect_state.server_state == CONNECT_PEER )    /*�������ӵ�������*/
		{
			if( socketstate( SOCKET_STATE ) == SOCKET_READY )
			{
				connect_state.server_state = CONNECTED;
			}else /*û�����ӳɹ�,�л�������*/
			{
				connect_state.server_index++;
				connect_state.server_state = CONNECT_IDLE;
			}
		}

		if( connect_state.server_state == CONNECTED )               /*��·ά��������*/
		{
			/*�жϵ�ǰ�����Ƿ��쳣*/
			if( socketstate( SOCKET_STATE ) == CONNECT_CLOSE )      /*���ӱ��Ҷϣ��������Ҷϻ�������ԭ��*/
			{
				connect_state.server_state = CONNECT_IDLE;          /*������cb_socket_close���ж�*/
				return;
			}

			switch( jt808_state )
			{
				case REGISTER:
					buf[0]	= jt808_param.id_0x0081 >> 8;           /*ʡ��*/
					buf[1]	= jt808_param.id_0x0081 & 0xff;
					buf[2]	= jt808_param.id_0x0082 >> 8;           /*����*/
					buf[3]	= jt808_param.id_0x0082 & 0xff;
					memcpy( buf + 4, jt808_param.id_0xF000, 5 );    /*������ID*/
					memcpy( buf + 9, jt808_param.id_0xF001, 20 );   /*�ն��ͺ�*/
					memcpy( buf + 29, jt808_param.id_0xF002, 7 );   /*�ն�ID*/
					buf[36] = jt808_param.id_0xF004;
					strcpy( buf + 37, jt808_param.id_0xF005 );      /*������ʾ��VIN*/
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
						/*Ҫ����������*/
						if( ( rt_tick_get( ) - server_heartbeat_tick ) >= ( jt808_param.id_0x0001 * RT_TICK_PER_SECOND ) )
						{
						}
					}
					server_heartbeat_tick = rt_tick_get( ); /*�״��ñ�����ǰʱ��*/
					break;
			}

			return;                                         /*ֱ�ӷ��أ�����ICCARD*/
		}

		if( connect_state.server_state == CONNECT_CLOSE )   /*���ӹرգ������������Ǳ����ر�*/
		{
			connect_state.server_state = CONNECT_IDLE;      /*��������*/
		}

		/*����IC��������*/

		if( connect_state.auth_state == CONNECT_IDLE )      /*û������*/
		{
			if( connect_state.auth_index % 2 )              /*�����÷�����*/
			{
				ctl_socket( 2, 't', jt808_param.id_0x001A, jt808_param.id_0x001B, 1 );
			}else /*����������*/
			{
				ctl_socket( 2, 't', jt808_param.id_0x001D, jt808_param.id_0x001B, 1 );
			}
			connect_state.auth_state = CONNECT_PEER;
			return;
		}

		if( connect_state.auth_state == CONNECT_PEER ) /*�������ӵ�������*/
		{
			if( socketstate( 0 ) == SOCKET_READY )
			{
				connect_state.auth_state = CONNECTED;
			}else /*û�����ӳɹ�,�л�������*/
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
   tts���������Ĵ���

   ��ͨ��
   %TTS: 0 �ж�tts״̬(���ɲ�����ÿ�ζ������)
   ����AT%TTS? ��ѯ״̬
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
/*gsm�ڴ�����������*/
	oldstate = gsmstate( GSM_STATE_GET );
	if( oldstate != GSM_TCPIP )
	{
		if( oldstate != GSM_AT )
		{
			return;
		}
	}

/*�Ƿ�����ϢҪ����*/
	ret = rt_mb_recv( &mb_tts, (rt_uint32_t*)&pinfo, 0 );
	if( ret != RT_EOK )
	{
		return;
	}

	gsmstate( GSM_AT_SEND );

	GPIO_ResetBits( GPIOD, GPIO_Pin_9 ); /*������*/

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
/*���жϣ���gsmrx_cb�д���*/
	rt_free( pinfo );
	ret = gsm_send( "", RT_NULL, "%TTS: 0", RESP_TYPE_STR, RT_TICK_PER_SECOND * 35, 1 );
	GPIO_SetBits( GPIOD, GPIO_Pin_9 ); /*�ع���*/
	gsmstate( oldstate );
}

/*
   at������յ�OK��ʱ�˳�
 */
void jt808_at_tx_proc( void )
{
	rt_err_t	ret;
	rt_size_t	len;
	uint8_t		*pinfo, *p;
	uint8_t		c;
	T_GSM_STATE oldstate;

/*gsm�ڴ�����������*/
	oldstate = gsmstate( GSM_STATE_GET );
	if( oldstate != GSM_TCPIP )
	{
		if( oldstate != GSM_AT )
		{
			return;
		}
	}

/*�Ƿ�����ϢҪ����*/
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
   ����״̬ά��
   jt808Э�鴦��

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
/*��ʼ��������Ϣ*/
	if( strlen( jt808_param.id_0xF003 ) ) /*�Ƿ����м�Ȩ��*/
	{
		jt808_state = AUTH;
	}

	while( 1 )
	{
/*����gprs��Ϣ*/
		ret = rt_mb_recv( &mb_gprsrx, ( rt_uint32_t* )&pstr, 5 );
		if( ret == RT_EOK )
		{
			jt808_rx_proc( pstr );
			rt_free( pstr );
		}

		jt808_socket_proc( );   /*jt808 socket����*/

		jt808_tts_proc( );      /*tts����*/

		jt808_at_tx_proc( );    /*at�����*/

/*���Ŵ���*/
		SMS_Process( );

/*������Ϣ��������*/
		iter = list_jt808_tx->first;

		if( jt808_tx_proc( iter ) == WAIT_DELETE )  /*ɾ���ýڵ�*/
		{
			//rt_kprintf( "%d>%s,%d\r\n", rt_tick_get( ), __func__, __LINE__ );
			pnodedata = ( JT808_TX_NODEDATA* )( iter->data );
			rt_free( pnodedata->user_para );
			rt_free( pnodedata );                   /*ɾ���ڵ�����*/
			list_jt808_tx->first = iter->next;      /*ָ����һ��*/
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
  *��������:u8 BkpSram_write(u32 addr,u8 *data, u16 len)
  *��������:backup sram ����д��
  *��	��:	addr	:д��ĵ�ַ
   data	:д�������ָ��
   len		:д��ĳ���
  *��	��:	none
  *�� �� ֵ:u8	:	0:��ʾ����ʧ�ܣ�	1:��ʾ�����ɹ�
  *��	��:������
  *��������:2013-06-18
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
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
  *��������:u16 bkpSram_read(u32 addr,u8 *data, u16 len)
  *��������:backup sram ���ݶ�ȡ
  *��	��:	addr	:��ȡ�ĵ�ַ
   data	:��ȡ������ָ��
   len		:��ȡ�ĳ���
  *��	��:	none
  *�� �� ֵ:u16	:��ʾʵ�ʶ�ȡ�ĳ���
  *��	��:������
  *��������:2013-06-18
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
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

/*jt808�����̳߳�ʼ��*/
void jt808_init( void )
{
	/*��ȡ������������,���ʱ��Ӧ��û�в���flash��*/
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

/*gprs���մ���,�յ�����Ҫ���촦��*/
rt_err_t gprs_rx( uint8_t linkno, uint8_t * pinfo, uint16_t length )
{
	uint8_t * pmsg;
	pmsg = rt_malloc( length + 3 );                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            /*����������Ϣ*/
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
   �յ�tts��Ϣ������
   ����0:OK
    1:����RAM����
 */
rt_size_t tts_write( char* info, uint16_t len )
{
	uint8_t *pmsg;
	/*ֱ�ӷ��͵�Mailbox��,�ڲ�����*/
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
   ����AT����
   ��α�֤������,������ִ�У�����ȴ���ʱ��

 */
rt_size_t at( char *sinfo )
{
	uint8_t		*pmsg;
	uint16_t	count;
	count = strlen( sinfo );

	/*ֱ�ӷ��͵�Mailbox��,�ڲ�����*/
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
   �����豸
   �������ԭ��
 */
void reset( uint32_t reason )
{
/*û�з��͵�����Ҫ����*/

/*�ر�����*/

/*��־��¼ʱ������ԭ��*/

	rt_kprintf( "\r\n%08d reset>reason=%08x", rt_tick_get( ), reason );
/*ִ������*/
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


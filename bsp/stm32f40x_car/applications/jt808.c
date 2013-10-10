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

typedef enum
{
	STOP_REPORT = 0,                    /*ͣ��*/
	REGISTER,                           /*ע��*/
	AUTH,                               /*��Ȩ*/
	REPORT,                             /*�ϱ�*/
	WAIT                                /*�ȴ�*/
}JT808_STATE;

JT808_STATE		jt808_state = REGISTER;

static uint16_t tx_seq = 0;             /*�������*/

static uint16_t total_send_error = 0;   /*�ܵķ��ͳ����������ﵽһ���Ĵ���Ҫ����M66*/

/*������Ϣ�б�*/
MsgList* list_jt808_tx;

/*������Ϣ�б�*/
MsgList				* list_jt808_rx;

static rt_tick_t	tick_server_heartbeat	= 0;
static rt_tick_t	tick_auth_heartbeat		= 0;


/*
   ���ͺ��յ�Ӧ����
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


	switch( ack_id )            // �ж϶�Ӧ�ն���Ϣ��ID�����ִ���
	{
		case 0x0002:            //	��������Ӧ��
			//rt_kprintf( "\nCentre  Heart ACK!\n" );
			break;
		case 0x0101:            //	�ն�ע��Ӧ��
			break;
		case 0x0102:            //	�ն˼�Ȩ
			if( ack_res == 0 )  /*�ɹ�*/
			{
				jt808_state = REPORT;
			} else
			{
				jt808_state = REGISTER;
			}
			break;
		case 0x0800: // ��ý���¼���Ϣ�ϴ�
			break;
		case 0x0702:
			//rt_kprintf( "\n��ʻԱ��Ϣ�ϱ�---����Ӧ��!" );
			break;
		case 0x0701:
			//rt_kprintf( "�����˵��ϱ�---����Ӧ��!" );
			break;
		default:
			//rt_kprintf( "\nunknown id=%04x", ack_id );
			break;
	}
	return ACK_OK;
}

/*
   ��Ϣ���ͳ�ʱ
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
   ����һ��ָ����С��node
 */
JT808_TX_NODEDATA * node_begin( uint8_t linkno,
                                JT808_MSG_TYPE msgtype,    /*�Ƿ�Ϊ���*/
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
	//rt_kprintf( "\n%d>����(id:%04x size:%d) %p", rt_tick_get( ), id, datasize,pnodedata );
	memset( pnodedata, 0, sizeof( JT808_TX_NODEDATA ) );    ///���Բ����٣�����ϵͳ����
	pnodedata->linkno	= linkno;
	pnodedata->state	= IDLE;
	pnodedata->head_id	= id;
	pnodedata->retry	= 0;
	pnodedata->type		= msgtype;
	if( msgtype > SINGLE_CMD )                              /*����͵���Ӧ��ֻ��һ��*/
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
   �������������,�γ���Ч������
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
	if( pnodedata->type >= MULTI_CMD )      /*�������*/
	{
		pdata[2] += 0x20;
		memcpy( pdata + 16, pinfo, len );   /*����û�����*/
		pnodedata->msg_len = len + 16;
	} else
	{
		memcpy( pdata + 12, pinfo, len );   /*����û�����*/
		pnodedata->msg_len = len + 12;
	}
	return pnodedata;
}

/*�����������ݵ���Ϣ����*/
void node_datalen( JT808_TX_NODEDATA* pnodedata, uint16_t datalen )
{
	uint8_t* pdata_head = pnodedata->tag_data;

	pdata_head[2]		= datalen >> 8;
	pdata_head[3]		= datalen & 0xFF;
	pnodedata->msg_len	= datalen + 12; /*ȱʡ�ǵ���*/
	if( pnodedata->type > SINGLE_ACK )  /*�������*/
	{
		pdata_head[12]	= pnodedata->packet_num >> 8;
		pdata_head[13]	= pnodedata->packet_num & 0xFF;
		pnodedata->packet_no++;
		pdata_head[14]		= pnodedata->packet_no >> 8;
		pdata_head[15]		= pnodedata->packet_no & 0xFF;
		pdata_head[2]		|= 0x20;    /*�������*/
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

/*��ӵ������б�**/
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
                   JT808_MSG_TYPE msgtype,              /*�Ƿ�Ϊ���*/
                   uint16_t id,
                   int32_t seq,
                   JT808_MSG_STATE ( *cb_tx_timeout )( ),
                   JT808_MSG_STATE ( *cb_tx_response )( ),
                   uint16_t len,                        /*��Ϣ����*/
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
   �ն�ͨ��Ӧ��
   ֻ����1�Σ������ɾ��
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
   ƽ̨ͨ��Ӧ��,�յ���Ϣ��ֹͣ����
 */
static int handle_rx_0x8001( uint8_t linkno, uint8_t *pmsg )
{
	MsgListNode			* iter;
	JT808_TX_NODEDATA	* iterdata;

	uint16_t	ack_id;
	uint16_t	ack_seq;
//	uint8_t		ack_res;
/*������Ϣͷ12byte*/
	ack_seq = ( *( pmsg + 12 ) << 8 ) | *( pmsg + 13 );
	ack_id	= ( *( pmsg + 14 ) << 8 ) | *( pmsg + 15 );
	rt_kprintf( "\n%d>ACK %04x:%04x:%d", rt_tick_get( ), ack_id, ack_seq, pmsg[16] );

	/*��������*/
	iter = list_jt808_tx->first;
	while( iter != RT_NULL )                                                /*���ӱ������п��ܲ���*/
	{
		iterdata = (JT808_TX_NODEDATA*)iter->data;
		if( ( iterdata->head_id == ack_id ) && ( iterdata->head_sn == ack_seq ) )
		{
			iterdata->state = iterdata->cb_tx_response( iterdata, pmsg );   /*Ӧ������*/
			return 1;
		}
		iter = iter->next;
	}
	return 1;
}

/*�����ְ�����*/
static int handle_rx_0x8003( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
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

/*�����ն˲���*/
static int handle_rx_0x8103( uint8_t linkno, uint8_t *pmsg )
{
	jt808_param_0x8103( pmsg );
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
#if 0
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
#endif
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
	jt808_tx_ack( 0x0107, buf, ( 46 + len1 + len2 + 2 ) );
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
		jt808_8202_track_counter	= 0;
	}
	jt808_tx_0x0001( ( pmsg[10] << 8 ) | pmsg[11], 0x8202, 0 );
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
//	uint32_t			media_id;

	/*������Ϣͷ12byte*/

	iter = list_jt808_tx->first;
	while( iter != RT_NULL )
	{
		iterdata = (JT808_TX_NODEDATA*)iter->data;
		if( iterdata->head_id == 0x0801 )
		{
			iterdata->cb_tx_response( iterdata, pmsg ); /*Ӧ������*/
			return 1;
		}
		iter = iter->next;
	}
#if 0
	media_id = ( pmsg[12] << 24 ) | ( pmsg[13] << 16 ) | ( pmsg[14] << 8 ) | ( pmsg[15] );

	while( iter != RT_NULL )
	{
		iterdata = (JT808_TX_NODEDATA*)iter->data;
		if( iterdata->head_id == media_id )             /*���ﲻ��*/
		{
			iterdata->cb_tx_response( iterdata, pmsg ); /*Ӧ������*/
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
	rt_kprintf( "\nunknown!\n" );
	return 1;
}

#define DECL_JT808_RX_HANDLE( a ) { a, handle_rx_ ## a }
//#define DECL_JT808_TX_HANDLE( a )	{ a, handle_jt808_tx_ ## a }

HANDLE_JT808_RX_MSG handle_rx_msg[] =
{
	DECL_JT808_RX_HANDLE( 0x8001 ), //	ͨ��Ӧ��
	DECL_JT808_RX_HANDLE( 0x8003 ), //	�����ְ�����
	DECL_JT808_RX_HANDLE( 0x8100 ), //  ������Ķ��ն�ע����Ϣ��Ӧ��
	DECL_JT808_RX_HANDLE( 0x8103 ), //	�����ն˲���
	DECL_JT808_RX_HANDLE( 0x8104 ), //	��ѯ�ն˲���
	DECL_JT808_RX_HANDLE( 0x8105 ), // �ն˿���
	DECL_JT808_RX_HANDLE( 0x8106 ), // ��ѯָ���ն˲���
	DECL_JT808_RX_HANDLE( 0x8107 ), /*��ѯ�ն�����,Ӧ�� 0x0107*/
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
   1.2.3.5.6
 */
void cb_socket_close( uint8_t cid )
{
	//gsm_socket[linkno-1].state = cid;
	//rt_kprintf( "\n%d>linkno %id close:%d", rt_tick_get( ),linkno, cid );
	if( cid == 1 )
	{
		if( gsm_socket[0].state == CONNECT_CLOSED ) /*�����رյ�*/
		{
			gsm_socket[0].state = CONNECT_NONE;
		}else
		{
			gsm_socket[0].index++;
			gsm_socket[0].state = CONNECT_IDLE;     /*������cb_socket_close���ж�*/
			jt808_state			= AUTH;             /*��������Ҫ���¼�Ȩ*/
		}
	}
	if( cid == 2 )
	{
		if( gsm_socket[1].state == CONNECT_CLOSED ) /*�����رյ�*/
		{
			gsm_socket[1].state = CONNECT_NONE;
		}else
		{
			gsm_socket[1].state = CONNECT_IDLE;     /*������cb_socket_close���ж�*/
		}
	}
	if( cid == 3 )
	{
		if( gsm_socket[2].state == CONNECT_CLOSED ) /*�����رյ�*/
		{
			gsm_socket[2].state = CONNECT_NONE;
		}else
		{
			gsm_socket[2].state = CONNECT_IDLE;     /*������cb_socket_close���ж�*/
		}
	}
	if( cid == 5 )                                  /*ȫ�����ӹҶ�*/
	{
	}
	pcurr_socket = RT_NULL;
}

/*
   ���մ���
   ����jt808��ʽ������
   <linkno><����2byte><��ʶ0x7e><��Ϣͷ><��Ϣ��><У����><��ʶ0x7e>

   20130625 ����ճ�������

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
	uint8_t		fstuff = 0;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /*�Ƿ��ֽ����*/

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
					rt_kprintf( "\ncount=%d,fcs err=%d", count, fcs );
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

/*���Ϳ���*/
static void jt808_tx_proc( MsgListNode * node )
{
	MsgListNode			* pnode		= ( MsgListNode* )node;
	JT808_TX_NODEDATA	* pnodedata = ( JT808_TX_NODEDATA* )( pnode->data );
	rt_err_t			ret;

	if( pnodedata->state == IDLE ) /*���У�������Ϣ��ʱ��û������*/
	{
		/*Ҫ�ж��ǲ��ǳ���GSM_TCPIP״̬,��ǰsocket�Ƿ����*/
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

		if( ret != RT_EOK ) /*gsm<ERROR:41 ERROR:35	��������û�еȵ�ģ�鷵�ص�OK�������ط������ǵ�һ��ʱ���ٷ�*/
		{
			gsm_socket[0].state = CONNECT_IDLE;
			gsmstate( GSM_POWEROFF );
			return;
		}
		gsmstate( GSM_TCPIP );
		tick_server_heartbeat = rt_tick_get( );

		if( pnodedata->type == SINGLE_ACK )                                                                 /*Ӧ����Ϣ��ֻ��һ�飬����ɾ������*/
		{
			pnodedata->state = ACK_OK;                                                                      /*����Ϳ���ɾ����*/
		}else
		{
			pnodedata->timeout_tick = rt_tick_get( ) + ( pnodedata->retry + 1 ) * pnodedata->timeout -10;  /*��10��Ϊ������*/
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

	if( pnodedata->state == WAIT_ACK )                                      /*�������Ӧ���Ƿ�ʱ*/
	{
		if( rt_tick_get( ) >= pnodedata->timeout_tick )
		{
			pnodedata->retry++;
			if( pnodedata->retry >= pnodedata->max_retry )
			{
				pnodedata->state = pnodedata->cb_tx_timeout( pnodedata );   /*20130912 ��������,�Ѿ��жϳ�ʱ��,���ڶ�ý����Ϣ����*/
			}else
			{
				pnodedata->state = IDLE;                                    /*�ȴ��´η���*/
			}
		}
	}

	if( ( pnodedata->state == ACK_TIMEOUT ) || ( pnodedata->state == ACK_OK ) )
	{
		//rt_kprintf( "\n%d>free node(%04x) %p", rt_tick_get( ), pnodedata->head_id, pnodedata );
		rt_free( pnodedata->user_para );
		rt_free( pnodedata );               /*ɾ���ڵ�����*/
		list_jt808_tx->first = node->next;  /*ָ����һ��*/
		rt_free( node );
	}
}

/*
   M66�涨����socket 0..2 ��Ӧ��linknoΪ1..3
   ����linkno
   1 �ϱ���������ƽ̨
   2 �ϱ��ӱ�������ƽ̨--�п��ܻ�ͬʱ�ϱ�
   3 �ϱ�IC�����Ļ���·���������

 */
GSM_SOCKET	gsm_socket[3];

GSM_SOCKET	*pcurr_socket = RT_NULL;

//GSM_SOCKET socket_master;
//GSM_SOCKET socket_slave;
//GSM_SOCKET socket_iccard;

/*����������ƽ̨*/
static void socket_master_proc( void )
{
	uint8_t buf[64];
	if( gsm_socket[0].state == CONNECT_NONE )   /*������*/
	{
		return;
	}
	if( gsm_socket[0].state == CONNECT_IDLE )
	{
		if( gsm_socket[0].index % 2 )           /*�����÷�����*/
		{
			strcpy( gsm_socket[0].ipstr, jt808_param.id_0x0017 );
			gsm_socket[0].port = jt808_param.id_0x0018;
		}else /*����������*/
		{
			strcpy( gsm_socket[0].ipstr, jt808_param.id_0x0013 );
			gsm_socket[0].port = jt808_param.id_0x0018;
		}
		gsm_socket[0].state = CONNECT_PEER;                         /*��ʱgsm_state���� GSM_SOCKET_PROC������󷵻� GSM_TCPIP*/
		pcurr_socket		= &gsm_socket[0];
		gsmstate( GSM_SOCKET_PROC );
		jt808_state = AUTH;                                         /*��������Ҫ��Ȩ��ʼ*/
		return;
	}

	if( gsm_socket[0].state == CONNECT_ERROR )                      /*û�����ӳɹ�,�л�������*/
	{
		gsm_socket[0].index++;
		gsm_socket[0].state = CONNECT_IDLE;
	}

	if( gsm_socket[0].state == CONNECTED )                          /*��·ά��������*/
	{
		pcurr_socket = RT_NULL;
		switch( jt808_state )
		{
			case REGISTER:
				buf[0]	= jt808_param.id_0x0081 >> 8;               /*ʡ��*/
				buf[1]	= jt808_param.id_0x0081 & 0xff;
				buf[2]	= jt808_param.id_0x0082 >> 8;               /*����*/
				buf[3]	= jt808_param.id_0x0082 & 0xff;
				memcpy( buf + 4, jt808_param.id_0xF000, 5 );        /*������ID*/
				memcpy( buf + 9, jt808_param.id_0xF001, 20 );       /*�ն��ͺ�*/
				memcpy( buf + 29, jt808_param.id_0xF002, 7 );       /*�ն�ID*/
				buf[36] = jt808_param.id_0x0084;
				strcpy( (char*)buf + 37, jt808_param.id_0x0083 );   /*������ʾ��VIN*/
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
				jt808_report_init( ); /*���³�ʼ��һ���ϱ�*/
				break;
			case REPORT:
				if( tick_server_heartbeat )
				{
					/*Ҫ����������*/
					if( ( rt_tick_get( ) - tick_server_heartbeat ) >= ( jt808_param.id_0x0001 * RT_TICK_PER_SECOND ) )
					{
						//jt808_tx_ack( 0x0002, buf, 0 );
						tick_server_heartbeat = rt_tick_get( ); /*�״��ñ�����ǰʱ��*/
					}
				}else
				{
					tick_server_heartbeat = rt_tick_get( );     /*�״��ñ�����ǰʱ��*/
				}
				break;
		}
	}
}

/*������Ӫ��ƽ̨*/
static void socket_slave_proc( void )
{
}

/*
   ����IC����Զ������ƽ̨
   �Ƿ���Ҫһ�ϵ�����ӣ����ǰ�������

 */
static void socket_iccard_iap_proc( void )
{
	if( gsm_socket[2].state == CONNECT_IDLE )
	{
		if( gsm_socket[2].index != 2 )      /*������oiap*/
		{
			if( gsm_socket[2].index % 2 )   /*�����÷�����*/
			{
				strcpy( gsm_socket[2].ipstr, jt808_param.id_0x001A );
				gsm_socket[2].port = jt808_param.id_0x001B;
			}else /*����������*/
			{
				strcpy( gsm_socket[2].ipstr, jt808_param.id_0x001D );
				gsm_socket[2].port = jt808_param.id_0x001B;
			}
		}
		gsm_socket[2].state = CONNECT_PEER;
		pcurr_socket		= &gsm_socket[2];
		gsmstate( GSM_SOCKET_PROC );
	}

	if( gsm_socket[2].state == CONNECT_ERROR ) /*û�����ӳɹ�,�л�������*/
	{
		//gsm_socket[2].index++;
		//gsm_socket[2].state = CONNECT_IDLE;
		rt_kprintf( "\nsocket2 ���Ӵ���" );
	}
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
	T_GSM_STATE state;

/*���GSM״̬*/
	state = gsmstate( GSM_STATE_GET );
	if( state == GSM_IDLE )
	{
		gsmstate( GSM_POWERON );        /*��������*/
		return;
	}
/*���Ƶ���*/
	if( state == GSM_AT )               /*����Ҫ�ж����Ǹ�apn user psw ����*/
	{
		if( gsm_socket[0].index % 2 )   /*�ñ��÷�����*/
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
/*���ƽ�������,�п��ܻ��޸�gsmstate״̬*/
	if( gsmstate( GSM_STATE_GET ) == GSM_TCPIP )        /*�Ѿ������ˣ�û�д�������socket*/
	{
		socket_master_proc( );
	}
	if( gsmstate( GSM_STATE_GET ) == GSM_TCPIP )        /*�Ѿ������ˣ�û�д�������socket*/
	{
		socket_slave_proc( );
	}
	if( gsmstate( GSM_STATE_GET ) == GSM_TCPIP )        /*�Ѿ������ˣ�û�д�������socket*/
	{
		socket_iccard_iap_proc( );
	}
}

/*
   ����״̬ά��
   jt808Э�鴦��

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

	GPIO_SetBits( GPIOD, GPIO_Pin_9 );      /*�ع���*/
	jt808_misc_init( );
	jt808_gps_init( );

	list_jt808_tx	= msglist_create( );
	list_jt808_rx	= msglist_create( );
/*��ʼ��������Ϣ*/
	if( strlen( jt808_param.id_0xF003 ) )   /*�Ƿ����м�Ȩ��*/
	{
		jt808_state = AUTH;
	}
	gsm_socket[0].state		= CONNECT_IDLE; /*����gsm_socket[0]����*/
	gsm_socket[0].index		= 0;
	gsm_socket[0].linkno	= 1;

	gsm_socket[1].linkno	= 2;
	gsm_socket[2].linkno	= 3;
	while( 1 )
	{
/*����gprs��Ϣ,Ҫ���ַ� �������Ƕ�����ô��?�����в��ϱ�����*/
		//ret = rt_mb_recv( &mb_gprsrx, ( rt_uint32_t* )&pstr, 5 );
		ret = rt_mb_recv( &mb_gprsrx, ( rt_uint32_t* )&pstr, 0 );
		if( ret == RT_EOK )
		{
			jt808_rx_proc( pstr );
			rt_free( pstr );
		}

		jt808_socket_proc( );       /*jt808 socket����*/

/*������Ϣ��������*/
		iter = list_jt808_tx->first;
		if( iter == RT_NULL )       /*û��Ҫ���͵�����*/
		{
			jt808_report_get( );    /*�����û��Ҫ���͵�����*/
		}else /*�����ͽڵ�״̬*/
		{
			jt808_tx_proc( iter );
		}
		rt_thread_delay( RT_TICK_PER_SECOND / 20 );
	}

	msglist_destroy( list_jt808_tx );
}

/*jt808�����̳߳�ʼ��*/
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

/*gprs���մ���,�յ�����Ҫ���촦��*/
rt_err_t gprs_rx( uint8_t linkno, uint8_t * pinfo, uint16_t length )
{
	uint8_t * pmsg;
	pmsg = rt_malloc( length + 3 );                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              /*����������Ϣ*/
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
   �����豸
   �������ԭ��
 */
void reset( unsigned int reason )
{
	uint32_t i = 0x7FFFFFF;
/*û�з��͵�����Ҫ����*/

/*�ر�����*/

/*��־��¼ʱ������ԭ��*/

	rt_kprintf( "\n%d reset>reason=%08x", rt_tick_get( ), reason );
/*ִ������*/
	//rt_thread_delay( RT_TICK_PER_SECOND * 3 );
	while( i-- )
	{
		;
	}

	NVIC_SystemReset( );
}

FINSH_FUNCTION_EXPORT( reset, restart device );

/*�ָ���������*/
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


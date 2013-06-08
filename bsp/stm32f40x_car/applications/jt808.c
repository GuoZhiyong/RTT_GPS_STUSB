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

#include "gsm.h"
#include "m66.h"

#include "jt808_param.h"

#define ByteSwap2( val )    \
    ( ( ( val & 0xff ) << 8 ) |   \
      ( ( val & 0xff00 ) >> 8 ) )

#define ByteSwap4( val )    \
    ( ( ( val & 0xff ) << 24 ) |   \
      ( ( val & 0xff00 ) << 8 ) |  \
      ( ( val & 0xff0000 ) >> 8 ) |  \
      ( ( val & 0xff000000 ) >> 24 ) )

typedef struct
{
	uint16_t id;
	int ( *func )( uint8_t linkno, uint8_t *pmsg );
}HANDLE_JT808_RX_MSG;

static struct rt_mailbox	mb_gprsdata;
#define MB_GPRSDATA_POOL_SIZE 32
static uint8_t				mb_gprsdata_pool[MB_GPRSDATA_POOL_SIZE];

static uint16_t				tx_seq = 0; /*�������*/

static uint16_t		total_send_error=0;  /*�ܵķ��ͳ����������ﵽһ���Ĵ���Ҫ����M66*/

/*������Ϣ�б�*/
MsgList* list_jt808_tx;

/*������Ϣ�б�*/
MsgList * list_jt808_rx;


enum
{
	GSMBUSY_NONE=0x0,
	GSMBUSY_POWERON,    /*�ϵ�Ĺ�����*/
	GSMBUSY_AT,         /*AT*/
	GSMBUSY_TTS,        /*TTSָ��*/
	GSMBUSY_VOICE,      /*¼��*/
	GSMBUSY_SOCKET,     /*�ϱ�����*/
} gsmbusy = GSMBUSY_NONE;


/*�Ƿ��ڲ����У����ܲ�����һ��*/
static uint8_t tts_busy = 0;

/*��������ʹ�õ�mailbox*/
static struct rt_mailbox	mb_tts;
#define MB_TTS_POOL_SIZE 32
static uint8_t				mb_tts_pool[MB_TTS_POOL_SIZE];


typedef enum
{
	CONNECT_IDLE = 0,
	CONNECT_PEER,
	CONNECTED,
	CONNECT_ERROR,
}CONN_STATE;

struct
{
	uint32_t	disable_connect; /*��ֹ���ӱ�־��Э����� 0:��������*/
	CONN_STATE	server_state;
	uint8_t		server_index;
	CONN_STATE	auth_state;
	uint8_t		auth_index;
} connect_state =
{ 0, CONNECT_IDLE, 0, CONNECT_IDLE, 0 };

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
   jt808��ʽ���ݽ����ж�
   <��ʶ0x7e><��Ϣͷ><��Ϣ��><У����><��ʶ0x7e>

   ������Ч�����ݳ���,Ϊ0 �����д�

 */
static uint16_t jt808_decode_fcs( uint8_t * pinfo, uint16_t length )
{
	uint8_t		* psrc, * pdst;
	uint16_t	count, len;
	uint8_t		fstuff	= 0;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     /*�Ƿ��ֽ����*/
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
	psrc	= pinfo + 1;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        /*1byte��ʶ��Ϊ��ʽ��Ϣ*/
	pdst	= pinfo;
	count	= 0;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                /*ת���ĳ���*/
	len		= length - 2;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /*ȥ����ʶλ�����ݳ���*/

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

/**���һ���ֽ�**/
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

/*���ݽ����ȣ����ڼ���*/
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

/*����ַ���***/
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

/**�������**/
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

/*
   jt808�ն˷�����Ϣ
   ���������Ϣע�ᵽ������Ϣ�Ĵ����߳���
   ��Ҫ������ϢID,����Ϣ�壬��jt808_send�߳����
    ��Ϣ�����
    ���ͺ��ط�����
    ��ˮ��
    �ѷ���Ϣ�Ļ���free
   ���ݽ����ĸ�ʽ
   <msgid 2bytes><msg_len 2bytes><msgbody nbytes>

 */
static void jt808_send( void * parameter )
{
}

/*���ͺ��յ�Ӧ����*/
void jt808_tx_response( uint8_t linkno, uint8_t *pmsg )
{
	uint8_t		* msg = pmsg + 12;
	uint16_t	id;
	uint16_t	seq;
	uint8_t		res;

	seq = ( *msg << 8 ) | *( msg + 1 );
	id	= ( *( msg + 2 ) << 8 ) | *( msg + 3 );
	res = *( msg + 4 );

	switch( id )                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            // �ж϶�Ӧ�ն���Ϣ��ID�����ִ���
	{
		case 0x0200:                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        //	��Ӧλ����Ϣ��Ӧ��
			rt_kprintf( "\r\nCentre ACK!\r\n" );
			break;
		case 0x0002:                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        //	��������Ӧ��
			rt_kprintf( "\r\n  Centre  Heart ACK!\r\n" );
			break;
		case 0x0101:                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        //	�ն�ע��Ӧ��
			break;
		case 0x0102:                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        //	�ն˼�Ȩ
			break;
		case 0x0800:                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        // ��ý���¼���Ϣ�ϴ�
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
}

/*
   ��Ϣ���ͳ�ʱ
 */
static rt_err_t jt808_tx_timeout( JT808_TX_MSG_NODEDATA * nodedata )
{
	rt_kprintf( "tx timeout\r\n" );
}

/*
   ���һ����Ϣ�������б���
 */
static rt_err_t jt808_add_tx_data( uint8_t linkno, JT808_MSG_TYPE type, uint16_t id, uint8_t *pinfo, uint16_t len )
{
	uint8_t					* pdata;
	JT808_TX_MSG_NODEDATA	* pnodedata;

	pnodedata = rt_malloc( sizeof( JT808_TX_MSG_NODEDATA ) );
	if( pnodedata == NULL )
	{
		return -RT_ERROR;
	}
	pnodedata->type				= type;
	pnodedata->state			= IDLE;
	pnodedata->retry			= 0;
	pnodedata->cb_tx_timeout	= jt808_tx_timeout;
	pnodedata->cb_tx_response	= jt808_tx_response;
//�ڴ˿��Դ洢���ϱ�
	pdata = rt_malloc( len );
	if( pdata == NULL )
	{
		rt_free( pnodedata );
		return -RT_ERROR;
	}
	memcpy( pdata, pinfo, len );
	pnodedata->msg_len	= len;
	pnodedata->pmsg		= pdata;
	pnodedata->head_sn	= tx_seq;
	pnodedata->head_id	= id;
	msglist_append( list_jt808_tx, pnodedata );
	tx_seq++;
}

/*
   �ն�ͨ��Ӧ��
 */
static rt_err_t jt808_tx_0x0001( uint8_t linkno, uint16_t seq, uint16_t id, uint8_t res )
{
	uint8_t					* pdata;
	JT808_TX_MSG_NODEDATA	* pnodedata;
	uint8_t					buf [256];
	uint8_t					* p;
	uint16_t				len;

	pnodedata = rt_malloc( sizeof( JT808_TX_MSG_NODEDATA ) );
	if( pnodedata == NULL )
	{
		return -RT_ERROR;
	}
	pnodedata->type				= TERMINAL_ACK;
	pnodedata->state			= IDLE;
	pnodedata->retry			= 0;
	pnodedata->cb_tx_timeout	= jt808_tx_timeout;
	pnodedata->cb_tx_response	= jt808_tx_response;

	len		= jt808_pack( buf, "%w%6s%w%w%w%b", 0x0001, term_param.mobile, tx_seq, seq, id, res );
	pdata	= rt_malloc( len );
	if( pdata == NULL )
	{
		rt_free( pnodedata );
		return -RT_ERROR;
	}
	memcpy( pdata, buf, len );
	pnodedata->msg_len	= len;
	pnodedata->pmsg		= pdata;
	msglist_append( list_jt808_tx, pnodedata );
	tx_seq++;
}

/*ƽ̨ͨ��Ӧ��,�յ���Ϣ��ֹͣ����*/
static int handle_rx_0x8001( uint8_t linkno, uint8_t *pmsg )
{
	MsgListNode				* iter;
	JT808_TX_MSG_NODEDATA	* iterdata;

	uint16_t				id;
	uint16_t				seq;
	uint8_t					res;
/*������Ϣͷ12byte*/
	seq = ( *( pmsg + 12 ) << 8 ) | *( pmsg + 13 );
	id	= ( *( pmsg + 14 ) << 8 ) | *( pmsg + 15 );
	res = *( pmsg + 16 );

	/*��������*/
	iter		= list_jt808_tx->first;
	iterdata	= (JT808_TX_MSG_NODEDATA*)iter->data;
	if( ( iterdata->head_id == id ) && ( iterdata->head_sn == seq ) )
	{
		iterdata->cb_tx_response( linkno, pmsg ); /*Ӧ������*/
		iterdata->state = ACK_OK;
	}
}

/*�����ְ�����*/
static int handle_rx_0x8003( uint8_t linkno, uint8_t *pmsg )
{
}

/* ������Ķ��ն�ע����Ϣ��Ӧ��*/
static int handle_rx_0x8100( uint8_t linkno, uint8_t *pmsg )
{
	MsgListNode				* iter;
	JT808_TX_MSG_NODEDATA	* iterdata;

	uint16_t				body_len; /*��Ϣ�峤��*/
	uint16_t				ack_seq;
	uint8_t					res;
	uint8_t					* msg;

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

/*�����ն˲���*/
static int handle_rx_0x8103( uint8_t linkno, uint8_t *pmsg )
{
	uint8_t		* p;
	uint8_t		res = 0;

	uint16_t	msg_len, count = 0;
	uint32_t	param_id;
	uint8_t		param_len;

	uint16_t	seq, id;

	if( *( pmsg + 2 ) >= 0x20 ) /*����Ƕ�������ò���*/
	{
		rt_kprintf( "\r\n>%s multi packet no support!", __func__ );
		return 1;
	}

	id	= ( pmsg[0] << 8 ) | pmsg[1];
	seq = ( pmsg[10] << 8 ) | pmsg[11];

	msg_len = ( ( pmsg[2] << 8 ) | pmsg[3] ) & 0x3FF - 1;
	p		= pmsg + 13;

	/*ʹ�����ݳ���,�ж������Ƿ������û��ʹ�ò�������*/
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
	/*����ͨ��Ӧ��*/
	jt808_tx_0x0001( linkno, seq, id, res );
	return 1;
}

/*��ѯȫ���ն˲������п��ܻᳬ����������ֽ�*/
static int handle_rx_0x8104( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/*�ն˿���*/
static int handle_rx_0x8105( uint8_t linkno, uint8_t *pmsg )
{
	uint8_t cmd;
	uint8_t * cmd_arg;

	cmd = *( pmsg + 12 );
	switch( cmd )
	{
		case 1: /*��������*/
			break;
		case 2: /*�ն˿�������ָ��������*/
			break;
		case 3: /*�ն˹ػ�*/
			break;
		case 4: /*�ն˸�λ*/
			break;
		case 5: /*�ָ���������*/
			break;
		case 6: /*�ر�����ͨѶ*/
			break;
		case 7: /*�ر���������ͨѶ*/
			break;
	}
	return 1;
}

/*��ѯָ���ն˲���,����Ӧ��0x0104*/
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

	pos					= 100;              /*�ȿճ�100byte*/
	param_count			= *( pmsg + 12 );   /*�ܵĲ�������*/
	return_param_count	= 0;
	p					= pmsg + 13;
	/*���Ҫ������Ϣ�����ݣ�����¼����*/
	for( i = 0; i < param_count; i++ )      /*�����δ֪��id��ô�죬����,�������������͸ı���*/
	{
		id	= *p++;
		id	|= ( *p++ ) << 8;
		id	|= ( *p++ ) << 16;
		id	|= ( *p++ ) << 24;
		len = param_get( id, value );       /*�õ������ĳ��ȣ�δת��*/
		if( len )
		{
			return_param_count++;           /*�ҵ���Ч��id*/
			pos += jt808_pack_int( buf + pos, &fcs, id, 2 );
			pos + jt808_pack_int( buf + pos, &fcs, len, 1 );
			pos			+= jt808_pack_array( buf + pos, &fcs, value, len );
			info_len	+= ( len + 3 );     /*id+����+����*/
		}
	}

	head_len	= 1;                        /*�ճ���ʼ��0x7e*/
	head_len	+= jt808_pack_int( buf + head_len, &fcs, 0x0104, 2 );
	head_len	+= jt808_pack_int( buf + head_len, &fcs, info_len + 3, 2 );
	head_len	+= jt808_pack_array( buf + head_len, &fcs, pmsg + 4, 6 );
	head_len	+= jt808_pack_int( buf + head_len, &fcs, tx_seq, 2 );

	head_len	+= jt808_pack_array( buf + head_len, &fcs, pmsg + 10, 2 );
	head_len	+= jt808_pack_int( buf + head_len, &fcs, return_param_count, 1 );

	memcpy( buf + head_len, buf + 100, pos - 100 ); /*ƴ������*/
	len = head_len + pos - 100;                     /*��ǰ����0x7e,<head><msg>*/

	len			+= jt808_pack_byte( buf + len, &fcs, fcs );
	buf [0]		= 0x7e;
	buf [len]	= 0x7e;

	jt808_add_tx_data( linkno, TERMINAL_ACK, 0x0104, buf, len + 1 );
	return 1;
}

/*��ѯ�ն�����,Ӧ�� 0x0107*/
static int handle_rx_0x8107( uint8_t linkno, uint8_t *pmsg )
{
	uint8_t		buf[100];
	uint8_t		fcs			= 0;
	uint16_t	len			= 1;
	uint16_t	info_len	= 0;
	uint16_t	head_len	= 1;

	len += jt808_pack_int( buf + len, &fcs, 0x0107, 2 );
	len += jt808_pack_int( buf + len, &fcs, 0x0107, 2 );
	len += jt808_pack_int( buf + len, &fcs, 0x0107, 2 );

	jt808_add_tx_data( linkno, TERMINAL_ACK, 0x0107, buf, len + 1 );
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

/*��ʻ��¼�����ݲɼ�*/
static int handle_rx_0x8700( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/*��ʻ��¼�ǲ����´�*/
static int handle_rx_0x8701( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
static int handle_rx_0x8800( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/*����ͷ������������*/
static int handle_rx_0x8801( uint8_t linkno, uint8_t *pmsg )
{
	return 1;
}

/**/
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
	DECL_JT808_RX_HANDLE( 0x8001 ), //	ͨ��Ӧ��
	DECL_JT808_RX_HANDLE( 0x8003 ), //	�����ְ�����
	DECL_JT808_RX_HANDLE( 0x8100 ), //  ������Ķ��ն�ע����Ϣ��Ӧ��
	DECL_JT808_RX_HANDLE( 0x8103 ), //	�����ն˲���
	DECL_JT808_RX_HANDLE( 0x8104 ), //	��ѯ�ն˲���
	DECL_JT808_RX_HANDLE( 0x8105 ), // �ն˿���
	DECL_JT808_RX_HANDLE( 0x8106 ), // ��ѯָ���ն˲���
	DECL_JT808_RX_HANDLE( 0x8201 ), // λ����Ϣ��ѯ    λ����Ϣ��ѯ��Ϣ��Ϊ��
	DECL_JT808_RX_HANDLE( 0x8202 ), // ��ʱλ�ø��ٿ���
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


/*
   ���մ���
   ����jt808��ʽ������
   <linkno><����2byte><��ʶ0x7e><��Ϣͷ><��Ϣ��><У����><��ʶ0x7e>

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
	rt_kprintf( ">dump start len=%d\r\n", len );

/*ȥת�壬����ֱ����pinfo�ϲ���*/
	len = jt808_decode_fcs( pinfo + 3, len );
	if( len == 0 )
	{
		rt_kprintf( ">len=0\r\n" );
		return 1;
	}
/*��ʾ��������Ϣ*/
	rt_kprintf( "\r\n>dump start" );
	psrc = pinfo + 3;
	for( i = 0; i < len; i++ )
	{
		if( i % 16 == 0 )
		{
			rt_kprintf( "\r\n" );
		}
		rt_kprintf( "%02x ", *psrc++ );
	}
	rt_kprintf( "\r\n>dump end\r\n" );
/*fcs����*/
	psrc = pinfo + 3;
	for( i = 0; i < len - 1; i++ )
	{
		fcs ^= *psrc++;
	}
	if( fcs != *( psrc + len - 1 ) )
	{
		rt_kprintf( "\r\n%d>%s fcs error!", rt_tick_get( ), __func__ );
		return 1;
	}

/*ֱ�Ӵ����յ�����Ϣ������ID�ַ���ֱ�ӷַ���Ϣ*/

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
   ����ÿ��Ҫ������Ϣ��״̬
   ���������д�����?

   2013.06.08���Ӷ�����ʹ���
 */

static MsgListRet jt808_tx_proc( MsgListNode * node )
{
	MsgListNode				* pnode		= ( MsgListNode* )node;
	JT808_TX_MSG_NODEDATA	* pnodedata = ( JT808_TX_MSG_NODEDATA* )( pnode->data );
	int						i;
	rt_err_t	ret;

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
		} else
		{
			if(pnodedata->multipacket==0) /*�ж��Ƿ�Ϊ�������*/
			{
				ret=socket_write( pnodedata->linkno, pnodedata->pmsg, pnodedata->msg_len );
				if(ret==RT_EOK) /*���ͳɹ��ȴ�����Ӧ����*/
				{
					pnodedata->tick = rt_tick_get( );
					pnodedata->retry++;
					pnodedata->timeout	= pnodedata->retry * jt808_param.id_0x0002 * RT_TICK_PER_SECOND;
					pnodedata->state	= WAIT_ACK;
					rt_kprintf( "send retry=%d,timeout=%d\r\n", pnodedata->retry, pnodedata->timeout * 10 );
				}else   /*��������û�еȵ�ģ�鷵�ص�OK�������ط������ǵ�һ��ʱ���ٷ�*/
				{
					pnodedata->retry++;		/*��λ�ٴη���*/
					total_send_error++;
					rt_kprintf("total_send_error=%d\r\n",total_send_error);
				}
			}
			else
			{



			}

			
		}
		return MSGLIST_RET_OK;
	}

	if( pnodedata->state == WAIT_ACK )	/*�������Ӧ���Ƿ�ʱ*/
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
void cb_socket_state( uint8_t linkno, T_SOCKET_STATE new_state )
{
	if( linkno == 1 )
	{
		connect_state.server_state = new_state;
	}

	if( linkno == 2 )
	{
		connect_state.auth_state = new_state;
	}
}


/*808���Ӵ���*/
static void jt808_socket_proc( void )
{
	T_GSM_STATE state;

/*����Ƿ�����gsm����*/
	if( connect_state.disable_connect )
	{
		return;
	}

/*���GSM״̬*/
	state = gsmstate( GSM_STATE );
	if( state == GSM_IDLE )
	{
		gsmstate( GSM_POWERON );        /*��������*/
		return;
	}
/*���Ƶ���*/
	if( state == GSM_AT )               /*����Ҫ�ж����Ǹ�apn user psw ����*/
	{
		if(connect_state.server_index % 2 )    /*�ñ��÷�����*/
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
	if( state == GSM_TCPIP )                        /*�Ѿ�������*/
	{
		if( connect_state.server_state == CONNECT_IDLE )
		{
			if( connect_state.server_index % 2 )    /*�����÷�����*/
			{
				ctl_socket( 1, 't', jt808_param.id_0x0017, jt808_param.id_0x0018, 1 );
			}else /*����������*/
			{
				ctl_socket( 1, 't', jt808_param.id_0x0013, jt808_param.id_0x0018, 1 );
			}
			connect_state.server_state = CONNECT_PEER;
			return;
		}

		if( connect_state.server_state == CONNECT_PEER ) /*�������ӵ�������*/
		{
			if( socketstate( SOCKET_STATE ) == SOCKET_READY )
			{
				connect_state.server_state = CONNECTED;
			}
			else		/*û�����ӳɹ�,�л�������*/
			{
				connect_state.server_index++;
				connect_state.server_state=CONNECT_IDLE;
			}
		}

		/*808������û�����ӳɹ����Ͳ���IC��������*/
		if( connect_state.server_state != CONNECTED )
		{
			return;
		}
		
		/*����IC��������*/

		if( connect_state.auth_state == CONNECT_IDLE )  /*û������*/
		{
			if( connect_state.auth_index % 2 )          /*�����÷�����*/
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
			}
			else		/*û�����ӳɹ�,�л�������*/
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
void jt808_tts_process( void )
{
	rt_err_t	ret;
	rt_size_t	len;
	uint8_t		*pinfo, *p;
	uint8_t		c;

	char		buf[20];
	char		tbl[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
/*��ǰ���ڲ�����*/
	if( tts_busy == 1 )
	{
		return;
	}
/*gsm�ڴ�����������*/
	if( gsmbusy )
	{
		return;
	}
	gsmbusy = GSMBUSY_TTS;
/*�Ƿ�����ϢҪ����*/
	ret = rt_mb_recv( &mb_tts, (rt_uint32_t*)&pinfo, 0 );
	if( ret == -RT_ETIMEOUT )
	{
		gsmbusy = GSMBUSY_NONE;
		return;
	}

	GPIO_ResetBits( GPIOD, GPIO_Pin_9 ); /*������*/
#if 0
	sprintf( buf, "AT%%TTS=2,3,5,\"" );

	m66_write( &dev_gsm, 0, buf, strlen( buf ) );
	rt_kprintf( "%s", buf );

	len = ( *pinfo << 8 ) | ( *( pinfo + 1 ) );
	p	= pinfo + 2;
	while( len-- )
	{
		c = *p++;

		USART_SendData( UART4, tbl[c >> 4] );
		while( USART_GetFlagStatus( UART4, USART_FLAG_TC ) == RESET )
		{
		}
		rt_kprintf( "%c", tbl[c >> 4] );
		USART_SendData( UART4, tbl[c & 0x0f] );
		while( USART_GetFlagStatus( UART4, USART_FLAG_TC ) == RESET )
		{
		}
		rt_kprintf( "%c", tbl[c & 0x0f] );
	}
	buf[0]	= '"';
	buf[1]	= 0x0d;
	buf[2]	= 0x0a;
	buf[3]	= 0;
	m66_write( &dev_gsm, 0, buf, 3 );

	rt_kprintf( "%s", buf );
#endif
/*���жϣ���gsmrx_cb�д���*/
	rt_free( pinfo );
	//gsm_wait_str( "%TTS: 0", RT_TICK_PER_SECOND * 5 );
	GPIO_SetBits( GPIOD, GPIO_Pin_9 ); /*�ع���*/
	gsmbusy = GSMBUSY_NONE;
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
	rt_err_t				ret;
	int						i;
	uint8_t					* pstr;

	MsgListNode				* iter;
	JT808_TX_MSG_NODEDATA	* pnodedata;

	int						j = 0xaabbccdd;

	rt_kprintf( "\r\n1.id0=%08x\r\n", param_get_int( 0x0000 ) );

	param_put_int( 0x000, j );
	rt_kprintf( "\r\nwrite j=%08x read=%08x\r\n", j, param_get_int( 0x0000 ) );

	param_put( 0x000, 4, (uint8_t*)&j );
	rt_kprintf( "\r\nid0=%08x\r\n", param_get_int( 0x0000 ) );

/*��ȡ������������*/
	//param_load( );

	list_jt808_tx	= msglist_create( );
	list_jt808_rx	= msglist_create( );

	while( 1 )
	{
/*����gprs��Ϣ*/
		ret = rt_mb_recv( &mb_gprsdata, ( rt_uint32_t* )&pstr, 5 );
		if( ret == RT_EOK )
		{
			jt808_rx_proc( pstr );
			rt_free( pstr );
		}
/*jt808 socket����*/
		jt808_socket_proc( );
/*tts����*/
		jt808_tts_process( );


/*������Ϣ��������*/
		iter = list_jt808_tx->first;

		if( jt808_tx_proc( iter ) == MSGLIST_RET_DELETE_NODE )  /*ɾ���ýڵ�*/
		{
			pnodedata = ( JT808_TX_MSG_NODEDATA* )( iter->data );
			rt_free( pnodedata->pmsg );                         /*ɾ���û�����*/
			rt_free( pnodedata );                               /*ɾ���ڵ�����*/
			list_jt808_tx->first = iter->next;                  /*ָ����һ��*/
			rt_free( iter );
		}
		rt_thread_delay( RT_TICK_PER_SECOND / 20 );
	}

	msglist_destroy( list_jt808_tx );
}

/*jt808�����̳߳�ʼ��*/
void jt808_init( void )
{
	rt_mb_init( &mb_gprsdata, "mb_gprs", &mb_gprsdata_pool, MB_GPRSDATA_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );
	rt_mb_init( &mb_tts, "mb_tts", &mb_tts_pool, MB_TTS_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );

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
	pmsg = rt_malloc( length + 3 );                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            /*����������Ϣ*/
	if( pmsg != RT_NULL )
	{
		pmsg [0]	= linkno;
		pmsg [1]	= length >> 8;
		pmsg [2]	= length & 0xff;
		memcpy( pmsg + 3, pinfo, length );
		rt_mb_send( &mb_gprsdata, ( rt_uint32_t )pmsg );
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
static rt_err_t demo_jt808_tx( void )
{
	uint8_t					* pdata;
	JT808_TX_MSG_NODEDATA	* pnodedata;
	uint8_t					buf [256];
	uint16_t				len;
	uint8_t					fcs = 0;
	int						i;

	pnodedata = rt_malloc( sizeof( JT808_TX_MSG_NODEDATA ) );
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




/*
   �յ�tts��Ϣ������
   ����0:OK
    1:����RAM����
 */
rt_size_t tts_write( char* info )
{
	uint8_t		*pmsg;
	uint16_t	count;
	count = strlen( info );

	/*ֱ�ӷ��͵�Mailbox��,�ڲ�����*/
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


/************************************** The End Of File **************************************/


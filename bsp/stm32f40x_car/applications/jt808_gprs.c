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
#include "stm32f4xx.h"


/*
   jt808��ʽ���ݽ����ж�
   ����ʱ��ȥ���ȵ������ֽڣ��Ͱ�ͷ�Ŀ���

   <����2byte><��ʶ0x7e><��Ϣͷ><��Ϣ��><У����><��ʶ0x7e>

   ������Ч�����ݳ���,Ϊ0 �����д�

 */
static uint16_t jt808_decode_fcs( uint8_t *pinfo, uint16_t length )
{
	uint8_t		*psrc, *pdst;
	uint16_t	count	= length - 4, len = length - 4;
	uint8_t		fstuff	= 0; /*�Ƿ��ֽ����*/
	uint8_t		fcs		= 0;

	if( length < 5 )
	{
		return 0;
	}
	if( *( pinfo + 2 ) != 0x7e )
	{
		return 0;
	}
	if( *( pinfo + length + 1 ) )
	{
		return 0;
	}
	psrc	= pinfo + 3; /*2byte����+1byte��ʶ��Ϊ��ʽ��Ϣ*/
	pdst	= pinfo;
	while( len )
	{
		if( fstuff )
		{
			if( *psrc == 0x02 )
			{
				*pdst = 0x7e;
			}
			if( *psrc == 0x01 )
			{
				*pdst = 0x7d;
			}
			fstuff = 0;
			count--;
			fcs ^= *pdst;
		}else
		{
			if( *psrc == 0x7d )
			{
				fstuff = 1;
			} else
			{
				*pdst	= *psrc;
				fcs		^= *pdst;
			}
		}
		psrc++;
		pdst++;
		len--;
	}
	if( fcs != 0 )
	{
		rt_kprintf( "%s>fcs error\n", __func__ );
		return 0;
	}
	return count;
}

#if 0


/*
   �����ݽ���FCS�����У��
 */
static uint8_t jt808_fcs( uint8_t * pinfo, uint16_t length )
{
	uint8_t		fcs = 0;
	uint8_t		*psrc;
	uint16_t	i;
	psrc = pinfo;
	for( i = 0; i < length; i++ )
	{
		fcs ^= *psrc;
	}
	return fcs;
}

#endif


/*
   jt808��ʽ����FCS���㣬������
   ���ر��������ݳ���
 */
static uint16_t jt808_encode( uint8_t * pinfo, uint16_t length )
{
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
static void jt808_send( void* parameter )
{
}

#define ACK_0200 1

/*���������·���ACK��Ϣ*/
static uint16_t jt808_analy_ack( uint8_t * pinfo, uint16_t length )
{
}

/*
   ����jt808��ʽ������
   <����2byte><��ʶ0x7e><��Ϣͷ><��Ϣ��><У����><��ʶ0x7e>

 */
uint16_t jt808_analy( uint8_t * pinfo )
{
	uint8_t		*psrc;

	uint16_t	header_id;          /*��ϢID*/
	uint16_t	header_attr;        /*����*/
	uint8_t		header_mobile[6];   /*BCD �ֻ���*/
	uint16_t	header_seq;         /*��ˮ��*/
	uint16_t	msg_len;            /*��Ϣ����*/

	uint16_t	msg_seq;            /*�յ���Ϣ�е���ˮ��*/
	uint16_t	msg_id;             /*�յ���Ϣ�е�����id*/
	uint16_t	len;

	len = ( pinfo[0] << 8 ) | pinfo[1];
	rt_kprintf( "gsm>rx %d bytes\r\n", len );
	len = jt808_decode_fcs( pinfo + 2, len );
	if( len == 0 )
	{
		return 0;
	}
	psrc		= pinfo;
	header_id	= *( psrc + 0 ) << 8 | *( psrc + 1 );
	header_attr = *( psrc + 2 ) << 8 | *( psrc + 3 );
	memcpy( header_mobile, psrc + 4, 6 );
	header_seq = *( psrc + 10 ) << 8 | *( psrc + 11 );
/*�ְ�����*/
	if( header_attr & 0x20 )
	{
	}
	msg_len = header_attr & 0x3ff;
	switch( header_id )
	{
		case 0x8001:            /*ƽ̨ͨ��Ӧ��*/


			/* ��û�зְ�����Ļ�  ��Ϣͷ��12  ��0��ʼ�����12���ֽ�����Ϣ�������
			   13 14	��Ӧ���ն���Ϣ��ˮ��
			   15 16	��Ӧ�ն˵���Ϣ
			 */
			msg_id = ( *( psrc + 14 ) << 8 ) + *( psrc + 15 );

			switch( msg_id )    // �ж϶�Ӧ�ն���Ϣ��ID�����ִ���
			{
				case 0x0200:    //  ��Ӧλ����Ϣ��Ӧ��
					rt_kprintf( "\r\nCentre ACK!\r\n" );
					break;
				case 0x0002:    //  ��������Ӧ��
					rt_kprintf( "\r\n  Centre  Heart ACK!\r\n" );
					break;
				case 0x0101:    //  �ն�ע��Ӧ��
					break;
				case 0x0102:    //  �ն˼�Ȩ
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
					break;
			}
			break;
		case 0x8100:    //  ������Ķ��ն�ע����Ϣ��Ӧ��
			break;
		case 0x8103:    //  �����ն˲���
			break;
		case 0x8104:    //  ��ѯ�ն˲���
			break;
		case 0x8105:    // �ն˿���
			break;
		case 0x8201:    // λ����Ϣ��ѯ    λ����Ϣ��ѯ��Ϣ��Ϊ��
			break;
		case 0x8202:    // ��ʱλ�ø��ٿ���
			break;
		case 0x8300:    //  �ı���Ϣ�·�
			break;
		case 0x8301:    //  �¼�����
			break;
		case 0x8302:    // �����·�
			break;
		case 0x8303:    //  ��Ϣ�㲥�˵�����
			break;
		case 0x8304:    //  ��Ϣ����
			break;
		case 0x8400:    //  �绰�ز�
			break;
		case 0x8401:    //   ���õ绰��
			break;
		case 0x8500:    //  ��������
			break;
		case 0x8600:    //  ����Բ������
			break;
		case 0x8601:    //  ɾ��Բ������
			break;
		case 0x8602:    //  ���þ�������
			break;
		case 0x8603:    //  ɾ����������
			break;
		case 0x8604:    //  ���������
			break;
		case 0x8605:    //  ɾ���������
			break;
		case 0x8606:    //  ����·��
			break;
		case 0x8607:    //  ɾ��·��
			break;
		case 0x8700:    //  �г���¼�����ݲɼ�����
			break;
		case 0x8701:    //  ��ʻ��¼�ǲ����´�����
			break;
		case 0x8800:    //	��ý�������ϴ�Ӧ��
			break;
		case 0x8801:    //	����ͷ��������
			break;
		case 0x8802:    //	�洢��ý�����ݼ���
			break;
		case 0x8803:    //	�洢��ý�������ϴ�����
			break;
		case 0x8804:    //	¼����ʼ����
			break;
		case 0x8805:    //	�����洢��ý�����ݼ����ϴ����� ---- ����Э��Ҫ��
			break;
		case 0x8900:    //	��������͸��
			break;
		case 0x8A00:    //	ƽ̨RSA��Կ
			break;
		default:
			break;
	}
}

/************************************** The End Of File **************************************/

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
#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_posix.h>

#include "stm32f4xx.h"
#include "rs485.h"
#include "jt808.h"
#include "camera.h"
#include "jt808_camera.h"
#include <finsh.h>
#include "sst25.h"

typedef __packed struct
{
	u32 Address;        ///��ַ
	u32 Data_ID;        ///����ID
	u8	Delete;         ///ɾ�����
	u8	Pack_Mark[16];  ///�����
}TypePicMultTransPara;


/*********************************************************************************
  *��������:u16 Cam_add_tx_pic_getdata( JT808_TX_NODEDATA * nodedata )
  *��������:��jt808_tx_proc��״̬ΪGET_DATAʱ��ȡ��Ƭ���ݣ��ú����� JT808_TX_NODEDATA �� get_data �ص�����
  *�� ��:	nodedata	:���ڴ���ķ�������
  *�� ��: none
  *�� �� ֵ:rt_err_t
  *�� ��:������
  *��������:2013-06-16
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
static u16 Cam_add_tx_pic_getdata( JT808_TX_NODEDATA * nodedata )
{
	JT808_TX_NODEDATA		* iterdata	= nodedata;
	TypePicMultTransPara	* p_para	= (TypePicMultTransPara*)nodedata->user_para;
	TypeDF_PackageHead		TempPackageHead;
	uint16_t				i, wrlen, pack_num;
	uint16_t				body_len; /*��Ϣ�峤��*/
	uint8_t					* msg;
	uint32_t				tempu32data;
	u16						ret = 0;
	uint8_t					* pdata;

	RT_ASSERT( RT_EOK == rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY ) );
	tempu32data = Cam_Flash_FindPicID( p_para->Data_ID, &TempPackageHead );
	if( tempu32data == 0xFFFFFFFF )
	{
		rt_kprintf( "\n û���ҵ�ͼƬ��ID=%d", p_para->Data_ID );
		ret = 0xFFFF;
		goto FUNC_RET;
	}
	pdata = nodedata->tag_data;

	for( i = 0; i < iterdata->packet_num; i++ )
	{
		if( p_para->Pack_Mark[i / 8] & BIT( i % 8 ) )
		{
			if( ( i + 1 ) < iterdata->packet_num )
			{
				body_len = JT808_PACKAGE_MAX;
			}else /*���һ��*/
			{
				body_len = iterdata->size - JT808_PACKAGE_MAX * i;
			}
			pdata[0]	= 0x08;
			pdata[1]	= 0x01;
			pdata[2]	= 0x20 | ( body_len >> 8 ); /*��Ϣ�峤��*/
			pdata[3]	= body_len & 0xff;

			iterdata->packet_no = i + 1;
			iterdata->head_sn	= 0xF001 + i;
			pdata[10]			= ( iterdata->head_sn >> 8 );
			pdata[11]			= ( iterdata->head_sn & 0xFF );

			wrlen				= 16;                                                                   /*������Ϣͷ*/
			iterdata->msg_len	= body_len + 16;                                                        ///�������ݵĳ��ȼ�����Ϣͷ�ĳ���
			if( i == 0 )
			{
				sst25_read( tempu32data, (u8*)&TempPackageHead, sizeof( TypeDF_PackageHead ) );
				wrlen	+= data_to_buf( iterdata->tag_data + wrlen, TempPackageHead.Data_ID, 4 );       ///��ý��ID
				wrlen	+= data_to_buf( iterdata->tag_data + wrlen, TempPackageHead.Media_Style, 1 );   ///��ý������
				wrlen	+= data_to_buf( iterdata->tag_data + wrlen, TempPackageHead.Media_Format, 1 );  ///��ý���ʽ����
				wrlen	+= data_to_buf( iterdata->tag_data + wrlen, TempPackageHead.TiggerStyle, 1 );   ///�¼������
				wrlen	+= data_to_buf( iterdata->tag_data + wrlen, TempPackageHead.Channel_ID, 1 );    ///ͨ�� ID
				memcpy( iterdata->tag_data + wrlen, TempPackageHead.position, 28 );                     ///λ����Ϣ�㱨
				wrlen += 28;
				sst25_read( tempu32data + 64, iterdata->tag_data + wrlen, ( 512 - 36 ) );               /*��ǰ����ϢͷҲ���������*/
			}else
			{
				tempu32data = tempu32data + JT808_PACKAGE_MAX * i + 64 - 36;
				sst25_read( tempu32data, iterdata->tag_data + wrlen, body_len );
			}
			p_para->Pack_Mark[i / 8] &= ~( BIT( i % 8 ) );
			rt_kprintf( "\n cam_get_data ok PAGE=%d\n", iterdata->packet_no );
			ret = iterdata->packet_no;
			/*�������� ������Ϣͷ*/
			pdata[12]			= ( iterdata->packet_num >> 8 );
			pdata[13]			= ( iterdata->packet_num & 0xFF );
			pdata[14]			= ( iterdata->packet_no >> 8 );
			pdata[15]			= ( iterdata->packet_no & 0xFF );
			nodedata->state		= IDLE;
			nodedata->timeout	= RT_TICK_PER_SECOND * 5;
			nodedata->retry		= 0;
			nodedata->max_retry = 5;
			nodedata->timeout	= 10 * RT_TICK_PER_SECOND;

			goto FUNC_RET;
		}
	}
	rt_kprintf( "\n cam_get_data_false!" );
	ret = 0;
FUNC_RET:
	rt_sem_release( &sem_dataflash );
	return ret;
}

/*********************************************************************************
  *��������:JT808_MSG_STATE Cam_jt808_timeout( JT808_TX_NODEDATA * nodedata )
  *��������:����ͼƬ������ݵĳ�ʱ������
  *�� ��:	nodedata	:���ڴ���ķ�������
  *�� ��: none
  *�� �� ֵ:JT808_MSG_STATE
  *�� ��:������
  *��������:2013-06-16
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
static JT808_MSG_STATE Cam_jt808_timeout( JT808_TX_NODEDATA * nodedata )
{
	u16					cmd_id;
	TypePicMultTransPara* p_para;
	uint16_t			ret;

	cmd_id	= nodedata->head_id;
	p_para	= (TypePicMultTransPara*)( nodedata->user_para );
	switch( cmd_id )
	{
		case 0x0800:                                            /*��ʱ�Ժ�ֱ���ϱ�����*/
			Cam_jt808_0x0801( nodedata, p_para->Data_ID, p_para->Delete );
			break;
		case 0x0801:                                            /*�ϱ�ͼƬ����*/
			if( nodedata->packet_no == nodedata->packet_num )   /*���ϱ����ˣ�����ʱ*/
			{
				rt_kprintf( "\nȫ���ϱ����\n" );               /*�ȴ�Ӧ��0x8800*/
				nodedata->state = ACK_OK;

				return WAIT_DELETE;
			}
			ret = Cam_add_tx_pic_getdata( nodedata );
			if( ret == 0xFFFF )                                 /*bitter û���ҵ�ͼƬid*/
			{
				rt_free( p_para );
				p_para = RT_NULL;
				rt_free( nodedata );
				rt_kprintf( "\n%s������", __func__ );
			}
			break;
		default:
			break;
	}
	return IDLE;
}

/*********************************************************************************
  *��������:JT808_MSG_STATE Cam_jt808_0x801_response( JT808_TX_NODEDATA * nodedata , uint8_t *pmsg )
  *��������:��jt808�д�����Ƭ���ݴ���ACK_ok�������ú����� JT808_TX_NODEDATA �� cb_tx_response �ص�����
  *�� ��:	nodedata	:���ڴ���ķ�������
  *�� ��: none
  *�� �� ֵ:JT808_MSG_STATE
  *�� ��:������
  *��������:2013-06-16
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
static JT808_MSG_STATE Cam_jt808_0x0801_response( JT808_TX_NODEDATA * nodedata, uint8_t *pmsg )
{
	JT808_TX_NODEDATA		* iterdata = nodedata;
	TypePicMultTransPara	* p_para;
	uint16_t				temp_msg_id;
	uint16_t				temp_msg_len;
	uint16_t				i, pack_num;
	uint32_t				tempu32data;
	uint16_t				ret;
	uint16_t				ack_seq;                                            /*Ӧ�����*/

	temp_msg_id = buf_to_data( pmsg, 2 );
	p_para=nodedata->user_para;
	
	if( 0x8001 == temp_msg_id )                                                 ///ͨ��Ӧ��,�п���Ӧ����ʱ
	{
		ack_seq = ( pmsg[12] << 8 ) | pmsg[13];                                 /*�ж�Ӧ����ˮ��*/
		if( ack_seq != nodedata->head_sn )                                      /*��ˮ�Ŷ�Ӧ*/
		{
			nodedata->timeout_tick	= rt_tick_get( ) + 15 * RT_TICK_PER_SECOND; /*��15��*/
			nodedata->state			= WAIT_ACK;
			return WAIT_ACK;
		}
		if( nodedata->packet_no == nodedata->packet_num )                       /*�������ݰ��ϱ���ɣ��ȴ������·�0x8800*/
		{
			rt_kprintf( "\n�ȴ�8800Ӧ��\n" );
			nodedata->timeout_tick	= rt_tick_get( ) + 15 * RT_TICK_PER_SECOND; /*��15��*/
			nodedata->state			= WAIT_ACK;
			nodedata->packet_no++;
			return WAIT_ACK;
		}

		if( nodedata->packet_no > nodedata->packet_num )                        /*��ʱ�ȴ�0x8800Ӧ��*/
		{
			nodedata->state = ACK_OK;
			return WAIT_DELETE;
		}

		ret = Cam_add_tx_pic_getdata( nodedata );
		if( ret == 0xFFFF )                                                     /*bitter û���ҵ�ͼƬid*/
		{
			rt_free( p_para );
			p_para = RT_NULL;
			rt_free( nodedata );
			rt_kprintf( "\n%s������", __func__ );
			nodedata->state = ACK_OK;
			return WAIT_DELETE;
		}
		nodedata->state = IDLE;                     /*��������*/
		return IDLE;
	}

	if( 0x8800 == temp_msg_id )                     ///ר��Ӧ��
	{
		rt_kprintf( "\n�յ�ר��Ӧ��" );
		tempu32data = buf_to_data( pmsg + 12, 4 );  /*Ӧ��media ID*/

		if( tempu32data != p_para->Data_ID )
		{
			rt_kprintf( "\nӦ��ID����ȷ %08x %08x",tempu32data,p_para->Data_ID );
			nodedata->state = ACK_OK;
			return WAIT_DELETE;
		}
		if( pmsg[16] == 0 )       /*�ش�������*/
		{
			if( p_para->Delete )
			{
				//Cam_Flash_DelPic(p_para->Data_ID);
			}
			rt_kprintf( "\n����ͼƬ�ϱ����" );
			nodedata->state = ACK_OK;
			return WAIT_DELETE;
		}

		p_para = (TypePicMultTransPara*)( iterdata->user_para );
		memset( p_para->Pack_Mark, 0, sizeof( p_para->Pack_Mark ) );

		for( i = 0; i < pmsg[16]; i++ )
		{
			tempu32data = buf_to_data( pmsg + i * 2 + 17, 2 );
			if( tempu32data )
			{
				tempu32data--;
			}
			p_para->Pack_Mark[tempu32data / 8] |= BIT( tempu32data % 8 );
		}
		rt_kprintf( "\n Cam_jt808_0x801_response\n lost_pack=%d", pmsg[16] );   /*���»�ȡ��������*/
		ret = Cam_add_tx_pic_getdata( nodedata );
		if( ret == 0xFFFF )                                                     /*bitter û���ҵ�ͼƬid*/
		{
			rt_free( p_para );
			p_para = RT_NULL;
			rt_free( nodedata );
			rt_kprintf( "\n%s������", __func__ );
			nodedata->state = ACK_OK;
			return WAIT_DELETE;
		}
		nodedata->state=IDLE;
		return IDLE;
	}

	return nodedata->state;
}

/*********************************************************************************
  *��������:rt_err_t Cam_jt808_0x0801(u32 mdeia_id ,u8 media_delete)
  *��������:���һ����ý��ͼƬ�������б���
  *�� ��:	mdeia_id	:��Ƭid
   media_delete:��Ƭ���ͽ������Ƿ�ɾ����ǣ���0��ʾɾ��
  *�� ��: none
  *�� �� ֵ:rt_err_t
  *�� ��:������
  *��������:2013-06-16
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
rt_err_t Cam_jt808_0x0801( JT808_TX_NODEDATA *nodedata, u32 mdeia_id, u8 media_delete )
{
	u16						i;
	u32						TempAddress;
	TypePicMultTransPara	* p_para;
	TypeDF_PackageHead		TempPackageHead;
	rt_err_t				rt_ret;

	uint16_t				ret;

	JT808_TX_NODEDATA		* pnodedata;

	///���Ҷ�ý��ID�Ƿ����

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
	TempAddress = Cam_Flash_FindPicID( mdeia_id, &TempPackageHead );
	rt_sem_release( &sem_dataflash );

	if( TempAddress == 0xFFFFFFFF )
	{
		rt_kprintf( "\nû���ҵ�ͼƬ" );
		return RT_ERROR;
	}

	if( nodedata == RT_NULL )
	{
		///�����ý��˽����Դ
		p_para = rt_malloc( sizeof( TypePicMultTransPara ) );
		if( p_para == NULL )
		{
			return RT_ENOMEM;
		}

		///����û�������
		memset( p_para, 0xFF, sizeof( TypePicMultTransPara ) );

		p_para->Address = TempAddress;
		p_para->Data_ID = mdeia_id;
		p_para->Delete	= media_delete;

		pnodedata = node_begin( 1, MULTI_CMD, 0x0801, 0xF001, 512 + 64 );
		if( pnodedata == RT_NULL )
		{
			rt_free( p_para );
			rt_kprintf( "\n������Դʧ��" );
			return RT_ENOMEM;
		}
		pnodedata->size			= TempPackageHead.Len - 64 + 36; /*�ճ���64�ֽڵ�ͷ��������36�ֽ�ͼƬ��Ϣ*/
		pnodedata->packet_num	= ( pnodedata->size + JT808_PACKAGE_MAX - 1 ) / JT808_PACKAGE_MAX;
		pnodedata->packet_no	= 0;
		pnodedata->user_para	= p_para;
	}else
	{
		pnodedata				= nodedata;
		pnodedata->type	= MULTI_CMD;
	}

	ret = Cam_add_tx_pic_getdata( pnodedata );
	if( ret == 0xFFFF ) /*bitter û���ҵ�ͼƬid*/
	{
		rt_free( p_para );
		p_para = RT_NULL;
		rt_free( pnodedata );
		rt_kprintf( "\n%s������", __func__ );
	}else
	{
		pnodedata->retry		= 0;
		pnodedata->max_retry	= 5;
		pnodedata->timeout		= 10 * RT_TICK_PER_SECOND;
		node_end( pnodedata, Cam_jt808_timeout, Cam_jt808_0x0801_response, p_para );
	}
	return RT_EOK;
}

/*********************************************************************************
  *��������:JT808_MSG_STATE Cam_jt808_0x801_response( JT808_TX_NODEDATA * nodedata , uint8_t *pmsg )
  *��������:��jt808�д�����Ƭ���ݴ���ACK_ok�������ú����� JT808_TX_NODEDATA �� cb_tx_response �ص�����
  *�� ��:	nodedata	:���ڴ���ķ�������
  *�� ��: none
  *�� �� ֵ:JT808_MSG_STATE
  *�� ��:������
  *��������:2013-06-16
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
static JT808_MSG_STATE Cam_jt808_0x0800_response( JT808_TX_NODEDATA * nodedata, uint8_t *pmsg )
{
	JT808_TX_NODEDATA		* iterdata = nodedata;
	TypePicMultTransPara	* p_para;
	TypeDF_PackageHead		TempPackageHead;
	uint32_t				TempAddress;
	uint16_t				temp_msg_id;
	uint16_t				body_len; /*��Ϣ�峤��*/
	uint8_t					* msg;

	if( pmsg == RT_NULL )
	{
		return IDLE;
	}

	temp_msg_id = buf_to_data( pmsg, 2 );
	body_len	= buf_to_data( pmsg + 2, 2 ) & 0x3FF;
	msg			= pmsg + 12;
	if( 0x8001 == temp_msg_id ) ///ͨ��Ӧ��
	{
		if( ( nodedata->head_sn == buf_to_data( msg, 2 ) ) &&
		    ( nodedata->head_id == buf_to_data( msg + 2, 2 ) ) &&
		    ( msg[4] == 0 ) )
		{
			p_para = nodedata->user_para;

			nodedata->state = ACK_OK;
			return ACK_OK;


			/*
			   p_para = nodedata->user_para;
			   rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
			   TempAddress = Cam_Flash_FindPicID( p_para->Data_ID, &TempPackageHead );
			   rt_sem_release( &sem_dataflash );
			   if( TempAddress == 0xFFFFFFFF )
			   {
			   return ACK_OK;
			   }
			   nodedata->size			= TempPackageHead.Len - 64 + 36;
			   nodedata->multipacket	= 1;
			   nodedata->type			= SINGLE_CMD;
			   nodedata->state			= IDLE;
			   nodedata->retry			= 0;
			   nodedata->packet_num	= ( nodedata->size / JT808_PACKAGE_MAX );
			   if( nodedata->size % JT808_PACKAGE_MAX )
			   {
			   nodedata->packet_num++;
			   }
			   nodedata->packet_no			= 0;
			   nodedata->msg_len			= 0;
			   nodedata->head_id			= 0x801;
			   nodedata->head_sn			= 0xF001;
			   nodedata->timeout			= 0;
			   nodedata->cb_tx_timeout		= Cam_jt808_timeout;
			   nodedata->cb_tx_response	= Cam_jt808_0x801_response;

			   return IDLE;
			 */
		}
	}
	return IDLE;
}

/*********************************************************************************
  *��������:void Cam_jt808_0x800(TypeDF_PackageHead *phead)
  *��������:��ý���¼���Ϣ�ϴ�_������Ƭ��ý����Ϣ
  *��	��:	phead	:��Ƭ��Ϣ��Ϣ
  *��	��:	none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2013-06-16
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
rt_err_t Cam_jt808_0x0800( u32 mdeia_id, u8 media_delete )
{
	u8						ptempbuf[32];
	u16						datalen = 0;
	u16						i;
	u32						TempAddress;
	TypePicMultTransPara	* p_para;
	TypeDF_PackageHead		TempPackageHead;
	rt_err_t				rt_ret;
	JT808_TX_NODEDATA		* pnodedata;

	///���Ҷ�ý��ID�Ƿ����
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
	TempAddress = Cam_Flash_FindPicID( mdeia_id, &TempPackageHead );
	rt_sem_release( &sem_dataflash );
	if( TempAddress == 0xFFFFFFFF )
	{
		return RT_ERROR;
	}

	p_para = rt_malloc( sizeof( TypePicMultTransPara ) );
	if( p_para == NULL )
	{
		return RT_ENOMEM;
	}
	///����û�������
	memset( p_para, 0xFF, sizeof( TypePicMultTransPara ) );
	p_para->Address = TempAddress;
	p_para->Data_ID = mdeia_id;
	p_para->Delete	= media_delete;

	datalen += data_to_buf( ptempbuf + datalen, TempPackageHead.Data_ID, 4 );
	datalen += data_to_buf( ptempbuf + datalen, TempPackageHead.Media_Style, 1 );
	datalen += data_to_buf( ptempbuf + datalen, TempPackageHead.Media_Format, 1 );
	datalen += data_to_buf( ptempbuf + datalen, TempPackageHead.TiggerStyle, 1 );
	datalen += data_to_buf( ptempbuf + datalen, TempPackageHead.Channel_ID, 1 );

	pnodedata = node_begin( 1, SINGLE_CMD, 0x0800, -1, 512 + 32 ); /*0x0800�ǵ���*/
	if( pnodedata == RT_NULL )
	{
		rt_free( p_para );
		return RT_ENOMEM;
	}

	pnodedata->size			= TempPackageHead.Len - 64 + 36;
	pnodedata->packet_num	= ( pnodedata->size + JT808_PACKAGE_MAX - 1 ) / JT808_PACKAGE_MAX;
	pnodedata->packet_no	= 0;

	node_data( pnodedata, ptempbuf, datalen );
	node_end( pnodedata, Cam_jt808_timeout, Cam_jt808_0x0800_response, p_para );

	return RT_EOK;
}

/*********************************************************************************
  *��������:void Cam_jt808_0x8801_cam_ok( struct _Style_Cam_Requset_Para *para,uint32_t pic_id )
  *��������:ƽ̨�·�������������Ļص�����_������Ƭ����OK
  *��	��:	para	:���մ���ṹ��
   pic_id	:ͼƬID
  *��	��:	none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2013-06-17
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void Cam_jt808_0x8801_cam_ok( struct _Style_Cam_Requset_Para *para, uint32_t pic_id )
{
	u8	*pdestbuf;
	u16 datalen = 0;

	pdestbuf = (u8*)para->user_para;

	if( ( para->PhotoNum <= para->PhotoTotal ) && ( para->PhotoNum ) && ( para->PhotoNum <= 32 ) )
	{
		datalen = ( para->PhotoNum - 1 ) * 4 + 5;
	}else
	{
		return;
	}
	data_to_buf( pdestbuf + datalen, pic_id, 4 ); ///д��Ӧ����ˮ��
#if 1
	if( para->SendPhoto )
	{
		rt_kprintf( "\n%s pic_id=%d", __func__, pic_id );
		Cam_jt808_0x0801( RT_NULL, pic_id, !para->SavePhoto );
	}
#endif
}

/*********************************************************************************
  *��������:void Cam_jt808_0x8801_cam_end( struct _Style_Cam_Requset_Para *para )
  *��������:ƽ̨�·�������������Ļص�����
  *��	��:	para	:���մ���ṹ��
  *��	��:	none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2013-06-17
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
void Cam_jt808_0x8801_cam_end( struct _Style_Cam_Requset_Para *para )
{
	u8	*pdestbuf;
	u16 datalen;

	pdestbuf = (u8*)para->user_para;

	if( ( para->PhotoNum <= para->PhotoTotal ) && ( para->PhotoNum ) && ( para->PhotoNum <= 32 ) )
	{
		pdestbuf[2] = 0;
		datalen		= para->PhotoNum * 4 + 5;
	}else
	{
		pdestbuf[2] = 1;
		datalen		= 3;
	}
	//data_to_buf( pdestbuf + 3, para->PhotoNum, 2 ); ///д��Ӧ����ˮ��
	pdestbuf[3] = para->PhotoNum >> 8;
	pdestbuf[4] = para->PhotoNum & 0xFF;
	jt808_tx_ack( 0x0805, pdestbuf, datalen );

	rt_free( para->user_para );
	para->user_para = RT_NULL;
	rt_kprintf( "\nCam_jt808_0x8801_cam_end" );
	return;
}

/*********************************************************************************
   *��������:rt_err_t Cam_jt808_0x8801(uint8_t linkno,uint8_t *pmsg)
   *��������:ƽ̨�·������������
   *��	��:	pmsg	:808��Ϣ������
   msg_len	:808��Ϣ�峤��
   *��	��:	none
   *�� �� ֵ:rt_err_t
   *��	��:������
   *��������:2013-06-17
 *--------------------------------------------------------------------------------*/


/*
   7E
   8801 000C 029220613981 0007
   01 0001 0000 00 01 01 7F 01 01 01 957E
 */

rt_err_t Cam_jt808_0x8801( uint8_t linkno, uint8_t *pmsg )
{
	u8						*ptempbuf;
	u8						*pdestbuf;
	u16						datalen;
	u16						i, mediatotal;
	TypeDF_PackageHead		TempPackageHead;
	Style_Cam_Requset_Para	cam_para;
	u32						Tempu32data;
	rt_err_t				ret;
	u16						msg_len;
	msg_len = buf_to_data( pmsg + 2, 2 ) & 0x3FF;

	if( msg_len < 12 )
	{
		return RT_ERROR;
	}
	memset( &cam_para, 0, sizeof( cam_para ) );

	///���ô�������Ϊƽ̨����
	cam_para.TiggerStyle = Cam_TRIGGER_PLANTFORM;

	///ͨ�� ID 1BYTE
	datalen				= 0;
	cam_para.Channel_ID = pmsg[12];
	///�������� 2BYTE
	Tempu32data = ( pmsg[13] << 8 ) | pmsg[14];
	if( ( Tempu32data ) && ( Tempu32data != 0xFFFF ) )
	{
		cam_para.PhotoTotal = Tempu32data;
		if( cam_para.PhotoTotal > 32 )
		{
			cam_para.PhotoTotal = 32;
		}
	}else
	{
		return RT_ERROR;
	}
	///���ռ��/¼��ʱ�� 2BYTE second
	Tempu32data			= ( pmsg[15] << 8 ) | pmsg[16];
	datalen				+= 2;
	cam_para.PhoteSpace = Tempu32data * RT_TICK_PER_SECOND;
	///�����־
	if( pmsg[17] )
	{
		cam_para.SavePhoto	= 1;
		cam_para.SendPhoto	= 0;
	}else
	{
		cam_para.SavePhoto	= 0;
		cam_para.SendPhoto	= 1;
	}
	///���û��ص�������ص����ݲ���
	datalen		= cam_para.PhotoTotal * 4 + 5;  /*5�ֽ�ͷ+4*n*/
	pdestbuf	= rt_malloc( datalen );
	if( pdestbuf == RT_NULL )
	{
		return RT_ERROR;
	}
	memset( pdestbuf, 0, datalen );             ///�������

	pdestbuf[0]			= pmsg[10];
	pdestbuf[1]			= pmsg[11];
	cam_para.user_para	= (void*)pdestbuf;
	///һ����Ƭ���ճɹ��ص�����
	cam_para.cb_response_cam_ok = Cam_jt808_0x8801_cam_ok;
	///������Ƭ���ս����ص�����
	cam_para.cb_response_cam_end = Cam_jt808_0x8801_cam_end;

	///������������
	take_pic_request( &cam_para );
	return RT_EOK;
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
rt_err_t Cam_jt808_0x8802( uint8_t linkno, uint8_t *pmsg )
{
	u8					*ptempbuf;
	u8					*pdestbuf;
	u16					datalen = 0;
	u16					i, mediatotal;
	TypeDF_PackageHead	TempPackageHead;
	u32					TempAddress;
	rt_err_t			ret;

	u16					msg_len;
	u16					fram_num;

	msg_len		= buf_to_data( pmsg + 2, 2 ) & 0x3FF;
	fram_num	= buf_to_data( pmsg + 10, 2 );
	pmsg		+= 12;

	if( pmsg[0] )
	{
		return RT_ERROR;
	}

	ptempbuf = rt_malloc( Cam_get_state( ).Number * 4 );
	if( ptempbuf == NULL )
	{
		return RT_ERROR;
	}

	memset( &TempPackageHead, 0, sizeof( Style_Cam_Requset_Para ) );
	TempPackageHead.Media_Format	= 0;
	TempPackageHead.Media_Style		= 0;
	TempPackageHead.Channel_ID		= pmsg[1];
	TempPackageHead.TiggerStyle		= pmsg[2];
	///���ҷ���������ͼƬ������ͼƬ��ַ����ptempbuf��
	mediatotal = Cam_Flash_SearchPic( mytime_from_bcd( pmsg + 3 ), mytime_from_bcd( pmsg + 9 ), &TempPackageHead, ptempbuf );

	if( mediatotal > ( JT808_PACKAGE_MAX - 4 ) / 35 )
	{
		mediatotal = ( JT808_PACKAGE_MAX - 4 ) / 35;
	}

	pdestbuf = rt_malloc( mediatotal * 35 + 4 );
	if( pdestbuf == NULL )
	{
		rt_free( ptempbuf );
		return RT_ERROR;
	}

	datalen += data_to_buf( pdestbuf + datalen, fram_num, 2 );
	datalen += data_to_buf( pdestbuf + datalen, mediatotal, 2 );
	for( i = 0; i < mediatotal; i++ )
	{
		TempAddress = buf_to_data( ptempbuf, 4 );
		ptempbuf	+= 4;
		sst25_read( TempAddress, (u8*)&TempPackageHead, sizeof( TempPackageHead ) );
		datalen += data_to_buf( pdestbuf + datalen, TempPackageHead.Data_ID, 4 );
		datalen += data_to_buf( pdestbuf + datalen, TempPackageHead.Media_Style, 1 );
		datalen += data_to_buf( pdestbuf + datalen, TempPackageHead.Channel_ID, 1 );
		datalen += data_to_buf( pdestbuf + datalen, TempPackageHead.TiggerStyle, 1 );
		memcpy( pdestbuf + datalen, TempPackageHead.position, 28 ); ///λ����Ϣ�㱨
		datalen += 28;
	}
	jt808_tx_ack( 0x802, pdestbuf, datalen );

	rt_free( ptempbuf );
	rt_free( pdestbuf );
	return ret;
}

/*********************************************************************************
  *��������:rt_err_t Cam_jt808_0x8803(uint8_t *pmsg,u16 msg_len)
  *��������:�洢��ý�������ϴ�
  *��	��:	pmsg	:808��Ϣ������
   msg_len	:808��Ϣ�峤��
  *��	��:none
  *�� �� ֵ:rt_err_t
  *��	��:������
  *��������:2013-06-16
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
rt_err_t Cam_jt808_0x8803( uint8_t linkno, uint8_t *pmsg )
{
	u8					media_delete = 0;
	u8					*ptempbuf;
	u16					datalen = 0;
	u16					i, mediatotal;
	TypeDF_PackageHead	TempPackageHead;
	u32					TempAddress;
	rt_err_t			ret;

	u16					msg_len;
	u16					fram_num;

	msg_len		= buf_to_data( pmsg + 2, 2 ) & 0x3FF;
	fram_num	= buf_to_data( pmsg + 10, 2 );
	pmsg		+= 12;

	if( pmsg[0] )
	{
		return RT_ERROR;
	}
	if( Cam_get_state( ).Number == 0 )
	{
		return RT_ERROR;
	}

	ptempbuf = rt_malloc( Cam_get_state( ).Number * 4 );
	if( ptempbuf == NULL )
	{
		return RT_ERROR;
	}

	memset( &TempPackageHead, 0, sizeof( Style_Cam_Requset_Para ) );
	TempPackageHead.Media_Format	= 0;
	TempPackageHead.Media_Style		= 0;
	TempPackageHead.Channel_ID		= pmsg[1];
	TempPackageHead.TiggerStyle		= pmsg[2];
	media_delete					= pmsg[15];
	///���ҷ���������ͼƬ������ͼƬ��ַ����ptempbuf��
	mediatotal = Cam_Flash_SearchPic( mytime_from_bcd( pmsg + 3 ), mytime_from_bcd( pmsg + 9 ), &TempPackageHead, ptempbuf );
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
	for( i = 0; i < mediatotal; i++ )
	{
		TempAddress = buf_to_data( ptempbuf, 4 );
		ptempbuf	+= 4;
		sst25_read( TempAddress, (u8*)&TempPackageHead, sizeof( TempPackageHead ) );
		Cam_jt808_0x0801( RT_NULL, TempPackageHead.Data_ID, media_delete );
	}
	rt_sem_release( &sem_dataflash );
	rt_free( ptempbuf );
	return RT_EOK;
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
rt_err_t Cam_jt808_0x8805( uint8_t linkno, uint8_t *pmsg )
{
}

/************************************** The End Of File **************************************/

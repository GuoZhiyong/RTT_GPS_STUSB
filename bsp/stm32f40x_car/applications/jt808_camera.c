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

	//�ͷ�ԭ������Դ
	rt_free( iterdata->tag_data );
	iterdata->tag_data	= RT_NULL;
	iterdata->msg_len	= 0;
	//���ҵ�ǰͼƬ�Ƿ����
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	tempu32data = Cam_Flash_FindPicID( p_para->Data_ID, &TempPackageHead );
	if( tempu32data == 0xFFFFFFFF )
	{
		rt_kprintf( "\r\n û���ҵ�ͼƬ��ID=%d", p_para->Data_ID );
		ret = 0xFFFF; goto FUNC_RET;
	}
	for( i = 0; i < iterdata->packet_num; i++ )
	{
		if( p_para->Pack_Mark[i / 8] & BIT( i % 8 ) )
		{
			if( i + 1 < iterdata->packet_num )
			{
				body_len = JT808_PACKAGE_MAX;
			}else
			{
				body_len = iterdata->size - JT808_PACKAGE_MAX * i;
			}
			iterdata->tag_data = rt_malloc( body_len + 16 );
			if( RT_NULL == iterdata->tag_data )
			{
				rt_kprintf( "\r\n Cam_add_tx_pic_getdata rt_malloc error!" );
				ret = 0; goto FUNC_RET;
			}
			iterdata->msg_len	= body_len;                                                             ///�������ݵĳ��ȵ�����Ϣ��ɶ�
			iterdata->packet_no = i + 1;
			iterdata->head_sn	= 0xF001 + i;
			wrlen				= jt808_add_mult_tx_head( iterdata );
			iterdata->msg_len	+= wrlen;                                                               ///�������ݵĳ��ȼ�����Ϣͷ�ĳ���
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
				sst25_read( tempu32data + 64, iterdata->tag_data + wrlen, body_len - wrlen );
			}else
			{
				tempu32data = tempu32data + JT808_PACKAGE_MAX * i + 64 - 36;
				sst25_read( tempu32data, iterdata->tag_data + wrlen, body_len );
			}
			p_para->Pack_Mark[i / 8] &= ~( BIT( i % 8 ) );
			rt_kprintf( "\r\n cam_get_data ok\r\n PAGE=%d", iterdata->packet_no );
			ret = iterdata->packet_no; goto FUNC_RET;
		}
	}
	rt_kprintf( "\r\n cam_get_data_false!" );
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
	u16 cmd_id;
	cmd_id = nodedata->head_id;
	switch( cmd_id )
	{
		case 0x800:
		{
			rt_free( nodedata->user_para );
			nodedata->user_para = RT_NULL;
			break;
		}
		default:
		{
			break;
		}
	}
	rt_kprintf( "\r\n Cam_tx_timeout" );
	return ACK_OK;
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
	uint16_t				body_len; /*��Ϣ�峤��*/
	uint8_t					* msg;
	uint32_t				tempu32data;

	if( NULL == pmsg )
	{
		tempu32data = Cam_add_tx_pic_getdata( nodedata );
		if( tempu32data == 0xFFFF )
		{
			return ACK_OK;
		}else if( tempu32data == 0 )
		{
			return WAIT_ACK;
		}else
		{
			return IDLE;
		}
	}

	temp_msg_id = buf_to_data( pmsg, 2 );
	body_len	= buf_to_data( pmsg + 2, 2 ) & 0x3FF;
	msg			= pmsg + 12;
	if( 0x8001 == temp_msg_id )         ///ͨ��Ӧ��
	{
		if( ( nodedata->head_sn == buf_to_data( msg, 2 ) ) && ( nodedata->head_id == buf_to_data( msg + 2, 2 ) ) && ( msg[4] == 0 ) )
		{
			nodedata->retry = 0;
			return IDLE;
		}
	}else if( 0x8800 == temp_msg_id )   ///ר��Ӧ��
	{
		tempu32data = buf_to_data( msg, 4 );
		msg			+= 4;
		p_para		= (TypePicMultTransPara*)( iterdata->user_para );
		if( tempu32data == p_para->Data_ID )
		{
			memset( p_para->Pack_Mark, 0, sizeof( p_para->Pack_Mark ) );
			if( body_len >= 7 )
			{
				pack_num = *msg++;
				for( i = 0; i < pack_num; i++ )
				{
					tempu32data = buf_to_data( msg, 2 );
					if( tempu32data )
					{
						tempu32data--;
					}
					msg									+= 2;
					p_para->Pack_Mark[tempu32data / 8]	|= BIT( tempu32data % 8 );
				}
				rt_kprintf( "\r\n Cam_jt808_0x801_response\r\n lost_pack=%d", pack_num );
				nodedata->retry = 0;
				return IDLE;
			}else
			{
				if( p_para->Delete )
				{
					//Cam_Flash_DelPic(p_para->Data_ID);
				}
				rt_kprintf( "\r\n Cam_add_tx_pic_response_ok!" );
				return ACK_OK;
			}
		}
	}else
	{
	}
	return nodedata->state;
}

/*********************************************************************************
  *��������:rt_err_t Cam_jt808_0x801(u32 mdeia_id ,u8 media_delete)
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
rt_err_t Cam_jt808_0x0801( u32 mdeia_id, u8 media_delete )
{
	u16						i;
	u32						TempAddress;
	TypePicMultTransPara	* p_para;
	TypeDF_PackageHead		TempPackageHead;
	rt_err_t				rt_ret;

	///���Ҷ�ý��ID�Ƿ����
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	TempAddress = Cam_Flash_FindPicID( mdeia_id, &TempPackageHead );
	rt_sem_release( &sem_dataflash );
	if( TempAddress == 0xFFFFFFFF )
	{
		return RT_ERROR;
	}
	///�����ý��˽����Դ
	p_para = rt_malloc( sizeof( TypePicMultTransPara ) );
	if( p_para == NULL )
	{
		return RT_ERROR;
	}
	memset( p_para, 0xFF, sizeof( TypePicMultTransPara ) );
	///����û�������

	p_para->Address = TempAddress;
	p_para->Data_ID = mdeia_id;
	p_para->Delete	= media_delete;
	//memset(p_para,0xFF,sizeof(p_para->Pack_Mark));

	rt_ret = jt808_add_mult_tx_node( 1,
	                                 SINGLE_CMD,
	                                 0x801,
	                                 TempPackageHead.Len - 64 + 36,
	                                 0xF001,
	                                 Cam_jt808_timeout,
	                                 Cam_jt808_0x0801_response,
	                                 p_para );
	if( rt_ret == RT_EOK )
	{
		rt_kprintf( "\r\n Cam_add_tx_pic_ok!!!" );
	}
	return rt_ret;
}

FINSH_FUNCTION_EXPORT( Cam_jt808_0x0801, Cam_jt808_0x0801 );


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

	printer_data_hex( pmsg, 17 );

	temp_msg_id = buf_to_data( pmsg, 2 );
	body_len	= buf_to_data( pmsg + 2, 2 ) & 0x3FF;
	msg			= pmsg + 12;
	if( 0x8001 == temp_msg_id ) ///ͨ��Ӧ��
	{
		if( ( nodedata->head_sn == buf_to_data( msg, 2 ) ) && ( nodedata->head_id == buf_to_data( msg + 2, 2 ) ) && ( msg[4] == 0 ) )
		{
			p_para = nodedata->user_para;
			rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
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
			nodedata->packet_no = 0;
			nodedata->msg_len	= 0;
			//nodedata->user_para   = p_para;
			nodedata->head_id			= 0x801;
			nodedata->head_sn			= 0xF001;
			nodedata->tag_data			= RT_NULL;
			nodedata->tick				= 0;
			nodedata->timeout			= 0;
			nodedata->cb_tx_timeout		= Cam_jt808_timeout;
			nodedata->cb_tx_response	= Cam_jt808_0x0801_response;

			return IDLE;
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

	///���Ҷ�ý��ID�Ƿ����
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	TempAddress = Cam_Flash_FindPicID( mdeia_id, &TempPackageHead );
	rt_sem_release( &sem_dataflash );
	if( TempAddress == 0xFFFFFFFF )
	{
		return RT_ERROR;
	}
	///�����ý��˽����Դ
	p_para = rt_malloc( sizeof( TypePicMultTransPara ) );
	if( p_para == NULL )
	{
		return RT_ERROR;
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
	//return jt808_tx(0x800,ptempbuf,datalen);
	rt_ret = jt808_add_tx_data( 1,
	                            SINGLE_CMD,
	                            0x800,
	                            datalen,
	                            -1,
	                            Cam_jt808_timeout,
	                            Cam_jt808_0x0800_response,
	                            ptempbuf,
	                            p_para );
	if( rt_ret == RT_EOK )
	{
		rt_kprintf( "\r\n Cam_jt808_0x800" );
	}
	return rt_ret;
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
	rt_kprintf( "\r\n Cam_jt808_0x8801_cam_ok" );

	if( para->SendPhoto )
	{
		Cam_jt808_0x801( pic_id, !para->SavePhoto );
	}
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
		datalen		= 5;
	}
	data_to_buf( pdestbuf + 3, para->PhotoNum, 2 ); ///д��Ӧ����ˮ��

	jt808_tx_ack( 0x805, pdestbuf, datalen );

	rt_kprintf( "\r\n ���ս�������808����:\r\n" );
	printer_data_hex( pdestbuf, datalen );
	rt_kprintf( "\r\n" );

	rt_free( para->user_para );
	para->user_para = RT_NULL;
	rt_kprintf( "\r\nCam_jt808_0x8801_cam_end" );

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
  *---------------------------------------------------------------------------------
  *�� �� ��:
  *�޸�����:
  *�޸�����:
*********************************************************************************/
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
	u16						fram_num;

	msg_len		= buf_to_data( pmsg + 2, 2 ) & 0x3FF;
	fram_num	= buf_to_data( pmsg + 10, 2 );
	pmsg		+= 12;

	if( msg_len < 12 )
	{
		return RT_ERROR;
	}
	memset( &cam_para, 0, sizeof( cam_para ) );

	///���ô�������Ϊƽ̨����
	cam_para.TiggerStyle = Cam_TRIGGER_PLANTFORM;

	///ͨ�� ID 1BYTE
	datalen				= 0;
	cam_para.Channel_ID = pmsg[datalen++];
	///�������� 2BYTE
	Tempu32data = buf_to_data( pmsg + datalen, 2 );
	datalen		+= 2;
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
	Tempu32data			= buf_to_data( pmsg + datalen, 2 );
	datalen				+= 2;
	cam_para.PhoteSpace = Tempu32data * RT_TICK_PER_SECOND;
	///�����־
	if( pmsg[datalen++] )
	{
		cam_para.SavePhoto	= 1;
		cam_para.SendPhoto	= 0;
	}else
	{
		cam_para.SavePhoto	= 0;
		cam_para.SendPhoto	= 1;
	}
	///���û��ص�������ص����ݲ���
	datalen		= cam_para.PhotoTotal * 4 + 5;
	pdestbuf	= rt_malloc( datalen );
	if( pdestbuf == RT_NULL )
	{
		return RT_ERROR;
	}
	memset( pdestbuf, 0, datalen );         ///�������
	data_to_buf( pdestbuf, fram_num, 2 );   ///д��Ӧ����ˮ��
	cam_para.user_para = (void*)pdestbuf;
	///һ����Ƭ���ճɹ��ص�����
	cam_para.cb_response_cam_ok = Cam_jt808_0x8801_cam_ok;
	///������Ƭ���ս����ص�����
	cam_para.cb_response_cam_end = Cam_jt808_0x8801_cam_end;

	///������������
	take_pic_request( &cam_para );
	return RT_EOK;
}

typedef struct _pic_search_param
{
	uint8_t type;   /*����*/
	uint8_t chn;    /*ͨ��*/
	uint8_t event;  /*ʱ�������*/
	MYTIME start;  /*��ʼʱ��*/
	MYTIME end;    /*����ʱ��*/
	MYTIME curr;   /*��ǰҪ������ʱ��*/
}PIC_SEARCH_PARAM;


/*********************************************************************************
  *��������:rt_err_t Cam_jt808_0x8802(uint8_t linkno,uint8_t *pmsg)
  *��������:�洢��ý�����ݼ���
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

	uint8_t				* psrc = pmsg;
	uint16_t			seq, len;
	uint32_t			i;
	MYTIME				start, end;
	PIC_SEARCH_PARAM	* puserdata;
	uint8_t				* pdata;

	i = ( psrc[2] << 8 ) | psrc[3]; /*���ȣ�Ĭ�ϲ��Ƕ��*/
	if( i != 15 )
	{
		return RT_ERROR;
	}

	puserdata = rt_malloc( sizeof( PIC_SEARCH_PARAM ) );
	if( puserdata == RT_NULL )
	{
		rt_free(ptempbuf);
		return RT_ENOMEM;
	}

	puserdata->type	= psrc[12];
	puserdata->chn	= psrc[13];
	puserdata->event = psrc[14];
	puserdata->start = (T_TIMES*)( psrc + 15 );
	puserdata->end	= (T_TIMES*)( psrc + 21 );
	puserdata->curr	= puserdata.start;

	///���ҷ���������ͼƬ������ͼƬ��ַ����ptempbuf��
	mediatotal = Cam_Flash_SearchPic( (T_TIMES*)( pmsg + 3 ), (T_TIMES*)( pmsg + 9 ), &TempPackageHead, ptempbuf );

	if( mediatotal > 16 ) /*ȥ���˶�������*/
	{
		/*ÿ��ͼƬ�� 35�ֽڣ������ݰ�16�� 4���ֽڵ�Ӧ��ͷ*/
		pdata = node_begin( 1, MULTI_ACK, 0x0802, ( psrc[10] << 8 ) | psrc[11], 35 * 16 + 4 );
	}else
	{
		pdata = node_begin( 1, SINGLE_ACK, 0x0802, ( psrc[10] << 8 ) | psrc[11], 35 * 16 + 4 );
	}
	
	if( pdata == RT_NULL )
	{
		rt_free( puserdata );
		return RT_ENOMEM;
	}

	datalen += data_to_buf( pdestbuf + datalen, fram_num, 2 );
	datalen += data_to_buf( pdestbuf + datalen, mediatotal, 2 );
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
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
	rt_sem_release( &sem_dataflash );
	ret = jt808_tx_ack( 0x0802, pdestbuf, datalen );

	rt_free( ptempbuf );
	rt_free( pdestbuf );
	return ret;
}

#if 0


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
	mediatotal = Cam_Flash_SearchPic( (T_TIMES*)( pmsg + 3 ), (T_TIMES*)( pmsg + 9 ), &TempPackageHead, ptempbuf );

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
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
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
	rt_sem_release( &sem_dataflash );
	ret = jt808_tx_ack( 0x802, pdestbuf, datalen );

	rt_free( ptempbuf );
	rt_free( pdestbuf );
	return ret;
}

#endif


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
	mediatotal = Cam_Flash_SearchPic( (T_TIMES*)( pmsg + 3 ), (T_TIMES*)( pmsg + 9 ), &TempPackageHead, ptempbuf );
	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * 2 );
	for( i = 0; i < mediatotal; i++ )
	{
		TempAddress = buf_to_data( ptempbuf, 4 );
		ptempbuf	+= 4;
		sst25_read( TempAddress, (u8*)&TempPackageHead, sizeof( TempPackageHead ) );
		Cam_jt808_0x0801( TempPackageHead.Data_ID, media_delete );
	}
	rt_sem_release( &sem_dataflash );
	rt_free( ptempbuf );
	return RT_EOK;
}

/************************************** The End Of File **************************************/

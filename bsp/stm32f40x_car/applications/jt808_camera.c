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
	u32 Address;        ///地址
	u32 Data_ID;        ///数据ID
	u8	Delete;         ///删除标记
	u8	Pack_Mark[16];  ///包标记
}TypePicMultTransPara;


/*********************************************************************************
  *函数名称:u16 Cam_add_tx_pic_getdata( JT808_TX_NODEDATA * nodedata )
  *功能描述:在jt808_tx_proc当状态为GET_DATA时获取照片数据，该函数是 JT808_TX_NODEDATA 的 get_data 回调函数
  *输 入:	nodedata	:正在处理的发送链表
  *输 出: none
  *返 回 值:rt_err_t
  *作 者:白养民
  *创建日期:2013-06-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
static u16 Cam_add_tx_pic_getdata( JT808_TX_NODEDATA * nodedata )
{
	JT808_TX_NODEDATA		* iterdata	= nodedata;
	TypePicMultTransPara	* p_para	= (TypePicMultTransPara*)nodedata->user_para;
	TypeDF_PackageHead		TempPackageHead;
	uint16_t				i, wrlen, pack_num;
	uint16_t				body_len; /*消息体长度*/
	uint8_t					* msg;
	uint32_t				tempu32data;
	u16						ret = 0;
	uint8_t					* pdata;

	RT_ASSERT( RT_EOK == rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY ) );
	tempu32data = Cam_Flash_FindPicID( p_para->Data_ID, &TempPackageHead );
	if( tempu32data == 0xFFFFFFFF )
	{
		rt_kprintf( "\n 没有找到图片，ID=%d", p_para->Data_ID );
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
			}else /*最后一包*/
			{
				body_len = iterdata->size - JT808_PACKAGE_MAX * i;
			}
			pdata[0]	= 0x08;
			pdata[1]	= 0x01;
			pdata[2]	= 0x20 | ( body_len >> 8 ); /*消息体长度*/
			pdata[3]	= body_len & 0xff;

			iterdata->packet_no = i + 1;
			iterdata->head_sn	= 0xF001 + i;
			pdata[10]			= ( iterdata->head_sn >> 8 );
			pdata[11]			= ( iterdata->head_sn & 0xFF );

			wrlen				= 16;                                                                   /*跳过消息头*/
			iterdata->msg_len	= body_len + 16;                                                        ///发送数据的长度加上消息头的长度
			if( i == 0 )
			{
				sst25_read( tempu32data, (u8*)&TempPackageHead, sizeof( TypeDF_PackageHead ) );
				wrlen	+= data_to_buf( iterdata->tag_data + wrlen, TempPackageHead.Data_ID, 4 );       ///多媒体ID
				wrlen	+= data_to_buf( iterdata->tag_data + wrlen, TempPackageHead.Media_Style, 1 );   ///多媒体类型
				wrlen	+= data_to_buf( iterdata->tag_data + wrlen, TempPackageHead.Media_Format, 1 );  ///多媒体格式编码
				wrlen	+= data_to_buf( iterdata->tag_data + wrlen, TempPackageHead.TiggerStyle, 1 );   ///事件项编码
				wrlen	+= data_to_buf( iterdata->tag_data + wrlen, TempPackageHead.Channel_ID, 1 );    ///通道 ID
				memcpy( iterdata->tag_data + wrlen, TempPackageHead.position, 28 );                     ///位置信息汇报
				wrlen += 28;
				sst25_read( tempu32data + 64, iterdata->tag_data + wrlen, ( 512 - 36 ) );               /*当前把消息头也计算进来了*/
			}else
			{
				tempu32data = tempu32data + JT808_PACKAGE_MAX * i + 64 - 36;
				sst25_read( tempu32data, iterdata->tag_data + wrlen, body_len );
			}
			p_para->Pack_Mark[i / 8] &= ~( BIT( i % 8 ) );
			rt_kprintf( "\n cam_get_data ok PAGE=%d\n", iterdata->packet_no );
			ret = iterdata->packet_no;
			/*调整数据 设置消息头*/
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
  *函数名称:JT808_MSG_STATE Cam_jt808_timeout( JT808_TX_NODEDATA * nodedata )
  *功能描述:发送图片相关数据的超时处理函数
  *输 入:	nodedata	:正在处理的发送链表
  *输 出: none
  *返 回 值:JT808_MSG_STATE
  *作 者:白养民
  *创建日期:2013-06-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
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
		case 0x0800:                                            /*超时以后，直接上报数据*/
			Cam_jt808_0x0801( nodedata, p_para->Data_ID, p_para->Delete );
			break;
		case 0x0801:                                            /*上报图片数据*/
			if( nodedata->packet_no == nodedata->packet_num )   /*都上报完了，还超时*/
			{
				rt_kprintf( "\n全部上报完成\n" );               /*等待应答0x8800*/
				nodedata->state = ACK_OK;

				return WAIT_DELETE;
			}
			ret = Cam_add_tx_pic_getdata( nodedata );
			if( ret == 0xFFFF )                                 /*bitter 没有找到图片id*/
			{
				rt_free( p_para );
				p_para = RT_NULL;
				rt_free( nodedata );
				rt_kprintf( "\n%s有问题", __func__ );
			}
			break;
		default:
			break;
	}
	return IDLE;
}

/*********************************************************************************
  *函数名称:JT808_MSG_STATE Cam_jt808_0x801_response( JT808_TX_NODEDATA * nodedata , uint8_t *pmsg )
  *功能描述:在jt808中处理照片数据传输ACK_ok函数，该函数是 JT808_TX_NODEDATA 的 cb_tx_response 回调函数
  *输 入:	nodedata	:正在处理的发送链表
  *输 出: none
  *返 回 值:JT808_MSG_STATE
  *作 者:白养民
  *创建日期:2013-06-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
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
	uint16_t				ack_seq;                                            /*应答序号*/

	temp_msg_id = buf_to_data( pmsg, 2 );
	p_para=nodedata->user_para;
	
	if( 0x8001 == temp_msg_id )                                                 ///通用应答,有可能应答延时
	{
		ack_seq = ( pmsg[12] << 8 ) | pmsg[13];                                 /*判断应答流水号*/
		if( ack_seq != nodedata->head_sn )                                      /*流水号对应*/
		{
			nodedata->timeout_tick	= rt_tick_get( ) + 15 * RT_TICK_PER_SECOND; /*等15秒*/
			nodedata->state			= WAIT_ACK;
			return WAIT_ACK;
		}
		if( nodedata->packet_no == nodedata->packet_num )                       /*所有数据包上报完成，等待中心下发0x8800*/
		{
			rt_kprintf( "\n等待8800应答\n" );
			nodedata->timeout_tick	= rt_tick_get( ) + 15 * RT_TICK_PER_SECOND; /*等15秒*/
			nodedata->state			= WAIT_ACK;
			nodedata->packet_no++;
			return WAIT_ACK;
		}

		if( nodedata->packet_no > nodedata->packet_num )                        /*超时等待0x8800应答*/
		{
			nodedata->state = ACK_OK;
			return WAIT_DELETE;
		}

		ret = Cam_add_tx_pic_getdata( nodedata );
		if( ret == 0xFFFF )                                                     /*bitter 没有找到图片id*/
		{
			rt_free( p_para );
			p_para = RT_NULL;
			rt_free( nodedata );
			rt_kprintf( "\n%s有问题", __func__ );
			nodedata->state = ACK_OK;
			return WAIT_DELETE;
		}
		nodedata->state = IDLE;                     /*正常发送*/
		return IDLE;
	}

	if( 0x8800 == temp_msg_id )                     ///专用应答
	{
		rt_kprintf( "\n收到专用应答" );
		tempu32data = buf_to_data( pmsg + 12, 4 );  /*应答media ID*/

		if( tempu32data != p_para->Data_ID )
		{
			rt_kprintf( "\n应答ID不正确 %08x %08x",tempu32data,p_para->Data_ID );
			nodedata->state = ACK_OK;
			return WAIT_DELETE;
		}
		if( pmsg[16] == 0 )       /*重传包总数*/
		{
			if( p_para->Delete )
			{
				//Cam_Flash_DelPic(p_para->Data_ID);
			}
			rt_kprintf( "\n单幅图片上报完成" );
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
		rt_kprintf( "\n Cam_jt808_0x801_response\n lost_pack=%d", pmsg[16] );   /*重新获取发送数据*/
		ret = Cam_add_tx_pic_getdata( nodedata );
		if( ret == 0xFFFF )                                                     /*bitter 没有找到图片id*/
		{
			rt_free( p_para );
			p_para = RT_NULL;
			rt_free( nodedata );
			rt_kprintf( "\n%s有问题", __func__ );
			nodedata->state = ACK_OK;
			return WAIT_DELETE;
		}
		nodedata->state=IDLE;
		return IDLE;
	}

	return nodedata->state;
}

/*********************************************************************************
  *函数名称:rt_err_t Cam_jt808_0x0801(u32 mdeia_id ,u8 media_delete)
  *功能描述:添加一个多媒体图片到发送列表中
  *输 入:	mdeia_id	:照片id
   media_delete:照片传送结束后是否删除标记，非0表示删除
  *输 出: none
  *返 回 值:rt_err_t
  *作 者:白养民
  *创建日期:2013-06-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
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

	///查找多媒体ID是否存在

	rt_sem_take( &sem_dataflash, RT_TICK_PER_SECOND * FLASH_SEM_DELAY );
	TempAddress = Cam_Flash_FindPicID( mdeia_id, &TempPackageHead );
	rt_sem_release( &sem_dataflash );

	if( TempAddress == 0xFFFFFFFF )
	{
		rt_kprintf( "\n没有找到图片" );
		return RT_ERROR;
	}

	if( nodedata == RT_NULL )
	{
		///分配多媒体私有资源
		p_para = rt_malloc( sizeof( TypePicMultTransPara ) );
		if( p_para == NULL )
		{
			return RT_ENOMEM;
		}

		///填充用户区数据
		memset( p_para, 0xFF, sizeof( TypePicMultTransPara ) );

		p_para->Address = TempAddress;
		p_para->Data_ID = mdeia_id;
		p_para->Delete	= media_delete;

		pnodedata = node_begin( 1, MULTI_CMD, 0x0801, 0xF001, 512 + 64 );
		if( pnodedata == RT_NULL )
		{
			rt_free( p_para );
			rt_kprintf( "\n分配资源失败" );
			return RT_ENOMEM;
		}
		pnodedata->size			= TempPackageHead.Len - 64 + 36; /*空出了64字节的头部，加上36字节图片信息*/
		pnodedata->packet_num	= ( pnodedata->size + JT808_PACKAGE_MAX - 1 ) / JT808_PACKAGE_MAX;
		pnodedata->packet_no	= 0;
		pnodedata->user_para	= p_para;
	}else
	{
		pnodedata				= nodedata;
		pnodedata->type	= MULTI_CMD;
	}

	ret = Cam_add_tx_pic_getdata( pnodedata );
	if( ret == 0xFFFF ) /*bitter 没有找到图片id*/
	{
		rt_free( p_para );
		p_para = RT_NULL;
		rt_free( pnodedata );
		rt_kprintf( "\n%s有问题", __func__ );
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
  *函数名称:JT808_MSG_STATE Cam_jt808_0x801_response( JT808_TX_NODEDATA * nodedata , uint8_t *pmsg )
  *功能描述:在jt808中处理照片数据传输ACK_ok函数，该函数是 JT808_TX_NODEDATA 的 cb_tx_response 回调函数
  *输 入:	nodedata	:正在处理的发送链表
  *输 出: none
  *返 回 值:JT808_MSG_STATE
  *作 者:白养民
  *创建日期:2013-06-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
*********************************************************************************/
static JT808_MSG_STATE Cam_jt808_0x0800_response( JT808_TX_NODEDATA * nodedata, uint8_t *pmsg )
{
	JT808_TX_NODEDATA		* iterdata = nodedata;
	TypePicMultTransPara	* p_para;
	TypeDF_PackageHead		TempPackageHead;
	uint32_t				TempAddress;
	uint16_t				temp_msg_id;
	uint16_t				body_len; /*消息体长度*/
	uint8_t					* msg;

	if( pmsg == RT_NULL )
	{
		return IDLE;
	}

	temp_msg_id = buf_to_data( pmsg, 2 );
	body_len	= buf_to_data( pmsg + 2, 2 ) & 0x3FF;
	msg			= pmsg + 12;
	if( 0x8001 == temp_msg_id ) ///通用应答
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
  *函数名称:void Cam_jt808_0x800(TypeDF_PackageHead *phead)
  *功能描述:多媒体事件信息上传_发送照片多媒体信息
  *输	入:	phead	:照片信息信息
  *输	出:	none
  *返 回 值:rt_err_t
  *作	者:白养民
  *创建日期:2013-06-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
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

	///查找多媒体ID是否存在
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
	///填充用户区数据
	memset( p_para, 0xFF, sizeof( TypePicMultTransPara ) );
	p_para->Address = TempAddress;
	p_para->Data_ID = mdeia_id;
	p_para->Delete	= media_delete;

	datalen += data_to_buf( ptempbuf + datalen, TempPackageHead.Data_ID, 4 );
	datalen += data_to_buf( ptempbuf + datalen, TempPackageHead.Media_Style, 1 );
	datalen += data_to_buf( ptempbuf + datalen, TempPackageHead.Media_Format, 1 );
	datalen += data_to_buf( ptempbuf + datalen, TempPackageHead.TiggerStyle, 1 );
	datalen += data_to_buf( ptempbuf + datalen, TempPackageHead.Channel_ID, 1 );

	pnodedata = node_begin( 1, SINGLE_CMD, 0x0800, -1, 512 + 32 ); /*0x0800是单包*/
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
  *函数名称:void Cam_jt808_0x8801_cam_ok( struct _Style_Cam_Requset_Para *para,uint32_t pic_id )
  *功能描述:平台下发拍照命令处理函数的回调函数_单张照片拍照OK
  *输	入:	para	:拍照处理结构体
   pic_id	:图片ID
  *输	出:	none
  *返 回 值:rt_err_t
  *作	者:白养民
  *创建日期:2013-06-17
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
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
	data_to_buf( pdestbuf + datalen, pic_id, 4 ); ///写入应答流水号
#if 1
	if( para->SendPhoto )
	{
		rt_kprintf( "\n%s pic_id=%d", __func__, pic_id );
		Cam_jt808_0x0801( RT_NULL, pic_id, !para->SavePhoto );
	}
#endif
}

/*********************************************************************************
  *函数名称:void Cam_jt808_0x8801_cam_end( struct _Style_Cam_Requset_Para *para )
  *功能描述:平台下发拍照命令处理函数的回调函数
  *输	入:	para	:拍照处理结构体
  *输	出:	none
  *返 回 值:rt_err_t
  *作	者:白养民
  *创建日期:2013-06-17
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
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
	//data_to_buf( pdestbuf + 3, para->PhotoNum, 2 ); ///写入应答流水号
	pdestbuf[3] = para->PhotoNum >> 8;
	pdestbuf[4] = para->PhotoNum & 0xFF;
	jt808_tx_ack( 0x0805, pdestbuf, datalen );

	rt_free( para->user_para );
	para->user_para = RT_NULL;
	rt_kprintf( "\nCam_jt808_0x8801_cam_end" );
	return;
}

/*********************************************************************************
   *函数名称:rt_err_t Cam_jt808_0x8801(uint8_t linkno,uint8_t *pmsg)
   *功能描述:平台下发拍照命令处理函数
   *输	入:	pmsg	:808消息体数据
   msg_len	:808消息体长度
   *输	出:	none
   *返 回 值:rt_err_t
   *作	者:白养民
   *创建日期:2013-06-17
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

	///设置触发类型为平台触发
	cam_para.TiggerStyle = Cam_TRIGGER_PLANTFORM;

	///通道 ID 1BYTE
	datalen				= 0;
	cam_para.Channel_ID = pmsg[12];
	///拍摄命令 2BYTE
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
	///拍照间隔/录像时间 2BYTE second
	Tempu32data			= ( pmsg[15] << 8 ) | pmsg[16];
	datalen				+= 2;
	cam_para.PhoteSpace = Tempu32data * RT_TICK_PER_SECOND;
	///保存标志
	if( pmsg[17] )
	{
		cam_para.SavePhoto	= 1;
		cam_para.SendPhoto	= 0;
	}else
	{
		cam_para.SavePhoto	= 0;
		cam_para.SendPhoto	= 1;
	}
	///和用户回调函数相关的数据参数
	datalen		= cam_para.PhotoTotal * 4 + 5;  /*5字节头+4*n*/
	pdestbuf	= rt_malloc( datalen );
	if( pdestbuf == RT_NULL )
	{
		return RT_ERROR;
	}
	memset( pdestbuf, 0, datalen );             ///清空数据

	pdestbuf[0]			= pmsg[10];
	pdestbuf[1]			= pmsg[11];
	cam_para.user_para	= (void*)pdestbuf;
	///一张照片拍照成功回调函数
	cam_para.cb_response_cam_ok = Cam_jt808_0x8801_cam_ok;
	///所有照片拍照结束回调函数
	cam_para.cb_response_cam_end = Cam_jt808_0x8801_cam_end;

	///发送拍照请求
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
	///查找符合条件的图片，并将图片地址存入ptempbuf中
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
		memcpy( pdestbuf + datalen, TempPackageHead.position, 28 ); ///位置信息汇报
		datalen += 28;
	}
	jt808_tx_ack( 0x802, pdestbuf, datalen );

	rt_free( ptempbuf );
	rt_free( pdestbuf );
	return ret;
}

/*********************************************************************************
  *函数名称:rt_err_t Cam_jt808_0x8803(uint8_t *pmsg,u16 msg_len)
  *功能描述:存储多媒体数据上传
  *输	入:	pmsg	:808消息体数据
   msg_len	:808消息体长度
  *输	出:none
  *返 回 值:rt_err_t
  *作	者:白养民
  *创建日期:2013-06-16
  *---------------------------------------------------------------------------------
  *修 改 人:
  *修改日期:
  *修改描述:
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
	///查找符合条件的图片，并将图片地址存入ptempbuf中
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

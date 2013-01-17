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
#include "stm32f4xx.h"


/*
   jt808格式数据解码判断
   计算时减去长度的两个字节，和包头的开销

   <长度2byte><标识0x7e><消息头><消息体><校验码><标识0x7e>

   返回有效的数据长度,为0 表明有错

 */
static uint16_t jt808_decode_fcs( uint8_t *pinfo, uint16_t length )
{
	uint8_t		*psrc, *pdst;
	uint16_t	count	= length - 4, len = length - 4;
	uint8_t		fstuff	= 0; /*是否字节填充*/
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
	psrc	= pinfo + 3; /*2byte长度+1byte标识后为正式信息*/
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
   对数据进行FCS计算或校验
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
   jt808格式数据FCS计算，并编码
   返回编码后的数据长度
 */
static uint16_t jt808_encode( uint8_t * pinfo, uint16_t length )
{
}

/*
   jt808终端发送信息
   并将相关信息注册到接收信息的处理线程中
   需要传递消息ID,和消息体，由jt808_send线程完成
    消息的填充
    发送和重发机制
    流水号
    已发信息的回收free
   传递进来的格式
   <msgid 2bytes><msg_len 2bytes><msgbody nbytes>

 */
static void jt808_send( void* parameter )
{
}

#define ACK_0200 1

/*分析中心下发的ACK信息*/
static uint16_t jt808_analy_ack( uint8_t * pinfo, uint16_t length )
{
}

/*
   分析jt808格式的数据
   <长度2byte><标识0x7e><消息头><消息体><校验码><标识0x7e>

 */
uint16_t jt808_analy( uint8_t * pinfo )
{
	uint8_t		*psrc;

	uint16_t	header_id;          /*消息ID*/
	uint16_t	header_attr;        /*属性*/
	uint8_t		header_mobile[6];   /*BCD 手机号*/
	uint16_t	header_seq;         /*流水号*/
	uint16_t	msg_len;            /*消息长度*/

	uint16_t	msg_seq;            /*收到消息中的流水号*/
	uint16_t	msg_id;             /*收到消息中的命令id*/
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
/*分包处理*/
	if( header_attr & 0x20 )
	{
	}
	msg_len = header_attr & 0x3ff;
	switch( header_id )
	{
		case 0x8001:            /*平台通用应答*/


			/* 若没有分包处理的话  消息头长12  从0开始计算第12个字节是消息体得主体
			   13 14	对应的终端消息流水号
			   15 16	对应终端的消息
			 */
			msg_id = ( *( psrc + 14 ) << 8 ) + *( psrc + 15 );

			switch( msg_id )    // 判断对应终端消息的ID做区分处理
			{
				case 0x0200:    //  对应位置消息的应答
					rt_kprintf( "\r\nCentre ACK!\r\n" );
					break;
				case 0x0002:    //  心跳包的应答
					rt_kprintf( "\r\n  Centre  Heart ACK!\r\n" );
					break;
				case 0x0101:    //  终端注销应答
					break;
				case 0x0102:    //  终端鉴权
					break;
				case 0x0800:    // 多媒体事件信息上传
					break;
				case 0x0702:
					rt_kprintf( "\r\n  驾驶员信息上报---中心应答!  \r\n" );
					break;
				case 0x0701:
					rt_kprintf( "\r\n	电子运单上报---中心应答!  \r\n");
					break;
				default:
					break;
			}
			break;
		case 0x8100:    //  监控中心对终端注册消息的应答
			break;
		case 0x8103:    //  设置终端参数
			break;
		case 0x8104:    //  查询终端参数
			break;
		case 0x8105:    // 终端控制
			break;
		case 0x8201:    // 位置信息查询    位置信息查询消息体为空
			break;
		case 0x8202:    // 临时位置跟踪控制
			break;
		case 0x8300:    //  文本信息下发
			break;
		case 0x8301:    //  事件设置
			break;
		case 0x8302:    // 提问下发
			break;
		case 0x8303:    //  信息点播菜单设置
			break;
		case 0x8304:    //  信息服务
			break;
		case 0x8400:    //  电话回拨
			break;
		case 0x8401:    //   设置电话本
			break;
		case 0x8500:    //  车辆控制
			break;
		case 0x8600:    //  设置圆形区域
			break;
		case 0x8601:    //  删除圆形区域
			break;
		case 0x8602:    //  设置矩形区域
			break;
		case 0x8603:    //  删除矩形区域
			break;
		case 0x8604:    //  多边形区域
			break;
		case 0x8605:    //  删除多边区域
			break;
		case 0x8606:    //  设置路线
			break;
		case 0x8607:    //  删除路线
			break;
		case 0x8700:    //  行车记录仪数据采集命令
			break;
		case 0x8701:    //  行驶记录仪参数下传命令
			break;
		case 0x8800:    //	多媒体数据上传应答
			break;
		case 0x8801:    //	摄像头立即拍照
			break;
		case 0x8802:    //	存储多媒体数据检索
			break;
		case 0x8803:    //	存储多媒体数据上传命令
			break;
		case 0x8804:    //	录音开始命令
			break;
		case 0x8805:    //	单条存储多媒体数据检索上传命令 ---- 补充协议要求
			break;
		case 0x8900:    //	数据下行透传
			break;
		case 0x8A00:    //	平台RSA公钥
			break;
		default:
			break;
	}
}

/************************************** The End Of File **************************************/

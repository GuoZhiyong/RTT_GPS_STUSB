/************************************************************
 * Copyright (C), 2008-2012,
 * FileName:		//  jt808_sms.c
 * Author:			//  baiyangmin
 * Date:			//  2013-07-08
 * Description:		//  短信处理及发送，接收，修改参数等功能
 * Version:			//  V0.01
 * Function List:	//  主要函数及其功能
 *     1. -------
 * History:			//  历史修改记录
 *     <author>  <time>   <version >   <desc>
 *     David    96/10/12     1.0     build this moudle
 ***********************************************************/

#include <rtthread.h>
#include <rthw.h>
#include "stm32f4xx.h"
#include <finsh.h>

#include  <stdlib.h> //数字转换成字符串
#include  <stdio.h>
#include  <string.h>
#include  "jt808_sms.h"
#include "jt808.h"
#include "jt808_param.h"
#include "jt808_gps.h"
#include "m66.h"
#include "gps.h"

#define HALF_BYTE_SWAP( a ) ( ( ( ( a ) & 0x0f ) << 4 ) | ( ( a ) >> 4 ) )
#define GSM_7BIT	0x00
#define GSM_UCS2	0x08

static const char	sms_gb_data[]	= "冀京津沪宁渝琼藏川粤青贵闽吉陕蒙晋甘桂鄂赣浙苏新鲁皖湘黑辽云豫";
static uint16_t		sms_ucs2[31]	= { 0x5180, 0x4EAC, 0x6D25, 0x6CAA, 0x5B81, 0x6E1D, 0x743C, 0x85CF, 0x5DDD, 0x7CA4, 0x9752,
	                                    0x8D35,		   0x95FD, 0x5409, 0x9655, 0x8499, 0x664B, 0x7518, 0x6842, 0x9102, 0x8D63,
	                                    0x6D59,		   0x82CF, 0x65B0, 0x9C81, 0x7696, 0x6E58, 0x9ED1, 0x8FBD, 0x4E91, 0x8C6B, };

static char			sender[32];
static char			smsc[32];

/**/
u16  gsmencode8bit( const char *pSrc, char *pDst, u16 nSrcLength )
{
	u16 m;
	// 简单复制
	for( m = 0; m < nSrcLength; m++ )
	{
		*pDst++ = *pSrc++;
	}

	return nSrcLength;
}

/**/
static uint16_t ucs2_to_gb2312( uint16_t uni )
{
	uint8_t i;
	for( i = 0; i < 31; i++ )
	{
		if( uni == sms_ucs2[i] )
		{
			return ( sms_gb_data[i * 2] << 8 ) | sms_gb_data[i * 2 + 1];
		}
	}
	return 0xFF20;
}

/**/
static uint16_t gb2312_to_ucs2( uint16_t gb )
{
	uint8_t i;
	for( i = 0; i < 31; i++ )
	{
		if( gb == ( ( sms_gb_data[i * 2] << 8 ) | sms_gb_data[i * 2 + 1] ) )
		{
			return sms_ucs2[i];
		}
	}
	return 0;
}

/*ucs2过来的编码*/
u16 gsmdecodeucs2( char* pSrc, char* pDst, u16 nSrcLength )
{
	u16			nDstLength = 0; // UNICODE宽字符数目
	u16			i;
	uint16_t	uni, gb;
	char		* src	= pSrc;
	char		* dst	= pDst;

	for( i = 0; i < nSrcLength; i += 2 )
	{
		uni = ( src[i] << 8 ) | src[i + 1];
		if( uni > 0x80 ) /**/
		{
			gb			= ucs2_to_gb2312( uni );
			*dst++		= ( gb >> 8 );
			*dst++		= ( gb & 0xFF );
			nDstLength	+= 2;
		}else /*也是双字节的 比如 '1' ==> 0031*/
		{
			*dst++ = uni;
			nDstLength++;
		}
	}
	return nDstLength;
}

/**/
u16 gsmencodeucs2( u8* pSrc, u8* pDst, u16 nSrcLength )
{
	uint16_t	len = nSrcLength;
	uint16_t	gb, ucs2;
	uint8_t		*src	= pSrc;
	uint8_t		*dst	= pDst;

	while( len )
	{
		gb = *src++;
		if( gb > 0x7F )
		{
			gb		|= *src++;
			ucs2	= gb2312_to_ucs2( gb ); /*有可能没有找到返回00*/
			*dst++	= ( ucs2 >> 8 );
			*dst++	= ( ucs2 & 0xFF );
			len		-= 2;
		}else
		{
			*dst++	= 0x00;
			*dst++	= ( gb & 0xFF );
			len--;
		}
	}
	// 返回目标编码串长度
	return ( nSrcLength << 1 );
}

/**/
u16 gsmdecode7bit( char* pSrc, char* pDst, u16 nSrcLength )
{
	u16 nSrc;   // 源字符串的计数值
	u16 nDst;   // 目标解码串的计数值
	u16 nByte;  // 当前正在处理的组内字节的序号，范围是0-6
	u8	nLeft;  // 上一字节残余的数据

	// 计数值初始化
	nSrc	= 0;
	nDst	= 0;

	// 组内字节序号和残余数据初始化
	nByte	= 0;
	nLeft	= 0;

	// 将源数据每7个字节分为一组，解压缩成8个字节
	// 循环该处理过程，直至源数据被处理完
	// 如果分组不到7字节，也能正确处理
	while( nSrc < nSrcLength )
	{
		*pDst	= ( ( *pSrc << nByte ) | nLeft ) & 0x7f;    // 将源字节右边部分与残余数据相加，去掉最高位，得到一个目标解码字节
		nLeft	= *pSrc >> ( 7 - nByte );                   // 将该字节剩下的左边部分，作为残余数据保存起来
		pDst++;                                             // 修改目标串的指针和计数值
		nDst++;
		nByte++;                                            // 修改字节计数值
		if( nByte == 7 )                                    // 到了一组的最后一个字节
		{
			*pDst = nLeft;                                  // 额外得到一个目标解码字节
			pDst++;                                         // 修改目标串的指针和计数值
			nDst++;
			nByte	= 0;                                    // 组内字节序号和残余数据初始化
			nLeft	= 0;
		}
		pSrc++;                                             // 修改源串的指针和计数值
		nSrc++;
	}
	*pDst = 0;
	return nDst;                                            // 返回目标串长度
}


/*信息编码为7bit pdu模式,并发送*/


/***********************************************************
* Function:
* Description:
* Input:
* Output:
* Return:
* Others:
***********************************************************/
void sms_encode_pdu_7bit( char* info )
{
#define BITMASK_7BITS 0x7F

	char			buf[200];
	uint16_t		len;
	char			*pSrc, *pDst;
	unsigned char	c;
	char			ASC[]	= "0123456789ABCDEF";
	uint8_t			count	= 0;

	unsigned char	nSrc	= 0, nDst = 0, nChar = 0;
	uint8_t			nLeft	= 0;

	len = strlen( info );
	rt_kprintf( "\nSMS>ENCODE(%d):%s", len, info );

	//strcpy( buf, "0891683108200205F01100" );
	strcpy( buf, "001100" );
	count = strlen( sender );
	strcat( buf, sender );
	strcat( buf, "000000" );                                //	7bit编码

	pSrc	= info;
	pDst	= buf + ( count + 12 + 2 );                     //sender长度+12ASC+2预留TP长度

	while( nSrc < ( len + 1 ) )
	{
		nChar = nSrc & 0x07;                                // 取源字符串的计数值的最低3位
		if( nChar == 0 )                                    // 处理源串的每个字节
		{
			nLeft = *pSrc;                                  // 组内第一个字节，只是保存起来，待处理下一个字节时使用
		}else
		{
			c		= ( *pSrc << ( 8 - nChar ) ) | nLeft;   // 组内其它字节，将其右边部分与残余数据相加，得到一个目标编码字节
			*pDst++ = ASC[c >> 4];
			*pDst++ = ASC[c & 0x0f];
			nLeft	= *pSrc >> nChar;                       // 将该字节剩下的左边部分，作为残余数据保存起来
			nDst++;
		}
		pSrc++;
		nSrc++;                                             // 修改源串的指针和计数值
	}
	*pDst = 0;
	//buf[count + 28] = ASC[len >> 4];
	//buf[count + 29] = ASC[len & 0x0F];
	buf[count + 12] = ASC[len >> 4];
	buf[count + 13] = ASC[len & 0x0F];

	rt_kprintf( "\nSMS>encode 7BIT:%s", buf );
	//sms_tx( buf, ( strlen( buf ) - 18 ) >> 1 );
	sms_tx( buf, ( strlen( buf ) - 2 ) >> 1 );
}

/*信息编码为uscs2 pdu模式,并发送*/

void sms_encode_pdu_ucs2( char* info )
{
	char		buf[400];
	uint16_t	len, gb, ucs2;
	char		*pSrc, *pDst;
	char		ASC[]	= "0123456789ABCDEF";
	uint8_t		count	= 0;
	uint8_t		TP_UL	= 0;

	len = strlen( info );
	rt_kprintf( "\nSMS>ENCODE(%d):%s", len, info );

//	strcpy(buf,smsc,strlen(smsc));
	strcpy( buf, "001100" );
	count = strlen( sender );
	strcat( buf, sender );
	strcat( buf, "00088F" );
//	ucs2bit编码
	pSrc	= info;
	pDst	= buf + ( count + 12 + 2 ); //sender长度+12ASC+2预留TP长度
	while( *pSrc )
	{
		gb = *pSrc++;
		if( gb > 0x7F )
		{
			gb		<<= 8;
			gb		|= *pSrc++;
			ucs2	= gb2312_to_ucs2( gb ); /*有可能没有找到返回00*/
			*pDst++ = ASC[( ucs2 & 0xF000 ) >> 12];
			*pDst++ = ASC[( ucs2 & 0x0F00 ) >> 8];
			*pDst++ = ASC[( ucs2 & 0x00F0 ) >> 4];
			*pDst++ = ASC[( ucs2 & 0x000F )];
		}else
		{
			*pDst++ = '0';
			*pDst++ = '0';
			*pDst++ = ASC[gb >> 4];
			*pDst++ = ASC[gb & 0x0F];
		}
		TP_UL += 2;
	}
	*pDst			= 0;
	buf[count + 12] = ASC[TP_UL >> 4];
	buf[count + 13] = ASC[TP_UL & 0x0F];

	//rt_kprintf( "\nSMS>encode UCS2:%s", buf );
	sms_tx( buf, ( strlen( buf ) - 2 ) >> 1 );
}

/*输入命令,按要求返回
   对于查询命令直接发送 返回0，不需要应答或内部已应答
   其他命令返回1,交由调用方处理,需要通用应答


 */
uint8_t analy_param( char* cmd, char*value )
{
	char buf[160];
	char *ip;
	int port;
	
	if( strlen( cmd ) == 0 )
	{
		return 0;
	}
	rt_kprintf( "\nSMS>cmd=%s,value=%s", cmd, value );
/*先找不带参数的*/
	if( strncmp( cmd, "TIREDCLEAR", 10 ) == 0 )
	{
		return 1;
	}
	if( strncmp( cmd, "DISCLEAR", 8 ) == 0 )
	{
		return 1;
	}
	if( strncmp( cmd, "RESET", 5 ) == 0 )               /*要求复位,发送完成后复位，如何做?*/
	{
		reset(0xaa);
		return 1;
	}else if( strncmp( cmd, "CLEARREGIST", 11 ) == 0 )  /*清除注册*/
	{
		memset( jt808_param.id_0xF003, 0, 32 );         /*清除鉴权码*/
		return 1;
	}


/*带参数的，先判有没有参数*/
	if( strlen( value ) == 0 )
	{
		return 0;
	}
/*查找对应的参数*/
	if( strncmp( cmd, "DNSR", 4 ) == 0 )
	{
		if( cmd[4] == '1' )
		{
			strcpy( jt808_param.id_0x0013, value );
		}
		if( cmd[4] == '2' )
		{
			strcpy( jt808_param.id_0x0017, value );
		}
		return 1;
	}
	if( strncmp( cmd, "PORT", 4 ) == 0 )
	{
		if( cmd[4] == '1' )
		{
			jt808_param.id_0x0018=atoi( value );
		}
		if( cmd[4] == '2' )
		{
			jt808_param.id_0x0019=atoi( value );
		}
		return 1;
	}
	if( strncmp( cmd, "IP", 2 ) == 0 )
	{
		if( cmd[2] == '1' )
		{
			strcpy( jt808_param.id_0x0013, value );
		}
		if( cmd[2] == '2' )
		{
			strcpy( jt808_param.id_0x0017, value );
		}
		return 1;
	}
	if( strncmp( cmd, "DUR", 3 ) == 0 )
	{
		jt808_param.id_0x0029=atoi(value);
		return 1;
	}
	if( strncmp( cmd, "SIMID", 6 ) == 0 )
	{
		strcpy( jt808_param.id_0xF006, value );
		return 1;
	}
	if( strncmp( cmd, "DEVICEID", 8 ) == 0 )
	{
		strcpy( jt808_param.id_0xF002, value ); /*终端ID 大写字母+数字*/
		return 1;
	}
	if( strncmp( cmd, "MODE", 4 ) == 0 )        ///6. 设置定位模式
	{
		if( strncmp( value, "BD", 2 ) == 0 )
		{
			gps_status.mode = MODE_BD;
		}
		if( strncmp( value, "GP", 2 ) == 0 )
		{
			gps_status.mode = MODE_GPS;
		}
		if( strncmp( value, "GN", 2 ) == 0 )
		{
			gps_status.mode= MODE_BDGPS;
		}
		gps_mode( gps_status.mode );
		return 1;
	}
	if( strncmp( cmd, "VIN", 3 ) == 0 )
	{
		strcpy( jt808_param.id_0xF005, value ); /*VIN(TJSDA09876723424243214324)*/
		return 1;
	}
	/*	RELAY(0)关闭继电器 继电器通 RELAY(1) 断开继电器 继电器断*/
	if( strncmp( cmd, "RELAY", 5 ) == 0 )
	{
		return 1;
	}
	if( strncmp( cmd, "TAKE", 4 ) == 0 )    /*拍照 1.2.3.4路*/
	{
		return 1;
	}
	if( strncmp( cmd, "PLAY", 4 ) == 0 )    /*语音播报*/
	{
		tts_write( value, strlen( value ) );
		return 1;
	}
	if( strncmp( cmd, "QUERY", 5 ) == 0 )   /*0 车辆相关信息 1 网络系统配置信息 2位置信息*/
	{
		/*直接发送信息*/
		if( *value == '0' )
		{
			sprintf( buf, "TCBTW703#%s#%s#%s#%d",
			         jt808_param.id_0x0083 + 2,
			         jt808_param.id_0xF002,
			         jt808_param.id_0xF005,
			         jt808_param.id_0x0029 );
			sms_encode_pdu_7bit( buf );
		}

		if( *value == '1' )
		{
			sprintf( buf, "TCBTW703#%s#%s#%s#%d",
			         jt808_param.id_0x0083 + 2,
			         jt808_param.id_0xF002,
			         jt808_param.id_0x0013,
			         jt808_param.id_0x0018);
			sms_encode_pdu_7bit( buf );
		}
		if( *value == '2' )
		{
			sprintf( buf, "TCBTW703#%s#%s#%s#%d",
			         jt808_param.id_0x0083 + 2,
			         jt808_param.id_0xF002,
			         jt808_param.id_0xF005,
			         jt808_param.id_0x0029 );
			sms_encode_pdu_7bit( buf );
		}

		return 0;
	}
	if( strncmp( cmd, "ISP", 3 ) == 0 )         /*	ISP(202.89.23.210：9000)*/
	{
		sscanf(value,"%s:%d",ip,&port);
		gsm_socket[2].index=2;
		strcpy(gsm_socket[2].ipstr,ip);
		gsm_socket[2].port=port;
		gsm_socket[2].active=1;
		return 1;
	}
	if( strncmp( cmd, "PLATENUM", 8 ) == 0 )    /*PLATENUM(津A8888)	*/
	{
		strcpy(jt808_param.id_0x0083,value);
		param_save();
		return 0x80;
	}
	if( strncmp( cmd, "COLOR", 5 ) == 0 )       /* COLOR(0) 1： 蓝色  2： 黄色    3： 黑色    4： 白色    9：  其他*/
	{
		jt808_param.id_0x0084=atoi(value);
		param_save();
		return 1;
	}
	if( strncmp( cmd, "CONNECT", 7 ) == 0 )     /*CONNECT(0)  0:  DNSR   优先     1：  MainIP   优先 */
	{
		return 1;
	}
	if( strncmp( cmd, "PASSWORD", 7 ) == 0 )    /*PASSWORD（0） 1： 通过   0： 不通过*/
	{
		return 1;
	}
	return 0;
}

/*收到短信息处理*/
void jt808_sms_rx( char *info, uint16_t size )
{
	uint16_t	i;
	uint8_t		tbl[24] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf };
	char		*p, *pdst;
	char		c, pid;
	uint16_t	len;
	uint8_t		pdu_type, msg_len;
	uint8_t		dcs;

	char		decode_msg[180];    /*解码后的信息*/
	uint16_t	decode_msg_len = 0;
	char		tmpbuf[180];        /*解码后的信息*/

	char		cmd[64];
	char		value[64];
	uint8_t		count;
	char		*psave;

	memset( sender, 0, 64 );
	memset( smsc, 0, 64 );
	p = info;                       /*重新定位到开始*/
	/*SCA*/
	len = tbl[p[0] - '0'] << 4;
	len |= tbl[p[1] - '0'];         /*服务中心号码 Length (长度)+Tosca（服务中心类型）+Address（地址）*/
	len <<= 1;                      /*转成双字*/
	for( i = 0; i < len + 2; i++ )  /*包括长度*/
	{
		smsc[i] = *p++;
	}
	/*pdu类型*/
	pdu_type	= tbl[*p++ - '0'] << 4;
	pdu_type	|= tbl[*p++ - '0'];

	/*OA来源地址 05 A1 0180F6*/
	len = tbl[p[0] - '0'] << 4;
	len |= tbl[p[1] - '0']; /*手机发送源地址 Length (长度,是数字个数)+Tosca（地址类型）+Address（地址）*/
	if( len & 0x01 )
	{
		len++;              /*这里是数字位数，比如10086是5位 用三个字节表示*/
	}

	for( i = 0; i < len + 4; i++ )
	{
		sender[i] = *p++;   /*这里没有半字节交互，发送的时候还得换过来*/
	}

	/*PID标志*/
	pid = tbl[*p++ - '0'] << 4;
	pid |= tbl[*p++ - '0'];
	/*DCS编码*/
	dcs = tbl[*p++ - '0'] << 4;
	dcs |= tbl[*p++ - '0'];
	/*SCTS服务中心时间戳 yymmddhhmmsszz 包含时区*/
	p += 14;
	/*消息长度*/
	msg_len = tbl[*p++ - '0'] << 4;
	msg_len |= tbl[*p++ - '0'];
	/*是否为长信息pdu_type 的bit6 UDHI=1*/
	if( pdu_type & 0x40 )           /*跳过长信息的头*/
	{
		len		= tbl[*p++ - '0'] << 4;
		len		|= tbl[*p++ - '0'];
		p		+= ( len >> 1 );    /*现在还是OCT模式*/
		msg_len -= ( len + 1 );     /*长度占一个字节*/
	}
	rt_kprintf( "\nSMS>PDU Type=%02x PID=%02x DCS=%02x TP-UDL=%d", pdu_type, pid, dcs, msg_len );

/*直接在info上操作有误?*/
	pdst = tmpbuf;
	/*将OCT数据转为HEX,两个字节拼成一个*/
	for( i = 0; i < msg_len; i++ )
	{
		c		= tbl[*p++ - '0'] << 4;
		c		|= tbl[*p++ - '0'];
		*pdst++ = c;
	}
	*pdst = 0;

	if( ( dcs & 0x0c ) == GSM_7BIT )
	{
		decode_msg_len = gsmdecode7bit( tmpbuf, decode_msg, msg_len );
	}else if( ( dcs & 0x0c ) == GSM_UCS2 )
	{
		decode_msg_len = gsmdecodeucs2( tmpbuf, decode_msg, msg_len );
	}else
	{
		memcpy( decode_msg, tmpbuf, msg_len );
		decode_msg_len = msg_len;   /*简单拷贝即可*/
	}

	if( decode_msg_len == 0 )       /*对于不是合法的数据是不是直接返回0*/
	{
		return;
	}
	decode_msg[decode_msg_len] = 0;

/*decode_msg保存解码后的信息*/
	p = decode_msg;
	rt_kprintf( "\nSMS(%dbytes)>%s", decode_msg_len, p );
	if( strncmp( p, "TW703", 5 ) != 0 )
	{
		return;
	}
	p += 5;
	memset( cmd, 0, 64 );
	memset( value, 0, 64 );
	count	= 0;
	psave	= cmd;
	i		= 0;
/*tmpbuf用作应答的信息 TW703#*/
	while( *p != 0 )
	{
		switch( *p )
		{
			case '#':
				i += analy_param( cmd, value );
				memset( cmd, 0, 64 );
				memset( value, 0, 64 );
				psave	= cmd;      /*先往cmd缓冲中存*/
				count	= 0;
				break;
			case '(':
				psave	= value;    /*向取值区保存*/
				count	= 0;
				break;
			case ')':               /*表示结束,不保存*/
				break;
			default:
				if( count < 64 )    /*正常，保存*/
				{
					*psave++ = *p;
					count++;
				}
				break;
		}
		p++;
	}
	i += analy_param( cmd, value );
	
	if( i > 0x7F ) /*要返回信息,有车牌*/
	{
		sprintf( tmpbuf, "%s#%-7s#%s", jt808_param.id_0x0083 + 2, jt808_param.id_0xF002, decode_msg + 6 );
		sms_encode_pdu_ucs2( tmpbuf );
		param_save();
	}else if( i > 0 )
	{
		sprintf( tmpbuf, "%s#%-7s#%s", jt808_param.id_0x0083 + 2, jt808_param.id_0xF002, decode_msg + 6 );
		sms_encode_pdu_7bit( tmpbuf );
		param_save();		/*修改参数*/
	}
}

/*
   0891683108200205F0000D91683106029691F10008318011216584232C0054005700370030003300230050004C004100540045004E0055004D00286D25005400540036003500320029
   0891683108200205F0000D91683106029691F100003180112175222332D4EB0D361B0D9F4E67714845C15223A2732A8DA1D4F498EB7C46E7E17497BB4C4F8DA04F293586BAC160B814

   在线验证

   http://www.diafaan.com/sms-tutorials/gsm-modem-tutorial/online-sms-submit-pdu-decoder/


 */


#if 1

void sms_test( uint8_t index )
{
	char *s[2] =
	{ "0891683108200205F0000D91683106029691F10008318011216584232C0054005700370030003300230050004C004100540045004E0055004D00286D25005400540036003500320029",
	  "0891683108200205F0000D91683106029691F100003180112175222332D4EB0D361B0D9F4E67714845C15223A2732A8DA1D4F498EB7C46E7E17497BB4C4F8DA04F293586BAC160B814" };
	jt808_sms_rx( s[index], strlen( s[index] ) );
}

FINSH_FUNCTION_EXPORT( sms_test, test sms );


void oiap_test(char *ip,uint16_t port)
{
	gsm_socket[2].index=2;
	strcpy(gsm_socket[2].ipstr,ip);
	gsm_socket[2].port=port;
	gsm_socket[2].active=1;
}
FINSH_FUNCTION_EXPORT( oiap_test, conn onair iap );

#endif

/************************************** The End Of File **************************************/

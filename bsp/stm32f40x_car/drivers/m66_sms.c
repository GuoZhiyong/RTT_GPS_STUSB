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
#include "m66_sms.h"
#include <stdio.h>
#include <string.h>


/* 消息邮箱控制块*/
static struct rt_mailbox mb_sms_rx;
#define MB_SMS_RX_POOL_SIZE 32
/* 消息邮箱中用到的放置消息的内存池*/
static uint8_t		mb_sms_rx_pool[MB_SMS_RX_POOL_SIZE];



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

static const char	GB_DATA[]	= "京津沪宁渝琼藏川粤青贵闽吉陕蒙晋甘桂鄂赣浙苏新鲁皖湘黑辽云豫冀";
static uint16_t		ucs2[31]	= { 0x4EAC, 0x6D25, 0x6CAA, 0x5B81, 0x6E1D, 0x743C, 0x85CF, 0x5DDD, 0x7CA4, 0x9752,
	                                0x8D35,		   0x95FD, 0x5409, 0x9655, 0x8499, 0x664B, 0x7518, 0x6842, 0x9102, 0x8D63,
	                                0x6D59,		   0x82CF, 0x65B0, 0x9C81, 0x7696, 0x6E58, 0x9ED1, 0x8FBD, 0x4E91, 0x8C6B,
	                                0x5180 };

/**/
static uint16_t ucs2_to_gb2312( uint16_t uni )
{
	uint8_t i;
	for( i = 0; i < 31; i++ )
	{
		if( uni == ucs2[i] )
		{
			return ( GB_DATA[i * 2] << 8 ) | GB_DATA[i * 2 + 1];
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
		if( gb == ( ( GB_DATA[i * 2] << 8 ) | GB_DATA[i * 2 + 1] ) )
		{
			return ucs2[i];
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
			gb = ucs2_to_gb2312( uni );
			*dst++		= ( gb >> 8 );
			*dst++		= ( gb & 0xFF );
			nDstLength	+= 2;
		}else   /*也是双字节的 比如 '1' ==> 0031*/
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
	uint16_t len=nSrcLength;
	uint16_t gb,ucs2;
	uint8_t *src=pSrc;
	uint8_t *dst=pDst;
	
	while(len)
	{
		gb=*src++;
		if(gb>0x7F)
		{
			gb|=*src++;
			ucs2=gb2312_to_ucs2(gb); /*有可能没有找到返回00*/
			*dst++=(ucs2>>8);
			*dst++=(ucs2&0xFF);
			len-=2;
		}
		else
		{
			*dst++=0x00;
			*dst++=(gb&0xFF);
			len--;
		}
	}
	// 返回目标编码串长度
	return ( nSrcLength << 1 );
}

/**/
u16 gsmdecode7bit( const u8* pSrc, u8* pDst, u16 nSrcLength )
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
		// 将源字节右边部分与残余数据相加，去掉最高位，得到一个目标解码字节
		*pDst = ( ( *pSrc << nByte ) | nLeft ) & 0x7f;
		// 将该字节剩下的左边部分，作为残余数据保存起来
		nLeft = *pSrc >> ( 7 - nByte );

		// 修改目标串的指针和计数值
		pDst++;
		nDst++;
		// 修改字节计数值
		nByte++;

		// 到了一组的最后一个字节
		if( nByte == 7 )
		{
			// 额外得到一个目标解码字节
			*pDst = nLeft;

			// 修改目标串的指针和计数值
			pDst++;
			nDst++;

			// 组内字节序号和残余数据初始化
			nByte	= 0;
			nLeft	= 0;
		}

		// 修改源串的指针和计数值
		pSrc++;
		nSrc++;
	}

	*pDst = 0;

	// 返回目标串长度
	return nDst;
}

//将每个ascii8位编码的Bit8去掉，依次将下7位编码的后几位逐次移到前面，形成新的8位编码。
u16 gsmencode7bit( const u8* pSrc, u8* pDst, u16 nSrcLength )
{
	u16 nSrc;   // 源字符串的计数值
	u16 nDst;   // 目标编码串的计数值
	u16 nChar;  // 当前正在处理的组内字符字节的序号，范围是0-7
	u8	nLeft;  // 上一字节残余的数据

	// 计数值初始化
	nSrc	= 0;
	nDst	= 0;

	// 将源串每8个字节分为一组，压缩成7个字节
	// 循环该处理过程，直至源串被处理完
	// 如果分组不到8字节，也能正确处理
	while( nSrc < nSrcLength + 1 )
	{
		// 取源字符串的计数值的最低3位
		nChar = nSrc & 7;
		// 处理源串的每个字节
		if( nChar == 0 )
		{
			// 组内第一个字节，只是保存起来，待处理下一个字节时使用
			nLeft = *pSrc;
		}else
		{
			// 组内其它字节，将其右边部分与残余数据相加，得到一个目标编码字节
			*pDst = ( *pSrc << ( 8 - nChar ) ) | nLeft;
			// 将该字节剩下的左边部分，作为残余数据保存起来
			nLeft = *pSrc >> nChar;
			// 修改目标串的指针和计数值 pDst++;
			pDst++;
			nDst++;
		}

		// 修改源串的指针和计数值
		pSrc++; nSrc++;
	}

	// 返回目标串长度
	return nDst;
}

#define HALF_BYTE_SWAP( a ) ( ( ( ( a ) & 0x0f ) << 4 ) | ( ( a ) >> 4 ) )
#define GSM_7BIT	0x00
#define GSM_UCS2	0x08


/*
   236446 gsm<
   SCA      08 91683108200245F3
   PDU TYPE 40
   OA       05 A1 0180F6
   PID标志  00
   DCS编码  08
   SCTS服务中心时间戳 31809090346323
   UDL长度  8C
   050003690301
   60A8597DFF0C60A85404987959579910768451C65B9E65F66D888D3960C551B559824E0BFF1A0031300151687403901A554665C559579910003500380028003200300031003272480029FF0C56FD51854E3B53EB56FD518565F6957F0031003600305206949FFF1A5DF24F7F75280031003000375206949FFF0C672A4F7F7528003500335206
 */
uint8_t sms_decode_pdu( char *pinfo, uint16_t size )
{
	char		*p = pinfo, *pdst = pinfo;
	char		c,pid;
	uint8_t		tbl[24] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf };
	uint16_t	len;
	uint8_t		pdu_type, msg_len;
	uint16_t	i;
	uint8_t		dcs;
	char		sender[32];
	uint8_t		decode_msg[180]; /*解码后的信息*/
	uint16_t	decode_msg_len;

/*将OCT数据转为HEX,两个字节拼成一个*/
	for( i = 0; i < size; i++ )
	{
		c		= tbl[*p++ - '0'] << 4;
		c		|= tbl[*p++ - '0'];
		*pdst++ = c;
	}
	p = pinfo; /*重新定位到开始*/
/*SCA*/
	len = *p;	/*服务中心号码 Length (长度)+Tosca（服务中心类型）+Address（地址）*/
/*pdu类型*/
	p			+= ( len + 1 );
	pdu_type	= *p++;
	
/*OA来源地址*/
	len = *p++; 	/*手机发送源地址 Length (长度,是数字个数)+Tosca（地址类型）+Address（地址）*/
	len=(len+1)>>1;	 /*这里是数字位数，比如10086是5位 用三个字节表示*/
	p++;            /*跳过地址类型*/
	for( i = 0; i < len; i++ )
	{
		sender[i] = *p++;   /*这里没有半字节交互，发送的时候还得换过来*/
	}

/*PID标志*/
	pid=*p++;
/*DCS编码*/
	dcs = *p++;
/*SCTS服务中心时间戳*/
	p += 7;

/*消息长度*/
	msg_len = *p++;
/*是否为长信息pdu_type 的bit6 UDHI=1*/
	if( pdu_type & 0x40 ) /*跳过长信息的头*/
	{
		len = *p;
		len++;
		p		+= len;
		msg_len -= len;
	}
	rt_kprintf( "\nSMS>PDU Type=%02x PID=%02x DCS=%02x MSG_LEN=%d", pdu_type,pid,dcs,msg_len );

	if( ( dcs & 0x0c ) == GSM_7BIT )
	{
		decode_msg_len = gsmdecode7bit( p, decode_msg, msg_len );
	}else if( ( dcs & 0x0c ) == GSM_UCS2 )
	{
		decode_msg_len = gsmdecodeucs2( p, decode_msg, msg_len );
	}else
	{
		memcpy( decode_msg, p, msg_len );
		decode_msg_len = msg_len;   /*简单拷贝即可*/
	}

	if( decode_msg_len )            /*对于不是合法的数据是不是直接返回0*/
	{
		decode_msg[decode_msg_len]=0;
		jt808_sms_rx( sender, decode_msg, decode_msg_len );
	}
}

/*收到短信

   +CMTI: "SM",7
   236426 gsm>AT+CMGR=7


   236441 gsm<+CMGR: 0,156
   236446 gsm<0891683108200245F34005A10180F60008318090903463238C05000369030160A8597DFF0C60A85404987959579910768451C65B9E65F66D888D3960C551B559824E0BFF1A0031300151687403901A554665C559579910003500380028003200300031003272480029FF0C56FD51854E3B53EB56FD518565F6957F0031003600305206949FFF1A5DF24F7F75280031003000375206949FFF0C672A4F7F7528003500335206
   236519 gsm<OK

 */

enum _sms_state {
	SMS_IDLE=0,
	SMS_WAIT_CMGR_DATA,
	SMS_WAIT_CMGR_OK,
	SMS_WAIT_CMGD_OK,
};

enum _sms_state sms_state = SMS_IDLE;

static uint32_t sms_tick = 0;


/***********************************************************
* Function:
* Description:
* Input:
* Input:
* Output:
* Return:	0:没有短信处理内容 非零值 有后续的处理
* Others:
***********************************************************/
uint8_t sms_rx_proc( char *pinfo, uint16_t size )
{
	static int	st, index, count;
	char		buf[40]; /*160个OCT需要320byte*/
	uint32_t	tick = rt_tick_get( );

	switch( sms_state )
	{
		case SMS_IDLE:
			if( strncmp( pinfo, "+CMTI: \"SM\",", 12 ) == 0 )
			{
				if( sscanf( pinfo + 12, "%d", &index ) )
				{
					rt_mb_send(&mb_sms_rx,index);
				}
			}else if( strncmp( pinfo, "+CMGR: ", 7 ) == 0 )             /*+CMGR: 0,156*/
			{
				if( sscanf( pinfo + 7, "%d,%d", &st, &count ) == 2 )    /*得到信息的长度*/
				{
					sms_state = SMS_WAIT_CMGR_DATA;
				}
			}else if( strncmp( pinfo, "+CMT:", 5 ) == 0 )
			{
			}
			sms_tick = tick;
			break;
		case SMS_WAIT_CMGR_DATA:                    /*收到短信数据*/
			sms_decode_pdu( pinfo, size );          /*解析数据*/
			sms_state = SMS_WAIT_CMGR_OK;
			break;
		case SMS_WAIT_CMGR_OK:
			if( strncmp( pinfo, "OK", 2 ) == 0 )    /*读完了信息,删除*/
			{
				sprintf( buf, "AT+CMGD=%d", index );
				at( buf );
				sms_tick	= tick;
				sms_state	= SMS_WAIT_CMGD_OK;
			}
			break;
		case SMS_WAIT_CMGD_OK:
			if( strncmp( pinfo, "OK", 2 ) == 0 ) /*删除短信成功*/
			{
				sms_state = SMS_IDLE;
			}
			break;
	}

/*判断超时*/
	if( tick - sms_tick > RT_TICK_PER_SECOND * 10 )
	{
		sms_state = SMS_IDLE;
	}

	return sms_state;
}



void sms_proc(void)
{
	uint32_t i;
	char buf[40];
	if(rt_mb_recv(&mb_sms_rx,&i,0)==RT_EOK)		/*收到短信*/
	{
		sprintf( buf, "AT+CMGR=%d", i );
		at( buf );
		


	}


}





/*发送短信*/
void sms_tx( char *info )
{
}

void sms_init(void)
{
	rt_mb_init( &mb_sms_rx, "sms_rx", &mb_sms_rx_pool, MB_SMS_RX_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );
}

/************************************** The End Of File **************************************/

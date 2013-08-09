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
#include "m66_sms.h"
#include <stdio.h>
#include <string.h>


/* ��Ϣ������ƿ�*/
static struct rt_mailbox mb_sms_rx;
#define MB_SMS_RX_POOL_SIZE 32
/* ��Ϣ�������õ��ķ�����Ϣ���ڴ��*/
static uint8_t		mb_sms_rx_pool[MB_SMS_RX_POOL_SIZE];



/**/
u16  gsmencode8bit( const char *pSrc, char *pDst, u16 nSrcLength )
{
	u16 m;
	// �򵥸���
	for( m = 0; m < nSrcLength; m++ )
	{
		*pDst++ = *pSrc++;
	}

	return nSrcLength;
}

static const char	GB_DATA[]	= "����������ش�������������ɽ��ʹ����������³���������ԥ��";
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

/*ucs2�����ı���*/
u16 gsmdecodeucs2( char* pSrc, char* pDst, u16 nSrcLength )
{
	u16			nDstLength = 0; // UNICODE���ַ���Ŀ
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
		}else   /*Ҳ��˫�ֽڵ� ���� '1' ==> 0031*/
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
			ucs2=gb2312_to_ucs2(gb); /*�п���û���ҵ�����00*/
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
	// ����Ŀ����봮����
	return ( nSrcLength << 1 );
}

/**/
u16 gsmdecode7bit( const u8* pSrc, u8* pDst, u16 nSrcLength )
{
	u16 nSrc;   // Դ�ַ����ļ���ֵ
	u16 nDst;   // Ŀ����봮�ļ���ֵ
	u16 nByte;  // ��ǰ���ڴ���������ֽڵ���ţ���Χ��0-6
	u8	nLeft;  // ��һ�ֽڲ��������

	// ����ֵ��ʼ��
	nSrc	= 0;
	nDst	= 0;

	// �����ֽ���źͲ������ݳ�ʼ��
	nByte	= 0;
	nLeft	= 0;

	// ��Դ����ÿ7���ֽڷ�Ϊһ�飬��ѹ����8���ֽ�
	// ѭ���ô�����̣�ֱ��Դ���ݱ�������
	// ������鲻��7�ֽڣ�Ҳ����ȷ����
	while( nSrc < nSrcLength )
	{
		// ��Դ�ֽ��ұ߲��������������ӣ�ȥ�����λ���õ�һ��Ŀ������ֽ�
		*pDst = ( ( *pSrc << nByte ) | nLeft ) & 0x7f;
		// �����ֽ�ʣ�µ���߲��֣���Ϊ�������ݱ�������
		nLeft = *pSrc >> ( 7 - nByte );

		// �޸�Ŀ�괮��ָ��ͼ���ֵ
		pDst++;
		nDst++;
		// �޸��ֽڼ���ֵ
		nByte++;

		// ����һ������һ���ֽ�
		if( nByte == 7 )
		{
			// ����õ�һ��Ŀ������ֽ�
			*pDst = nLeft;

			// �޸�Ŀ�괮��ָ��ͼ���ֵ
			pDst++;
			nDst++;

			// �����ֽ���źͲ������ݳ�ʼ��
			nByte	= 0;
			nLeft	= 0;
		}

		// �޸�Դ����ָ��ͼ���ֵ
		pSrc++;
		nSrc++;
	}

	*pDst = 0;

	// ����Ŀ�괮����
	return nDst;
}

//��ÿ��ascii8λ�����Bit8ȥ�������ν���7λ����ĺ�λ����Ƶ�ǰ�棬�γ��µ�8λ���롣
u16 gsmencode7bit( const u8* pSrc, u8* pDst, u16 nSrcLength )
{
	u16 nSrc;   // Դ�ַ����ļ���ֵ
	u16 nDst;   // Ŀ����봮�ļ���ֵ
	u16 nChar;  // ��ǰ���ڴ���������ַ��ֽڵ���ţ���Χ��0-7
	u8	nLeft;  // ��һ�ֽڲ��������

	// ����ֵ��ʼ��
	nSrc	= 0;
	nDst	= 0;

	// ��Դ��ÿ8���ֽڷ�Ϊһ�飬ѹ����7���ֽ�
	// ѭ���ô�����̣�ֱ��Դ����������
	// ������鲻��8�ֽڣ�Ҳ����ȷ����
	while( nSrc < nSrcLength + 1 )
	{
		// ȡԴ�ַ����ļ���ֵ�����3λ
		nChar = nSrc & 7;
		// ����Դ����ÿ���ֽ�
		if( nChar == 0 )
		{
			// ���ڵ�һ���ֽڣ�ֻ�Ǳ�����������������һ���ֽ�ʱʹ��
			nLeft = *pSrc;
		}else
		{
			// ���������ֽڣ������ұ߲��������������ӣ��õ�һ��Ŀ������ֽ�
			*pDst = ( *pSrc << ( 8 - nChar ) ) | nLeft;
			// �����ֽ�ʣ�µ���߲��֣���Ϊ�������ݱ�������
			nLeft = *pSrc >> nChar;
			// �޸�Ŀ�괮��ָ��ͼ���ֵ pDst++;
			pDst++;
			nDst++;
		}

		// �޸�Դ����ָ��ͼ���ֵ
		pSrc++; nSrc++;
	}

	// ����Ŀ�괮����
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
   PID��־  00
   DCS����  08
   SCTS��������ʱ��� 31809090346323
   UDL����  8C
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
	uint8_t		decode_msg[180]; /*��������Ϣ*/
	uint16_t	decode_msg_len;

/*��OCT����תΪHEX,�����ֽ�ƴ��һ��*/
	for( i = 0; i < size; i++ )
	{
		c		= tbl[*p++ - '0'] << 4;
		c		|= tbl[*p++ - '0'];
		*pdst++ = c;
	}
	p = pinfo; /*���¶�λ����ʼ*/
/*SCA*/
	len = *p;	/*�������ĺ��� Length (����)+Tosca�������������ͣ�+Address����ַ��*/
/*pdu����*/
	p			+= ( len + 1 );
	pdu_type	= *p++;
	
/*OA��Դ��ַ*/
	len = *p++; 	/*�ֻ�����Դ��ַ Length (����,�����ָ���)+Tosca����ַ���ͣ�+Address����ַ��*/
	len=(len+1)>>1;	 /*����������λ��������10086��5λ �������ֽڱ�ʾ*/
	p++;            /*������ַ����*/
	for( i = 0; i < len; i++ )
	{
		sender[i] = *p++;   /*����û�а��ֽڽ��������͵�ʱ�򻹵û�����*/
	}

/*PID��־*/
	pid=*p++;
/*DCS����*/
	dcs = *p++;
/*SCTS��������ʱ���*/
	p += 7;

/*��Ϣ����*/
	msg_len = *p++;
/*�Ƿ�Ϊ����Ϣpdu_type ��bit6 UDHI=1*/
	if( pdu_type & 0x40 ) /*��������Ϣ��ͷ*/
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
		decode_msg_len = msg_len;   /*�򵥿�������*/
	}

	if( decode_msg_len )            /*���ڲ��ǺϷ��������ǲ���ֱ�ӷ���0*/
	{
		decode_msg[decode_msg_len]=0;
		jt808_sms_rx( sender, decode_msg, decode_msg_len );
	}
}

/*�յ�����

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
* Return:	0:û�ж��Ŵ������� ����ֵ �к����Ĵ���
* Others:
***********************************************************/
uint8_t sms_rx_proc( char *pinfo, uint16_t size )
{
	static int	st, index, count;
	char		buf[40]; /*160��OCT��Ҫ320byte*/
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
				if( sscanf( pinfo + 7, "%d,%d", &st, &count ) == 2 )    /*�õ���Ϣ�ĳ���*/
				{
					sms_state = SMS_WAIT_CMGR_DATA;
				}
			}else if( strncmp( pinfo, "+CMT:", 5 ) == 0 )
			{
			}
			sms_tick = tick;
			break;
		case SMS_WAIT_CMGR_DATA:                    /*�յ���������*/
			sms_decode_pdu( pinfo, size );          /*��������*/
			sms_state = SMS_WAIT_CMGR_OK;
			break;
		case SMS_WAIT_CMGR_OK:
			if( strncmp( pinfo, "OK", 2 ) == 0 )    /*��������Ϣ,ɾ��*/
			{
				sprintf( buf, "AT+CMGD=%d", index );
				at( buf );
				sms_tick	= tick;
				sms_state	= SMS_WAIT_CMGD_OK;
			}
			break;
		case SMS_WAIT_CMGD_OK:
			if( strncmp( pinfo, "OK", 2 ) == 0 ) /*ɾ�����ųɹ�*/
			{
				sms_state = SMS_IDLE;
			}
			break;
	}

/*�жϳ�ʱ*/
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
	if(rt_mb_recv(&mb_sms_rx,&i,0)==RT_EOK)		/*�յ�����*/
	{
		sprintf( buf, "AT+CMGR=%d", i );
		at( buf );
		


	}


}





/*���Ͷ���*/
void sms_tx( char *info )
{
}

void sms_init(void)
{
	rt_mb_init( &mb_sms_rx, "sms_rx", &mb_sms_rx_pool, MB_SMS_RX_POOL_SIZE / 4, RT_IPC_FLAG_FIFO );
}

/************************************** The End Of File **************************************/
